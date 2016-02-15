// SeeSound.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "LPCSound.h"
//#include <stdio.h>
//#include <mmsystem.h>
#include <math.h>
#include <float.h>
#include <WinBase.h>
#include <CommCtrl.h>

#include "../library/wwaveloop.h"
#include "../library/DisplayFunctions.h"

#include "cw.h"
#include "configuration.h"

#define MAX_LOADSTRING 100


#define AUDIO_BUFFER_SIZE 512
#define MATH_BUFFER_SIZE AUDIO_BUFFER_SIZE
#define RM_SIZE MATH_BUFFER_SIZE
#define FFT_SIZE MATH_BUFFER_SIZE
#define SAMPLE_FREQ 8000

#define FFT
//#define GAMMATONE

#define AUTOCORR_SIZE 256		
#define NUM_CHANNELS 512

//Display size
#define XMAX 1366
#define YMAX 768

//#define PLAYBACK

void fft(double *xinr,double *xini,double *xoutr,double *xouti,int n,int forward);
void genblack(double *win, int n);
double biquad(double input, double fc[],double z[]);
double BQhipass300[],BQhipass2k[],BQlopass1k[],BQhipass1k[];
void makeRateMap(double *ratemap,int *numframesExport, double *x, int nsamples, int fs, 
				 double lowcf, double highcf, int numchans, double frameshift, double ti, int compression_type);


// Global Variables:
HINSTANCE hInst=0;								// current instance
HWND hWnd = 0;								// Main window handle
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND hTrackWnd, hTrackWnd2;
int terminate_flag = -100;


short abuff[AUDIO_BUFFER_SIZE*2];
double dbuffr[MATH_BUFFER_SIZE],dbuffi[MATH_BUFFER_SIZE],fbuffr[MATH_BUFFER_SIZE],fbuffi[MATH_BUFFER_SIZE];
double filterr[MATH_BUFFER_SIZE], filteri[MATH_BUFFER_SIZE];
double cbuffr[MATH_BUFFER_SIZE],cbuffi[MATH_BUFFER_SIZE];
double wbuff[MATH_BUFFER_SIZE];
double exbuff[MATH_BUFFER_SIZE*2];
double autocorr[MATH_BUFFER_SIZE], autocorrd[MATH_BUFFER_SIZE], autocorrFilt[MATH_BUFFER_SIZE];

double dwin[MATH_BUFFER_SIZE],postmultr[MATH_BUFFER_SIZE],postmulti[MATH_BUFFER_SIZE];
double scrbuff[MATH_BUFFER_SIZE],scrbuffb[MATH_BUFFER_SIZE];
double gbuff[MATH_BUFFER_SIZE];

double ratemap[64][NUM_CHANNELS];
double pi = 3.1415926535897932384643383;
int numFramesExported=0;

double autocorr_scale = 1./(AUTOCORR_SIZE*32768.0);
double autocorrAlpha=.5;

double GLOBAL_inputgain = 1.;

_int64 tstamp1,tdiff,tstamp2=0;
_int64 tmax,tmin;
double cpu_freq_factor;

extern wchar_t dummybuffer[];

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

double filterstate[8];

double gain=2.0;
double GTVgain = 10.;
double max,maxi;


cw decoder;

int timedown = 0;


void marktimeStart(){
	QueryPerformanceCounter((LARGE_INTEGER*)&tstamp1);
}

void marktimeEnd(){
	QueryPerformanceCounter((LARGE_INTEGER*)&tstamp2);

	if (--timedown<0) {
		timedown = 200;
		tmin = 99999999;
		tmax = 0;
	}

	tdiff = tstamp2 - tstamp1;
	if (tstamp2 != 0) {
		if (tdiff > tmax) tmax = tdiff;
		if (tdiff < tmin) tmin = tdiff;
	}
}

