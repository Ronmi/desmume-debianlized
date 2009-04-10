/*
	Microphone emulation for Win32

	The NDS microphone produces 8-bit sound sampled at 16khz.
	The sound data must be read sample-by-sample through the 
	ARM7 SPI device (touchscreen controller, channel 6).

	Note : I added these notes because the microphone isn't 
	documented on GBATek.
*/
#include <windows.h>
#include "../types.h"
#include "../debug.h"
#include "../mic.h"
#include <iostream>
#include <fstream>

int MicDisplay;
int MicButtonPressed=0;
int SampleLoaded=0;

#define MIC_CHECKERR(hr) if(hr != MMSYSERR_NOERROR) return FALSE;

#define MIC_BUFSIZE 4096

BOOL Mic_Inited = FALSE;

u8 Mic_TempBuf[MIC_BUFSIZE];
u8 Mic_Buffer[2][MIC_BUFSIZE];
u16 Mic_BufPos;
u8 Mic_WriteBuf;
u8 Mic_PlayBuf;

HWAVEIN waveIn;
WAVEHDR waveHdr;

static int CALLBACK waveInProc(HWAVEIN wavein, UINT msg, DWORD instance, DWORD param1, DWORD param2)
{
	LPWAVEHDR lpWaveHdr;
	HRESULT hr;

	if(!Mic_Inited)
		return 1;

	if(msg == WIM_DATA)
	{
		lpWaveHdr = (LPWAVEHDR)param1;

		memcpy(Mic_Buffer[Mic_WriteBuf], lpWaveHdr->lpData, MIC_BUFSIZE);
		Mic_WriteBuf ^= 1;

		hr = waveInAddBuffer(waveIn, lpWaveHdr, sizeof(WAVEHDR));
		if(hr != MMSYSERR_NOERROR)
			return 1;
	}

	return 0;
}

char* samplebuffer;
int samplebuffersize;
FILE* fp;

char* LoadSample(const char *name)
{

	std::ifstream fl(name);
	fl.seekg( 0, std::ios::end );
	size_t len = fl.tellg();
	samplebuffersize=len;
	char *ret = new char[len];
	fl.seekg(0, std::ios::beg);
	fl.read(ret, len);
	samplebuffer=ret;
	SampleLoaded=1;
	return ret;
}

BOOL Mic_Init() {

	if(Mic_Inited)
		return TRUE;

	Mic_Inited = FALSE;

	HRESULT hr;
	WAVEFORMATEX wfx;

	memset(Mic_TempBuf, 0, MIC_BUFSIZE);
	memset(Mic_Buffer[0], 0, MIC_BUFSIZE);
	memset(Mic_Buffer[1], 0, MIC_BUFSIZE);
	Mic_BufPos = 0;

	Mic_WriteBuf = 0;
	Mic_PlayBuf = 1;

	memset(&wfx, 0, sizeof(wfx));
	wfx.cbSize = 0;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = 16000;
	wfx.nBlockAlign = 1;
	wfx.nAvgBytesPerSec = 16000;
	wfx.wBitsPerSample = 8;

	hr = waveInOpen(&waveIn, WAVE_MAPPER, &wfx, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
	MIC_CHECKERR(hr)

	memset(&waveHdr, 0, sizeof(waveHdr));
	waveHdr.lpData = (LPSTR)Mic_TempBuf;
	waveHdr.dwBufferLength = MIC_BUFSIZE;

	hr = waveInPrepareHeader(waveIn, &waveHdr, sizeof(WAVEHDR));
	MIC_CHECKERR(hr)

	hr = waveInAddBuffer(waveIn, &waveHdr, sizeof(WAVEHDR));
	MIC_CHECKERR(hr)

	hr = waveInStart(waveIn);
	MIC_CHECKERR(hr)

	Mic_Inited = TRUE;
	return TRUE;
}

void Mic_Reset()
{
	if(!Mic_Inited)
		return;

	memset(Mic_TempBuf, 0, MIC_BUFSIZE);
	memset(Mic_Buffer[0], 0, MIC_BUFSIZE);
	memset(Mic_Buffer[1], 0, MIC_BUFSIZE);
	Mic_BufPos = 0;

	Mic_WriteBuf = 0;
	Mic_PlayBuf = 1;
}

void Mic_DeInit()
{
	if(!Mic_Inited)
		return;

	Mic_Inited = FALSE;

	waveInReset(waveIn);
	waveInClose(waveIn);
}

int random[32] = {0xB1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE9, 0x70, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x28, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x20, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE9};

int x=0;

u8 Mic_ReadSample()
{

	if(!Mic_Inited)
		return 0;

	u8 ret;

	if(MicButtonPressed)
		if(SampleLoaded) {  //use a sample
			x++;
			if(x > samplebuffersize)
				x=0;
			ret=samplebuffer[x];
		}
		else {  //use the "random" values
			x++;
			if(x > ARRAYSIZE(random))
				x=0;
			ret = random[x];
		}
	else { //normal mic behavior
		u8 tmp = (u8)Mic_Buffer[Mic_BufPos >> 1];

		if(Mic_BufPos & 0x1)
		{
			ret = ((tmp & 0x1) << 7);
		}
		else
		{
			ret = ((tmp & 0xFE) >> 1);
		}
	}

	Mic_BufPos++;
	if(Mic_BufPos >= (MIC_BUFSIZE << 1))
	{
		Mic_BufPos = 0;
		Mic_PlayBuf ^= 1;
	}

	MicDisplay = ret;

	return ret;
}