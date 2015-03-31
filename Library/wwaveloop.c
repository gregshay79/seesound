//#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <mmsystem.h>
#include "wwaveloop.h"

#define WAVEAUDIO_NBUFS 2



int waveaudio_init(struct WAVEAUDIO *wa, int samplerate, int channels, int bitspersample,
				   int bufsize_samples)
{
	MMRESULT mmres;
	int bufsize, i;

	memset(wa, 0, sizeof(*wa));
	wa->he = CreateEvent(0, FALSE, FALSE, 0); /* Automatic event */
	if(!wa->he)
		return -1;
	/* Initialize format descriptor */
	wa->wfx.wFormatTag = WAVE_FORMAT_PCM;
	wa->wfx.nChannels = channels;
	wa->wfx.nSamplesPerSec = samplerate;
	wa->wfx.wBitsPerSample = bitspersample;
	wa->wfx.nBlockAlign = wa->wfx.nChannels * wa->wfx.wBitsPerSample / 8;
	wa->wfx.nAvgBytesPerSec = wa->wfx.nSamplesPerSec * wa->wfx.nBlockAlign;

//	i = IsFormatSupported(&wa->wfx, WAVE_MAPPER);
//	if (i!=0) {
//		printf("Format not supported \n");
//		return -2;
//	}
	/* Open wave in device */
	mmres = waveInOpen(&wa->hwi, WAVE_MAPPER, &wa->wfx, (DWORD)wa->he, 0, CALLBACK_EVENT);
	if(mmres != MMSYSERR_NOERROR)
		return mmres;
	/* Open wave out device */
	mmres = waveOutOpen(&wa->hwo, WAVE_MAPPER, &wa->wfx, (DWORD)wa->he, 0, CALLBACK_EVENT);
	if(mmres != MMSYSERR_NOERROR) {
		waveInClose(wa->hwi);
		return mmres;
	}

	bufsize = bufsize_samples * wa->wfx.nBlockAlign;

	/* Prepare record buffers */
	for(i = 0; i < WAVEAUDIO_NBUFS; i++) {
		WAVEHDR *wh = &wa->whin[i];
		wh->dwBufferLength = bufsize;
		wh->lpData = (LPSTR)malloc(wh->dwBufferLength);
		if(!wh->lpData)
			return -1; /* out of mem */
		mmres = waveInPrepareHeader(wa->hwi, wh, sizeof(*wh));
		if(mmres != MMSYSERR_NOERROR) 
			return mmres;
		wh->dwFlags |= WHDR_DONE;
	}

	/* Prepare playback buffers */
	for(i = 0; i < WAVEAUDIO_NBUFS; i++) {
		WAVEHDR *wh = &wa->whout[i];
		wh->dwBufferLength = bufsize;
		wh->lpData = (LPSTR)malloc(wh->dwBufferLength);
		if(!wh->lpData)
			return -1; /* out of mem */
		mmres = waveOutPrepareHeader(wa->hwo, wh, sizeof(*wh));
		if(mmres != MMSYSERR_NOERROR) 
			return mmres;
		wh->dwFlags |= WHDR_DONE;
	}

	/* Queue all rec bufs */
	for(i = 0; i < WAVEAUDIO_NBUFS; i++) {
		mmres = waveInAddBuffer(wa->hwi, &wa->whin[i], sizeof(wa->whin[i]));
		if(mmres) return mmres;
	}

	return waveInStart(wa->hwi);
}

void waveaudio_cleanup(struct WAVEAUDIO *wa)
{
	int i;
	WAVEHDR *wh;

	waveInStop(wa->hwi);
	waveInReset(wa->hwi);
	waveOutReset(wa->hwo);

	for(i = 0; i < WAVEAUDIO_NBUFS; i++) {
		wh = &wa->whout[i];	
		waveOutUnprepareHeader(wa->hwo, wh, sizeof(*wh));
		if (wh->lpData)
			free(wh->lpData);

		wh = &wa->whin[i];
		waveInUnprepareHeader(wa->hwi, wh, sizeof(*wh));
		if (wh->lpData)
			free(wh->lpData);
	}

	waveInClose(wa->hwi);
	waveOutClose(wa->hwo);
	CloseHandle(wa->he);
}

void process_samples(LPSTR lpDataIn, LPSTR lpDataOut, DWORD bytes)
{
	memcpy(lpDataOut, lpDataIn, bytes);
}

int main(int argc, char *argv[])
{
	struct WAVEAUDIO wa;
	MMRESULT mmres;
	int err,result;
	float playbacktime = 0.0;

	err = waveaudio_init(&wa, 44100, 2, 16, 4096 /* 44100/4 */ );
	if(err) {
		printf("waveaudio_init failed, value=%d\n",err);
		return err;
	}

	do {
		/* Wait for record and playback to finish on ibuf */
		
		while(!(wa.whin[wa.ibuf].dwFlags & WHDR_DONE) || !(wa.whout[wa.ibuf].dwFlags & WHDR_DONE))
			result = WaitForSingleObject(wa.he, 10 /*INFINITE*/);

		if (result==WAIT_OBJECT_0) {

			process_samples(wa.whin[wa.ibuf].lpData, wa.whout[wa.ibuf].lpData, wa.whin[wa.ibuf].dwBufferLength);
			/* Queue record ibuf */
			mmres = waveInAddBuffer(wa.hwi, &wa.whin[wa.ibuf], sizeof(wa.whin[wa.ibuf]));
			if(mmres) return 2;
			/* Play ibuf */
			mmres = waveOutWrite(wa.hwo, &wa.whout[wa.ibuf], sizeof(wa.whout[wa.ibuf]));
			if(++wa.ibuf >= WAVEAUDIO_NBUFS)
				wa.ibuf = 0;

			playbacktime += (float)wa.whin[wa.ibuf].dwBufferLength / (float)wa.wfx.nAvgBytesPerSec;
			printf("\r%c %.1fs", "|-"[wa.ibuf], playbacktime); /* simple ticker */
		}

	} while(!_kbhit());

	//tear stuff down	
	waveaudio_cleanup(&wa);
	
	return 0;
}