DWORD WINAPI doWindowsStuff(LPVOID param)
{
	MSG msg;
	int nCmdShow = *(int*)param;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInst, IDC_LPCSOUND, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInst);

	hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0 /*CW_USEDEFAULT*/, 0, 1200 /*CW_USEDEFAULT*/, 1064, NULL, NULL, hInst, NULL);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDC_LPCSOUND));

	// try a trackbar from the main window
	hTrackWnd = CreateWindowW(TRACKBAR_CLASS,
		L"Bob",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | /* TBS_AUTOTICKS*/ TBS_NOTICKS | WS_BORDER,
		2, YMAX - 228,
		200, 28,
		hWnd,
		NULL,
		hInst,
		NULL
		);

	SendMessage(hTrackWnd, TBM_SETRANGE, 1, MAKELONG(0, 200));
	SendMessage(hTrackWnd, TBM_SETPOS, 1, 50);
	UpdateWindow(hTrackWnd);


	// Main message loop:
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	// Here the program is supposed to exit..
	terminate_flag = msg.wParam;
	DestroyWindow(hTrackWnd);
	DestroyWindow(hTrackWnd2);
//	DestroyWindow(hWnd);

	ExitThread(0);
	return 0;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	struct WAVEAUDIO wa;
	MMRESULT mmres;
	int err,result;
	float playbacktime = 0.0;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
//	MSG msg;
	HACCEL hAccelTable;
//	HWND hWndMain=0;
	wchar_t pbuff[80];
	HDC hdc;
	int i,j,wcolor;
	double sum;
	bool notdone;
	double freq=128.,dfreq=-1.;
	double sigval;
	HPEN redPen,greenPen;
	HGDIOBJ  defpen;
	HGDIOBJ prevObj;
	int mcount = 10;
	DWORD threadID = 0;

	//	fesetenv(FE_DFL_DISABLE_SSE_DENORMS_ENV);
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON | 0x0040 | _MM_MASK_UNDERFLOW | _MM_MASK_DENORM);

#if 0
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_LPCSOUND, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
#endif

	QueryPerformanceFrequency((LARGE_INTEGER*)&tstamp1);
	cpu_freq_factor = 1.0e6/((double)tstamp1);
	// Perform application initialization:
	hInst = hInstance; // Store instance handle in our global variable

	// Spawn off a thread here to pump the message loop:
	if (!CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)doWindowsStuff, (LPVOID)&nCmdShow, 0, &threadID))
		exit(0);

	while (!hWnd);  // Wait here until hWnd created.
#if 0
	hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		0 /*CW_USEDEFAULT*/, 0, 1200 /*CW_USEDEFAULT*/,1064, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LPCSOUND));

	// try a trackbar from the main window
	hTrackWnd = CreateWindowW(TRACKBAR_CLASS,
		L"Bob",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | /* TBS_AUTOTICKS*/ TBS_NOTICKS | WS_BORDER,
		2, YMAX - 228,
		200, 28,
		hWnd,
		NULL,
		hInst,
		NULL
		);

	SendMessage(hTrackWnd, TBM_SETRANGE, 1, MAKELONG(0, 200));
	SendMessage(hTrackWnd, TBM_SETPOS, 1, 50);
//	UpdateWindow(hWnd);
#endif
//----------------------------------------------------------------------
	err = waveaudio_init(&wa, SAMPLE_FREQ, 2, 16, AUDIO_BUFFER_SIZE /* 44100/4 */ );
	if(err) {
		swprintf(pbuff,80,L"waveaudio_init failed, return code=%d\n",err);
		MessageBox(hWnd,pbuff,NULL,MB_OK);
		return FALSE;
	}

	for(i=0;i<MATH_BUFFER_SIZE;i++) {
		dbuffi[i]=0.;
		scrbuff[i] = 0.;
		scrbuffb[i] = 0.;
		filterr[i] = 0.;
		filteri[i] = 0.;
//		postmultr[i]= cos(i*pi/(2*MATH_BUFFER_SIZE));
//		postmulti[i]= sin(i*pi/(2*MATH_BUFFER_SIZE));
	}
	for(i=0;i<FFT_SIZE;i++) 
		autocorrFilt[i]=autocorr[i]=0.;

	for(i=0;i<8;i++)
		filterstate[i]=0.;

	genblack(dwin,FFT_SIZE); 
//-----------------------------------------------------------------------

