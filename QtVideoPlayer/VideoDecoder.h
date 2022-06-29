#pragma once


struct AVBufferRef;
struct AVCodec;
struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVIOContext;
struct AVPacket;
class FileMemoryContext;

typedef struct DecodeContext {
	AVBufferRef *hw_device_ref;
} DecodeContext;


class VideoDecoder
{
public:
    VideoDecoder();
    ~VideoDecoder();

    void Initialize();
    void Deinitialization();

    bool DecodeOneFrame();
    AVFrame* GetOneFrame();
    void ReleaseOneFrame();

    int GetVideoWidth();
    int GetVideoHeight();

private:
	DecodeContext m_decodeContex;
    AVCodec *m_pDecoder;
    AVCodecContext *m_pDecoderContex;
	FileMemoryContext *m_pIoContext;
    AVFormatContext *m_pFormatContex;
    AVPacket *m_pInputPacket;
	AVFrame *m_pOutputFrame;
	AVFrame *m_pOutputSWFrame;

    int m_nVideoindex;

    int m_nVideoWidth;
    int m_nVideoHeight;
};

