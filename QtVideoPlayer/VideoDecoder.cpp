#include "VideoDecoder.h"
#include "FileMemoryContext.h"

#include <cassert>


extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavdevice/avdevice.h>
    #include <libavformat/version.h>
    #include <libavutil/time.h>
    #include <libavutil/mathematics.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/pixfmt.h>
    #include <libswresample/swresample.h>
	#include <libavutil/hwcontext.h>
	#include <libavutil/hwcontext_qsv.h>
}



VideoDecoder::VideoDecoder()
	: m_decodeContex()
    , m_pDecoder(nullptr)
    , m_pDecoderContex(nullptr)
	, m_pIoContext(nullptr)
    , m_pFormatContex(nullptr)
    , m_pInputPacket(nullptr)
	, m_pOutputFrame(nullptr)
	, m_pOutputSWFrame(nullptr)

    , m_nVideoindex(-1)
    , m_nVideoWidth(0)
    , m_nVideoHeight(0)
{
    Initialize();
}


VideoDecoder::~VideoDecoder()
{
    Deinitialization();
}



static enum AVPixelFormat get_format(AVCodecContext *avctx, const enum AVPixelFormat *pix_fmts)
{
	while (*pix_fmts != AV_PIX_FMT_NONE) {
		if (*pix_fmts == AV_PIX_FMT_QSV) {
			/* create a pool of surfaces to be used by the decoder */
			DecodeContext *decode = (DecodeContext *)avctx->opaque;
			avctx->hw_frames_ctx = av_hwframe_ctx_alloc(decode->hw_device_ref);
			if (!avctx->hw_frames_ctx) {
				return AV_PIX_FMT_NONE;
			}

			AVHWFramesContext  *frames_ctx;
			frames_ctx = (AVHWFramesContext*)avctx->hw_frames_ctx->data;
			frames_ctx->format = AV_PIX_FMT_QSV;
			frames_ctx->sw_format = avctx->sw_pix_fmt;
			frames_ctx->width = FFALIGN(avctx->coded_width, 32);
			frames_ctx->height = FFALIGN(avctx->coded_height, 32);
			frames_ctx->initial_pool_size = 32;

			AVQSVFramesContext *frames_hwctx;
			frames_hwctx = (AVQSVFramesContext *)frames_ctx->hwctx;
			frames_hwctx->frame_type = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;

			if (av_hwframe_ctx_init(avctx->hw_frames_ctx) < 0) {
				return AV_PIX_FMT_NONE;
			}

			return AV_PIX_FMT_QSV;
		}

		pix_fmts++;
	}

	assert(false);

	return AV_PIX_FMT_NONE;
}


void VideoDecoder::Initialize()
{
    m_pFormatContex = avformat_alloc_context();

	m_pIoContext = new FileMemoryContext();
	m_pFormatContex->pb = m_pIoContext->Get();

    int ret = 0;
    ret = avformat_open_input(&m_pFormatContex, "", nullptr, nullptr);
	assert(ret == 0);
    ret = avformat_find_stream_info(m_pFormatContex, nullptr);
	assert(ret == 0);

	AVStream *pVideoStream = nullptr;
    for(unsigned int i = 0; i < m_pFormatContex->nb_streams; i++) {
        if(m_pFormatContex->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && (m_pFormatContex->streams[i]->codecpar->codec_id == AV_CODEC_ID_H264 || m_pFormatContex->streams[i]->codecpar->codec_id == AV_CODEC_ID_H265)) {
           m_nVideoindex = static_cast<int>(i);
		   pVideoStream = m_pFormatContex->streams[i];
           break;
       }
    }
	assert(pVideoStream);
	
	AVCodecID codecId = m_pFormatContex->streams[m_nVideoindex]->codecpar->codec_id;
	const char *codecName = "";
	if (codecId == AV_CODEC_ID_H264) {
		codecName = "h264_qsv";
	}
	else if (codecId == AV_CODEC_ID_H265) {
		codecName = "hevc_qsv";
	}
	assert(strlen(codecName) != 0);

	/* open the hardware device */
	ret = av_hwdevice_ctx_create(&m_decodeContex.hw_device_ref, AV_HWDEVICE_TYPE_QSV, "auto", NULL, 0);
	assert(ret >= 0);

	/* initialize the decoder */
	m_pDecoder = (AVCodec *)avcodec_find_decoder_by_name(codecName);
	assert(m_pDecoder);

    m_pDecoderContex = avcodec_alloc_context3(m_pDecoder);
	avcodec_parameters_to_context(m_pDecoderContex, m_pFormatContex->streams[m_nVideoindex]->codecpar);
	m_pDecoderContex->opaque = &m_decodeContex;
	m_pDecoderContex->get_format = get_format;
    
	ret = avcodec_open2(m_pDecoderContex, m_pDecoder, nullptr);
	assert(ret == 0);

    m_nVideoWidth = m_pDecoderContex->width;
    m_nVideoHeight = m_pDecoderContex->height;

    m_pInputPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
    m_pOutputFrame = av_frame_alloc();
	m_pOutputSWFrame = av_frame_alloc();
}


