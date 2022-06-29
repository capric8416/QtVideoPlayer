#pragma once

#include <stdint.h>


struct AVIOContext;

// ffmpeg/doc/examples/avio_reading.c
class FileMemoryContext
{
public:
	FileMemoryContext();
	~FileMemoryContext();

	static void Load(const char *path);
	static void Unload();

	AVIOContext* Get();

	int Read(uint8_t *buf, int buf_size);

private:
	AVIOContext *m_pIoContext;
	uint8_t *m_pIoBuf;
	static uint8_t *m_pBuf;

	static uint64_t m_nBufSize;
	uint64_t m_nBufOffset;
};
