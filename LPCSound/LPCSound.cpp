// SeeSound.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "LPCSound.h"
//#include <stdio.h>
//#include <mmsystem.h>
#include <math.h>
#include <WinBase.h>
#include "../library/wwaveloop.h"
#include "../library/DisplayFunctions.h"

#include "cw.h"
#include "configuration.h"

#define MAX_LOADSTRING 100


#define AUDIO_BUFFER_SIZE 256
#define MATH_BUFFER_SIZE AUDIO_BUFFER_SIZE
#define RM_SIZE MATH_BUFFER_SIZE
#define FFT_SIZE MATH_BUFFER_SIZE
#define SAMPLE_FREQ 16000

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
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
short abuff[AUDIO_BUFFER_SIZE*2];
double dbuffr[MATH_BUFFER_SIZE],dbuffi[MATH_BUFFER_SIZE],fbuffr[MATH_BUFFER_SIZE],fbuffi[MATH_BUFFER_SIZE];
double cbuffr[MATH_BUFFER_SIZE],cbuffi[MATH_BUFFER_SIZE];
double wbuff[MATH_BUFFER_SIZE];
double exbuff[MATH_BUFFER_SIZE*2];
double autocorr[MATH_BUFFER_SIZE],autocorrd[MATH_BUFFER_SIZE];;

double dwin[MATH_BUFFER_SIZE],postmultr[MATH_BUFFER_SIZE],postmulti[MATH_BUFFER_SIZE];
double scrbuff[MATH_BUFFER_SIZE];
double gbuff[MATH_BUFFER_SIZE];

double ratemap[64][NUM_CHANNELS];
double pi = 3.1415926535897932384643383;
int numFramesExported=0;

double autocorr_scale = 1./(AUTOCORR_SIZE*32768.0);
double autocorrAlpha=.5;

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
	MSG msg;
	HACCEL hAccelTable;
	HWND hWnd;
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

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_LPCSOUND, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	QueryPerformanceFrequency((LARGE_INTEGER*)&tstamp1);
	cpu_freq_factor = 1.0e6/((double)tstamp1);
	// Perform application initialization:
	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		0 /*CW_USEDEFAULT*/, 0, 1200 /*CW_USEDEFAULT*/,1064, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return FALSE;
	}
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

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
//		postmultr[i]= cos(i*pi/(2*MATH_BUFFER_SIZE));
//		postmulti[i]= sin(i*pi/(2*MATH_BUFFER_SIZE));
	}
	for(i=0;i<FFT_SIZE;i++) 
		autocorr[i]=0.;

	for(i=0;i<8;i++)
		filterstate[i]=0.;

	genblack(dwin,FFT_SIZE); 
