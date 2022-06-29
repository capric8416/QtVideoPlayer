#include "FileMemoryContext.h"

#include <cstdio>
#include <cassert>

extern "C"
{
	#include <libavformat/avformat.h>
}


int read_packet(void *opaque, uint8_t *buf, int buf_size);


uint8_t *FileMemoryContext::m_pBuf(nullptr);
uint64_t FileMemoryContext::m_nBufSize(0);


FileMemoryContext::FileMemoryContext()
	: m_pIoContext(nullptr)
	, m_pIoBuf(nullptr)

	, m_nBufOffset(0)
{
	// ģ���ļ�IO����
	m_pIoBuf = (uint8_t *)av_malloc(32768);
	m_pIoContext = avio_alloc_context(m_pIoBuf, 32768, 0, (void*)(this), &read_packet, nullptr, nullptr);
}


FileMemoryContext::~FileMemoryContext()
{
	// ��Ϊavformat_close_input�����avio_close�����Դ˴������ͷ�
	// libavformat/demux.c#378
	//if (m_pIoContext != nullptr)
	//{
	//	avio_close(m_pIoContext);
	//	m_pIoContext = nullptr;
	//}

	// ��Ϊavio_close�����av_freep�����Դ˴������ͷ�
	// libavformat/aviobuf.c#1199
	//if (m_pIoBuf != nullptr)
	//{
	//	av_free(m_pIoBuf);
	//	m_pIoBuf = nullptr;
	//}
}


void FileMemoryContext::Load(const char *path)
{
	// �������ļ������ڴ�
	// ���ļ�
	FILE *fp;
	errno_t error = fopen_s(&fp, path, "rb");
	assert(error == 0);

	// ��ȡ�ļ���С
	fseek(fp, 0, SEEK_END);
	m_nBufSize = ftell(fp);

	// ��ȡ�ļ�����
	rewind(fp);
	m_pBuf = new uint8_t[m_nBufSize];
	size_t readSize = fread_s(m_pBuf, m_nBufSize, sizeof(uint8_t), m_nBufSize, fp);
	assert(readSize == m_nBufSize);

	// �ر��ļ�
	fclose(fp);
}


void FileMemoryContext::Unload()
{
	if (m_pBuf != nullptr)
	{
		delete[] m_pBuf;
		m_pBuf = nullptr;
	}
}


AVIOContext* FileMemoryContext::Get()
{
	return m_pIoContext;
}


int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
	return static_cast<FileMemoryContext*>(opaque)->Read(buf, buf_size);
}


int FileMemoryContext::Read(uint8_t *buf, int buf_size)
{
	int64_t size = buf_size;
	if (size > m_nBufSize - m_nBufOffset) {
		size = m_nBufSize - m_nBufOffset;
	}

	if (size <= 0) {
		m_nBufOffset = 0; // ���¿�ʼ
		return 0;
	}

	memcpy_s(buf, buf_size, m_pBuf + m_nBufOffset, size);

	m_nBufOffset += size;

	return size;
}
