//#include <windows.h>
//#include <conio.h>
//#include <stdio.h>
#include <mmsystem.h>

#define WAVEAUDIO_NBUFS 2

struct WAVEAUDIO {
	WAVEFORMATEX wfx;
	HWAVEIN hwi;
	HWAVEOUT hwo;
	HANDLE he; /* synchronization event */
	WAVEHDR whin[WAVEAUDIO_NBUFS], whout[WAVEAUDIO_NBUFS]; 
	int ibuf;
};

void process_samples(LPSTR lpDataIn, LPSTR lpDataOut, DWORD bytes);
int waveaudio_init(struct WAVEAUDIO *wa, int samplerate, int channels, int bitspersample, int bufsize_samples);
void waveaudio_cleanup(struct WAVEAUDIO *wa);