//-----------------------------------------------------------------------

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LPCSOUND));

	wcolor=RGB(0,255,128); // GREENBLUE



	redPen = CreatePen(PS_SOLID,1,RGB(255,50,50)); // red pen
	greenPen = CreatePen(PS_SOLID,1,RGB(0,255,128)); // green pen
	hdc = GetDC(hWnd);
	defpen = SelectObject(hdc,GetStockObject(WHITE_PEN));
	PatBlt(hdc,0,0,1200,1024,BLACKNESS);

	mm.reset(3,250.);

	decoder.init();
	
	while(1) {
		// Main message loop:
		if (PeekMessage(&msg, NULL, 0, 0,PM_REMOVE)) {
			// test if this is a quit
			if (msg.message == WM_QUIT)
				break;

			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		while(notdone=(!(wa.whin[wa.ibuf].dwFlags & WHDR_DONE) || !(wa.whout[wa.ibuf].dwFlags & WHDR_DONE)))
			result = WaitForSingleObject(wa.he, 10 /*INFINITE*/);

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
				sigval = abuff[i<<1];
				//sigval = 16384.*cos(i*2.*pi/freq);		// float test signal

//				sigval = biquad(sigval,BQhipass2k,&filterstate[0]);
//				sigval = biquad(sigval,BQhipass2k,&filterstate[2]);

//				sigval = sigval + delay(sigval,80); // add in a delay of 80 samples (10ms)

				dbuffr[i] = gain * sigval;					// apply windowing function for fft

				//yval = (int)(dbuffr[i<<1]/128.);
				//SetPixel(hdc,i,(256+128)-yval,wcolor);
				//MoveToEx(hdc,i,256,NULL);
				//LineTo(hdc,i,256-yval);
			}

			// call decoder
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
			
			// DisplayWaveform(hdc, 256, 1, 512, 128, wbuff, FFT_SIZE, 1.);


#if 0
			// shuffle DCT input
			for(i=0;i<=(MATH_BUFFER_SIZE>>1)-1;i++) {
				cbuffr[i]=wbuff[i<<1]; // for DCT transform
				cbuffr[MATH_BUFFER_SIZE-1-i]=dbuffr[(i<<1)+1]; // for DCT transform
			}
#endif		


#ifdef FFT
			//Compute FFT
#if 0
			fft(wbuff,dbuffi,fbuffr,fbuffi,FFT_SIZE,1);  
			for(i=0;i< FFT_SIZE>>1 ;i++) {
				scrbuff[i]= .7* (10*log(/*sqrt*/(fbuffr[i]*fbuffr[i]+fbuffi[i]*fbuffi[i]))) + .3*scrbuff[i]; //FFT Magnitude
			}
#endif
			marktimeStart();

			 DisplayScrolling(hdc,8,128+8,1024,256,scrbuff,FFT_SIZE>>1);
			 			 marktimeEnd();

			 //mm.draw(hdc, 8+1024-2, 128 + 8, 256, 3200., 2, RGB(200, 100, 100));
			 mm.draw(hdc, 8 + 1024 - 2, 128 + 8, 256, 2., 2, RGB(128, 128, 128));
			 mm.draw(hdc, 8 + 1024 - 2, 128 + 8, 256, 150., 0, RGB(0, 250, 250));
			 mm.draw(hdc, 8 + 1024 - 2, 128 + 8, 256, 150., 1, RGB(250, 250, 0));
			 mm.draw(hdc, 8 + 1024 - 2, 128 + 8, 256, 150., 3, RGB(0, 255, 0));
			 mm.draw(hdc, 8 + 1024 - 2, 128 + 8, 256, 2., 4, RGB(255, 0, 0));

			 mm.draw(hdc, 8 + 1024 - 2, 128 + 8, 256, 500., 5, RGB(250, 250, 250));

			 mm.drawh(hdc, 8, 512, 64, 3);

			 if (--mcount == 0) {
				 mm.reset(3, 240.);
				 mcount = 64;
			 }


			


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
				 DisplayScrolling(hdc,528,2+YMAX-128-512,512,512,gbuff,NUM_CHANNELS);
			 }
			 DisplayWaveform(hdc,10,2+YMAX-128-512,512,256,ratemap[0],NUM_CHANNELS,1000.);
#endif


#if 0
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

			 sum = autocorr[0];
//			 for(i=0;i<FFT_SIZE;i++)
//				 autocorrd[i] = autocorr[i]*sum;

// 1366 x 768
//			DisplayWaveform(hdc,0,828,1024,128,autocorr,AUTOCORR_SIZE,16384./sum,80); // 10ms grid marks
			DisplayWaveform(hdc, 0, YMAX-128, 1024, 128, autocorr, AUTOCORR_SIZE, 16384. / sum, 80); // 10ms grid marks


			sum=0.;
			max = 0.;
			maxi = 0;
			int trigger=0;
			int bumptrig=0;
			int peakfind=0;
			for(i=0;i<AUTOCORR_SIZE;i++)  {
				 sum += autocorr[i];
				 if (autocorr[i]<0) trigger = 1;
				 if (trigger) {
					if (autocorr[i] > max) {
						max = autocorr[i];
						maxi = i;
						bumptrig=1;
						peakfind=1;
					} else if (peakfind) { // one place past the peak, interpolate

					}
					
					if ((autocorr[i]<0)&&bumptrig) {
						max *= 1.05;
						bumptrig=0;
					}

				 }
			}

			freq = SAMPLE_FREQ/maxi;
#endif
			//sum = sum/autocorr[0];
			swprintf(pbuff,80,L"(%ld,%ld) autocorr=% 5.2f, freq= % 5.2f", (long)(tmin*cpu_freq_factor),(long)(tmax*cpu_freq_factor),autocorr[0],freq);  

			PatBlt(hdc,800,YMAX-128,200,16,WHITENESS);
			TextOut(hdc,800,YMAX-128,pbuff,wcslen(pbuff));

			PatBlt(hdc, 0, YMAX - 128 - 16, 1280, 16, WHITENESS);
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
return (int) msg.wParam;
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

	switch (message)
	{
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