//	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LPCSOUND));

	wcolor=RGB(0,255,128); // GREENBLUE



	redPen = CreatePen(PS_SOLID,1,RGB(255,50,50)); // red pen
	greenPen = CreatePen(PS_SOLID,1,RGB(0,255,128)); // green pen
	hdc = GetDC(hWnd);
	defpen = SelectObject(hdc,GetStockObject(WHITE_PEN));
	PatBlt(hdc,0,0,1200,1024,BLACKNESS);

	mm.reset(3,2000.);

	decoder.init();
	
	//SendMessage(hTrackWnd, WM_PAINT, 0, 0);
	RedrawWindow(hTrackWnd, NULL, NULL, RDW_INVALIDATE);

	while(1) {

#if 0
//		 Main message loop:
		if (PeekMessage(&msg, NULL, 0, 0,PM_REMOVE)) {
			// test if this is a quit
			if (msg.message == WM_QUIT)
				break;

			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
#endif
		if (terminate_flag != -100)
			break; // break out of while(1)

		while(notdone=(!(wa.whin[wa.ibuf].dwFlags & WHDR_DONE) || !(wa.whout[wa.ibuf].dwFlags & WHDR_DONE)))
			result = WaitForSingleObject(wa.he, 10  /*INFINITE*/  /*was 10*/);

		if (!notdone) {

			process_samples(wa.whin[wa.ibuf].lpData, wa.whout[wa.ibuf].lpData, wa.whin[wa.ibuf].dwBufferLength);

			memcpy(abuff,wa.whin[wa.ibuf].lpData,wa.whin[wa.ibuf].dwBufferLength);
			/* Queue record ibuf */
			mmres = waveInAddBuffer(wa.hwi, &wa.whin[wa.ibuf], sizeof(wa.whin[wa.ibuf]));
			if(mmres) return 2;
#ifdef PLAYBACK
			/* Play ibuf */
			mmres = waveOutWrite(wa.hwo, &wa.whout[wa.ibuf], sizeof(wa.whout[wa.ibuf]));
#endif
			if(++wa.ibuf >= WAVEAUDIO_NBUFS)
				wa.ibuf = 0;

			playbacktime += (float)wa.whin[wa.ibuf].dwBufferLength / (float)wa.wfx.nAvgBytesPerSec;
			//swprintf(pbuff,80,L"\r%c %.1fs", "|-"[wa.ibuf], playbacktime); /* simple ticker */

			//PatBlt(hdc,0,0,128,16,WHITENESS);
			//TextOut(hdc,0,0,pbuff,wcslen(pbuff));

//			PatBlt(hdc,0,0,1024,512,BLACKNESS);

			//Draw center lines
//			SelectObject(hdc,GetStockObject(WHITE_PEN));
//			MoveToEx(hdc,0,128+256,NULL);
//			LineTo(hdc,1024,128+256);

			//MoveToEx(hdc,0,512+256,NULL);
			//LineTo(hdc,1024,512+256);

			sum=0.;
			freq += dfreq;
			if (freq > 256)
				dfreq = -dfreq;
			if (freq < 8)
				dfreq = -dfreq;

			for(i=0;i<MATH_BUFFER_SIZE;i++) {
				//abuff[i<<1]= (short)(32767.*cos(i*2.*pi/64.));  // integer test signal
				
				//yval = abuff[i<<1]>>7;
				sigval = abuff[i<<1];   // Extract left channel out of stereo pair
				//sigval = 16384.*cos(i*2.*pi/freq);		// float test signal

//				sigval = biquad(sigval,BQhipass2k,&filterstate[0]);
//				sigval = biquad(sigval,BQhipass2k,&filterstate[2]);

//				sigval = sigval + delay(sigval,80); // add in a delay of 80 samples (10ms)

				dbuffr[i] = GLOBAL_inputgain * gain * sigval;			// gain adjustment

				//yval = (int)(dbuffr[i<<1]/128.);
				//SetPixel(hdc,i,(256+128)-yval,wcolor);
				//MoveToEx(hdc,i,256,NULL);
				//LineTo(hdc,i,256-yval);
			}

			// call morse decoder
			decoder.rx_process(dbuffr, MATH_BUFFER_SIZE);
			//decoder.rx_process(&dbuffr[MATH_BUFFER_SIZE >> 1], MATH_BUFFER_SIZE >> 1);

			 //Apply windowing function
			 for(i=0;i<FFT_SIZE;i++) {
//				 int j;
//				 j = i+(MATH_BUFFER_SIZE>>1);
//				 if (j >= MATH_BUFFER_SIZE) j-= MATH_BUFFER_SIZE;
			 	wbuff[i]= dbuffr[i] * dwin[i];
//				cbuffr[i] *= dwin[j];
			 }


			//DisplayWaveform(hdc,256,8,512,256,wbuff,FFT_SIZE,1.);
			
			//DisplayWaveform(hdc, 256, 1, 512, 128, wbuff, FFT_SIZE, 1.);


#if 0
			// shuffle input for DCT
			for(i=0;i<=(MATH_BUFFER_SIZE>>1)-1;i++) {
				cbuffr[i]=wbuff[i<<1]; // for DCT transform
				cbuffr[MATH_BUFFER_SIZE-1-i]=dbuffr[(i<<1)+1]; // for DCT transform
			}
#endif		


#ifdef FFT
			//Compute FFT
#if 1
			fft(wbuff,dbuffi,fbuffr,fbuffi,FFT_SIZE,1);  

			double fcoef = 0.90;// 95;
			double tcoef = 0.90;

			for (i = 0; i < FFT_SIZE >> 1; i++) { //time domain filter before magnitude calculation
				filterr[i] = fcoef*filterr[i] + (1. - fcoef)*fbuffr[i];
				filteri[i] = fcoef*filteri[i] + (1. - fcoef)*fbuffi[i];
			}



			for(i=0;i< FFT_SIZE>>1 ;i++) {
				scrbuff[i]  = tcoef*scrbuff[i]  + (1. - tcoef)* (20 * log10(1E-15+sqrt(fbuffr[i] * fbuffr[i] + fbuffi[i] * fbuffi[i]))); //time average of log of FFT Magnitude
				scrbuffb[i] = tcoef*scrbuffb[i] + (1. - tcoef)* (20 * log10(1E-15+sqrt(filterr[i] * filterr[i] + filteri[i] * filteri[i]))); //time average of log of Magnitude of time average
			}
#endif
			marktimeStart();
#if 1
			//void DisplayWaveform(HDC hdc, int x, int y, int w, int h, double *data, int dataLen, double ygain, int grid, int erase=1, int color)
			DisplayWaveform(hdc, 64, 8, FFT_SIZE , 128, scrbuff, FFT_SIZE >> 1, 500., 20);
			DisplayWaveform(hdc, 64, 8, FFT_SIZE , 128, scrbuffb, FFT_SIZE >> 1, 500., 0, 0, RGB(255, 0, 0));
#endif


#if 1
			int ypos = 128;
			//DisplayHScrolling(HDC hdc, int px, int py, int w,	int h,	double *data,	int dlen)
			DisplayVScrolling(hdc, 64, ypos + 8, FFT_SIZE, 128, scrbuff, FFT_SIZE >> 1);

			marktimeEnd();
#if 0
			 //mm.draw(hdc, 8+1024-2, 128 + 8, 256, 3200., 2, RGB(200, 100, 100));
			mm.draw(hdc, 8 + 1024 - 2, ypos + 8, 256, 2., 2, RGB(128, 128, 128));
			mm.draw(hdc, 8 + 1024 - 2, ypos + 8, 256, 2000., 0, RGB(0, 250, 250)); //cyan
			mm.draw(hdc, 8 + 1024 - 2, ypos + 8, 256, 2000., 1, RGB(250, 250, 0));
			mm.draw(hdc, 8 + 1024 - 2, ypos + 8, 256, 2000., 3, RGB(0, 255, 0));
			mm.draw(hdc, 8 + 1024 - 2, ypos + 8, 256, 2., 4, RGB(255, 0, 0));

			mm.draw(hdc, 8 + 1024 - 2, ypos + 8, 256, 500., 5, RGB(250, 250, 250));

			 mm.drawh(hdc, 8, 512, 64, 3);
#endif

#endif
			 //if (--mcount == 0) {
				// mm.reset(3, 1000.);
				// mcount = 64;
			 //}

			 //DisplayWaveform(hdc,10,128+256+16,512,128,scrbuff,FFT_SIZE>>1,100.);
#endif


#ifdef GAMMATONE
			 // Compute gammatone ratemap 

			 
			 makeRateMap( &ratemap[0][0], &numFramesExported, dbuffr,  RM_SIZE,  SAMPLE_FREQ,(double) /*lowcf*/ 30., (double) /* highcf -1.*/ 8000.,
						NUM_CHANNELS, (double)/*frameshift*/4., (double)/*ti*/ 1., /*int compression_type*/1);
			 

			 
			 for (j=0;j<numFramesExported;j++) { 
				for(i=0;i<NUM_CHANNELS;i++) {
					gbuff[i]= GTVgain*ratemap[j][i];
				}
				 DisplayHScrolling(hdc,528,2+YMAX-128-512,512,512,gbuff,NUM_CHANNELS);
			 }
			 DisplayWaveform(hdc,10,2+YMAX-128-512,512,256,ratemap[0],NUM_CHANNELS,1000.);
#endif


#if 1
			 //Autocorrelation


			 //make double buffer; calculate and remove DC

			 sum=0.;
			 for(i=0;i<AUTOCORR_SIZE<<1;i++) {
				 exbuff[i]= dbuffr[i];//wbuff[i];
				 sum += exbuff[i];
			 }
			 sum = sum / (AUTOCORR_SIZE<<1);
			 // remove DC
			 for(i=0;i<AUTOCORR_SIZE<<1;i++) {
				 exbuff[i] -= sum;
			 }


			 for(i=0;i<AUTOCORR_SIZE;i++) {
				sum=0.;
				for(j=0;j<AUTOCORR_SIZE;j++) {
					 sum += exbuff[j]*exbuff[j+i];
					}
				sum *= autocorr_scale;
				autocorr[i]= (1.-autocorrAlpha)*autocorr[i]+ autocorrAlpha * sum;
			 }

			 //time filter
			 for (i = 0; i < AUTOCORR_SIZE; i++) {
				 autocorrFilt[i] = tcoef*autocorrFilt[i] + (1. - tcoef)*autocorr[i];
			 }


			 sum = autocorrFilt[0];
//			 for(i=0;i<FFT_SIZE;i++)
//				 t[i] = autocorr[i]*sum;

// 1366 x 768
//			DisplayWaveform(hdc,0,828,1024,128,autocorr,AUTOCORR_SIZE,16384./sum,80); // 10ms grid marks
			DisplayWaveform(hdc, 0, YMAX-128, 1024, 128, autocorrFilt, AUTOCORR_SIZE, 16384. / sum, 0); // 10ms grid marks


			sum=0.;
			max = 0.;
			maxi = 0;
			int trigger=0;
			int bumptrig=0;
			int peakfind=0;
			for(i=0;i<AUTOCORR_SIZE;i++)  {
				 sum += autocorrFilt[i];
				 if (autocorrFilt[i]<0) trigger = 1;
				 if (trigger) {
					if (autocorrFilt[i] > max) {
						max = autocorrFilt[i];
						maxi = i;
						bumptrig=1;
						peakfind=1;
					} else if (peakfind) { // one place past the peak, interpolate

					}
					
					if ((autocorrFilt[i]<0)&&bumptrig) {
						max *= 1.05;
						bumptrig=0;
					}

				 }
			}

			freq = SAMPLE_FREQ/maxi;
#endif
			//sum = sum/autocorr[0];
			swprintf(pbuff,80,L"(%ld,%ld) autocorrF=% 5.2f, freq= % 5.2f", (long)(tmin*cpu_freq_factor),(long)(tmax*cpu_freq_factor),autocorrFilt[0],freq);  

			SetTextColor(hdc, RGB(0, 255, 255));
			PatBlt(hdc,800,YMAX-128,200,16,BLACKNESS);
			TextOut(hdc,800,YMAX-128,pbuff,wcslen(pbuff));

			PatBlt(hdc, 0, YMAX - 128 - 16, 1280, 16, BLACKNESS);
			swprintf(pbuff, 80, L"frq=%5.1f,wpm=%d, cw adap=%ld", decoder.frequency, decoder.cw_receive_speed, decoder.cw_adaptive_receive_threshold);
			TextOut(hdc, 0, YMAX - 128 - 16, pbuff, wcslen(pbuff));
			TextOut(hdc, 256, YMAX - 128 - 16, dummybuffer, wcslen(dummybuffer));

			prevObj=SelectObject(hdc,redPen);
			MoveToEx(hdc,maxi*4,YMAX-128,NULL);
			LineTo(hdc,maxi*4,YMAX);
			SelectObject(hdc,prevObj);
			 
#if 0
			//  DCT
			// transform
			fft(cbuffr,dbuffi,fbuffr,fbuffi,MATH_BUFFER_SIZE,1); 
			//Postmultiply
			for(i=0;i<MATH_BUFFER_SIZE>>1;i++) {
				fbuffr[i] = postmultr[i]*fbuffr[i] + postmulti[i]*fbuffi[i];
			}
			//Draw DCT
			SelectObject(hdc,redPen);
			for(i=0;i<MATH_BUFFER_SIZE;i++) {
				int x,y;
				scrbuff[i]= 1.00*(20*log( fabs( fbuffr[i]))) + .0*scrbuff[i];  //DCT 
				yval = (int)scrbuff[i];
				//yval = (int)(dwin[i]*256);
				if (yval > 256) yval = 256;
				if (yval < -256) yval = -256;
				//MoveToEx(hdc,i+MATH_BUFFER_SIZE,256+128,NULL);
				x = (i);
				y = (256+512)-yval;
				if (i==0) MoveToEx(hdc,x,y,NULL);
				LineTo(hdc,x,y);
				//SetPixel(hdc,i+MATH_BUFFER_SIZE,(256+128)-yval,wcolor);
				//MoveToEx(hdc,i,256,NULL);
				//LineTo(hdc,i,256-yval);
			}
#endif



		}
	}

	SelectObject(hdc,defpen);
	DeleteObject(redPen);
	DeleteObject(greenPen);

	ReleaseDC(hWnd,hdc);

	waveaudio_cleanup(&wa);

	return terminate_flag;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LPCSOUND));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_LPCSOUND);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	RECT crect;

	int trackpos;
	WCHAR stxt[32];

	switch (message)
	{

	case WM_HSCROLL:
		if ((HWND)lParam == hTrackWnd) {
			trackpos = SendMessage(hTrackWnd, TBM_GETPOS, 0, 0); // Assume immediate return??
			GLOBAL_inputgain = (double)(4.* trackpos / 200.);
			swprintf(stxt, L"%4.2lg", GLOBAL_inputgain);
			hdc = GetDC(hWnd);
			BitBlt(hdc, 205, YMAX-228, 32, 16, NULL, 0, 0, WHITENESS);
			TextOutW(hdc, 205, YMAX-228, stxt, wcslen(stxt));
		
		}
		else
			if ((HWND)lParam == hTrackWnd2) {
				trackpos = SendMessage(hTrackWnd2, TBM_GETPOS, 0, 0); // Assume immediate return??
				wsprintfW(stxt, L"%3d", trackpos);
				hdc = GetDC(hWnd);
				BitBlt(hdc, 505, 55, 24, 16, NULL, 0, 0, WHITENESS);
				TextOutW(hdc, 505, 55, stxt, wcslen(stxt));
			}
			else break;

			//wsprintfW(stxt, L"0x%08x", lParam);
			//TextOutW(hdc, 0, 24, stxt, wcslen(stxt));
			ReleaseDC(hWnd, hdc);
			break;



	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);

		// TODO: Add any drawing code here...
		GetClientRect(hWnd, &crect);
		BitBlt(hdc, 0, 0, crect.right,crect.bottom, 0, 0, 0, BLACKNESS);

		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