void VideoDecoder::Deinitialization()
{
	if (m_pIoContext != nullptr) {
		delete m_pIoContext;
		m_pIoContext = nullptr;
	}

    if(m_pFormatContex != nullptr) {
		avformat_close_input(&m_pFormatContex);
        m_pFormatContex = nullptr;
    }

    if(m_pInputPacket != nullptr) {
        av_packet_free(&m_pInputPacket);
        m_pInputPacket = nullptr;
    }

    if(m_pOutputFrame != nullptr) {
        av_frame_free(&m_pOutputFrame);
        m_pOutputFrame = nullptr;
    }

	if (m_pOutputSWFrame != nullptr) {
		av_frame_free(&m_pOutputSWFrame);
		m_pOutputSWFrame = nullptr;
	}

	if (m_pDecoderContex != nullptr) {
		avcodec_free_context(&m_pDecoderContex);
		m_pDecoderContex = nullptr;

		av_buffer_unref(&m_decodeContex.hw_device_ref);
	}
}


#ifdef DEBUG_DECODE_ELAPSED_TIME
#include <QElapsedTimer>
#include <QDebug>
#endif // DEBUG_DECODE_ELAPSED_TIME


bool VideoDecoder::DecodeOneFrame()
{
#ifdef DEBUG_DECODE_ELAPSED_TIME
	QElapsedTimer elapsedTimer;
	elapsedTimer.start();
#endif // DEBUG_DECODE_ELAPSED_TIME

    while(av_read_frame(m_pFormatContex, m_pInputPacket) >= 0) {
        if (m_pInputPacket->stream_index == m_nVideoindex) {
            int ret = avcodec_send_packet(m_pDecoderContex, m_pInputPacket);            
            av_packet_unref(m_pInputPacket);
            if (ret < 0) {
                return false;
            }


            while (ret >= 0) {
                ret = avcodec_receive_frame(m_pDecoderContex, m_pOutputFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    av_frame_unref(m_pOutputFrame);
                    break;
                }
                else if (ret < 0) {
                    av_frame_unref(m_pOutputFrame);
                    return false;
                }

#ifdef DEBUG_DECODE_ELAPSED_TIME
				qDebug() << "avcodec_send_packet + avcodec_receive_frame = " << elapsedTimer.elapsed() << "ms";
				elapsedTimer.restart();
#endif // DEBUG_DECODE_ELAPSED_TIME

				ret = av_hwframe_transfer_data(m_pOutputSWFrame, m_pOutputFrame, 0);
				if (ret < 0) {
					av_frame_unref(m_pOutputFrame);
					av_frame_unref(m_pOutputSWFrame);
					return false;
				}

#ifdef DEBUG_DECODE_ELAPSED_TIME
				qDebug() << "av_hwframe_transfer_data = " << elapsedTimer.elapsed() << "ms";
#endif // DEBUG_DECODE_ELAPSED_TIME

                return true;
            }
        }
    }

    return false;
}


AVFrame* VideoDecoder::GetOneFrame()
{
    return m_pOutputSWFrame;
}


void VideoDecoder::ReleaseOneFrame()
{
	av_frame_unref(m_pOutputFrame);
	av_frame_unref(m_pOutputSWFrame);
}


int VideoDecoder::GetVideoWidth()
{
    return m_nVideoWidth;
}


int VideoDecoder::GetVideoHeight()
{
    return m_nVideoHeight;
}
