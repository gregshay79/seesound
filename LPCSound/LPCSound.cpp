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
#include "../Library/knobs.h"

#include "cw.h"
#include "configuration.h"
#include "morse.h"

#define MAX_LOADSTRING 100


#define AUDIO_BUFFER_SIZE 512
#define MATH_BUFFER_SIZE AUDIO_BUFFER_SIZE
#define RM_SIZE MATH_BUFFER_SIZE
#define FFT_SIZE MATH_BUFFER_SIZE
#define SAMPLE_FREQ 8000

#define FFT
//#define GAMMATONE

#define AUTOCORR_SIZE (MATH_BUFFER_SIZE/2)		
#define NUM_CHANNELS 512

//Display size
#define XMAX 1366
#define YMAX 768


#define MC_IDLE 0
#define MC_DIT  1
#define MC_DAH  2
#define MC_DITGAP 3
#define MC_CHARGAP 4
#define MC_WORDGAP 5


//#define PLAYBACK_SOURCE 1
#define PLAYBACK_SIGNAL 1

extern CW_TABLE cw_table[];

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
double demodbuff[MATH_BUFFER_SIZE];
double mag[MATH_BUFFER_SIZE];
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
double samplerate, block_size;

extern wchar_t decodebuffer[];

extern double coherent_decode_block(double *dbuffr, double *doutput, int dlen);
extern double spychannel[];

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

double filterstate[8];

double gain=2.0;
double GTVgain = 10.;
double max,maxi;
double tracker_freq;
double phaseMeter[8];
double meter[8];

cw cw_decoder;

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

TCHAR bnames[16][16] = { L"in", L"tst", L"run", L"hld", L"fre", L"aut", L"1sh", L"arm", L"go", L"end" };
TCHAR *bnamesp[16] = { bnames[0], bnames[1], bnames[2], bnames[3], bnames[4], bnames[5], bnames[6],
bnames[7], bnames[8], bnames[9] };

TCHAR insignames[8][8] = { L"inp", L"sine", L"imp", L"pls", L"mod",L"mse" };
TCHAR *insignamesp[8] = { insignames[0], insignames[1], insignames[2], insignames[3], insignames[4], insignames[5] };


DWORD WINAPI doWindowsStuff(LPVOID param)
{
	MSG msg;
	int nCmdShow = *(int*)param;
	HACCEL hAccelTable;
	int ypos;

	// Initialize global strings
	LoadString(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInst, IDC_LPCSOUND, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInst);

	hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0 /*CW_USEDEFAULT*/, 0, 1320 /*CW_USEDEFAULT*/, 1064, NULL, NULL, hInst, NULL);

	if (!hWnd) {
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDC_LPCSOUND));

#ifdef USETRACKBAR
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
//	UpdateWindow(hTrackWnd);
#endif



	// make a gain knob
	createKnob(hWnd, &knobs[KNOB_INGAIN], L"Inp Gain:", 2, 0, norm4Map, dBmap, 0, 200, 50);

	// Make a Push-on/push-off signal select button


	createButton(hWnd, 2, 18, &buttons[BUTTON_input_select], insignamesp, BUTTON_TYPE_MULTI, 6);
	// Make fcoeff knob
	createKnob(hWnd, &knobs[KNOB_FTc], L"F Tc:", 2, 36, tcmapblockSR, tcBlockSRDispMap, 0, 400, 25);
	createKnob(hWnd, &knobs[KNOB_TTc], L"T Tc:", 2, 64, tcmapblockSR, tcBlockSRDispMap, 0, 400, 25);
	createKnob(hWnd, &knobs[KNOB_freq], L"F delta:", 2, 80, norm4Map, NULL, 0, 400, 25);
	createKnob(hWnd, &knobs[KNOB_noiseamp], L"noise:", 2, 80+16, normMap, NULL, 0, 400, 0);
	createKnob(hWnd, &knobs[KNOB_sigamp], L"sigamp:", 2, 80+32, normMap, NULL, 0, 400, 200);


// Controls for triggered waveform display
	createButton(hWnd, 1160, 16, &buttons[BUTTON_trigger_mode], &bnamesp[4],BUTTON_TYPE_MULTI,3);
	createButton(hWnd, 1160, 48, &buttons[BUTTON_trigger_arm], &bnamesp[7], BUTTON_TYPE_MULTI,2);
	createKnob(hWnd, &knobs[KNOB_trig_thresh], L"Thresh", 1160, 80, normMap, NULL, 0, 400, 200);
	createKnob(hWnd, &knobs[KNOB_pretrigger], L"Pretrig", 1160, 112, passthruMap, IntegerDispMap, 0, 400, 100);
	createKnob(hWnd, &knobs[KNOB_Tscale], L"Tscale", 1160, 144, passthruMap, IntegerDispMap, 1, 16, 4);

	// Make knobs for coherent decoder
	ypos = 264;
	createKnob(hWnd, &knobs[KNOB_Cpro],  L"Cpro :", 2, ypos += 24, norm80Map, NULL, 0, 800, 300);
	createKnob(hWnd, &knobs[KNOB_Cint],  L"Cint :", 2, ypos += 24, milMap, NULL, 0, 400, 200);
	createKnob(hWnd, &knobs[KNOB_Cdiff], L"Cdiff:", 2, ypos += 24, norm4Map, NULL, 0, 400, 0);
	createKnob(hWnd, &knobs[KNOB_Gamma], L"gamma:", 2, ypos += 24, normMap, NULL, 0, 400, 10);
	createKnob(hWnd, &knobs[KNOB_DemodTc], L"dmTc :", 2, ypos += 24, tcmapfullSR, tcFullSRDispMap, 0, 400, 150);
	
//	createKnob(hWnd, &knobs[KNOB_freeze], L"Fz :", 2, ypos += 24, normMap, NULL, 0,100, 100);
	createButton(hWnd, 2, ypos += 24, &buttons[BUTTON_hold_loop], &bnamesp[2], BUTTON_TYPE_MOMENTARY, 2);





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
	double testfreq=500.,dfreq=-1.;
	double sigval;
	HPEN redPen,greenPen;
	HGDIOBJ  defpen;
	HGDIOBJ prevObj;
	int mcount = 10;
	DWORD threadID = 0;
	int xpos=0,ypos = 16;
	double theta = 0.;
	double scaleFactor = 1. / 32768.;
	double wincomp;
	double dval;
	TCHAR strval[32];
	double indicatorFreq;
	int sampcount = 0;

	 int msamplen = 0;
	 int wordlen = 1;
	 int mc_state = MC_CHARGAP;
	 const char *currMchar;
	int currMcharlen;
	int ditlen;
	int z;

	double sigamp, noise;

	int encodepos = 0;
	wchar_t encodebuffer[128];


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
	samplerate = SAMPLE_FREQ;
	block_size = AUDIO_BUFFER_SIZE;
	// Perform application initialization:
	hInst = hInstance; // Store instance handle in our global variable
	ditlen = (samplerate) / 18 /*wpm*/;
	encodebuffer[0] = 0;

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
	wincomp = 2.5;
//-----------------------------------------------------------------------

//	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LPCSOUND));

	wcolor=RGB(0,255,128); // GREENBLUE

	redPen = CreatePen(PS_SOLID,1,RGB(255,50,50)); // red pen
	greenPen = CreatePen(PS_SOLID,1,RGB(0,255,128)); // green pen
	hdc = GetDC(hWnd);
	defpen = SelectObject(hdc,GetStockObject(WHITE_PEN));
	PatBlt(hdc,0,0,1200,1024,BLACKNESS);

	mm.reset(3,2000.);

	cw_decoder.init();
	
	//SendMessage(hTrackWnd, WM_PAINT, 0, 0);
	RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);

	while(1) {
		ypos = 16;

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

			memcpy(abuff, wa.whin[wa.ibuf].lpData, wa.whin[wa.ibuf].dwBufferLength);

			/* Queue record ibuf */
			mmres = waveInAddBuffer(wa.hwi, &wa.whin[wa.ibuf], sizeof(wa.whin[wa.ibuf]));
			if (mmres) return 2;

#ifdef PLAYBACK_INPUT
			/* Play ibuf */
			mmres = waveOutWrite(wa.hwo, &wa.whout[wa.ibuf], sizeof(wa.whout[wa.ibuf]));
#endif

			//if(++wa.ibuf >= WAVEAUDIO_NBUFS)
			//	wa.ibuf = 0;

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

			sum = 0.;
			sigamp = .25*knobs[KNOB_sigamp].value;
			//freq += dfreq;
			//if (freq > 256)
			//	dfreq = -dfreq;
			//if (freq < 8)
			//	dfreq = -dfreq;

			for(i=0; i<MATH_BUFFER_SIZE; i++) {
				//abuff[i<<1]= (short)(32767.*cos(i*2.*pi/64.));  // integer test signal
				
				//yval = abuff[i<<1]>>7;

				//sigval = abuff[i<<1];   // Extract left channel out of stereo pair
				theta += (testfreq + knobs[KNOB_freq].value) * 2 * pi / samplerate;
				if (theta>2 * pi) theta -= 2 * pi;

				noise = knobs[KNOB_noiseamp].value * rand();

				if (++sampcount == (1 << 16)) sampcount = 0;

				switch (buttons[BUTTON_input_select].value) {
				case 0:
					sigval = abuff[i << 1];   // Extract left channel out of stereo pair
					break;
				case 1:
					// sine test signal
					sigval = noise+sigamp*32767.*cos(theta);
					break;
				case 2:// impulse train
					sigval = noise+sigamp*32767.* ((sampcount & 1023 ) == 0 ? 1.0 : 0);
					break;
				case 3: // pulse train 50ms wide pulses at 200ms rate
					sigval = noise + sigamp*32767.* ((sampcount & (1023 - 255)) == 0 ? 1.0 : 0);
					break;
				case 4: // modulated pulse train
					sigval = 32767.* ((sampcount & (1023 - 255)) == 0 ? 1.0 : 0);
					sigval = noise + sigamp*sigval*cos(theta);
					break;

				case 5: // Random morse code at 18wpm
					msamplen++;
					sigval = noise;
					switch (mc_state) { // morse character state
					case MC_DIT:
						if (msamplen > ditlen) {
							msamplen = 0;
							mc_state = MC_DITGAP;
						}
						else
							sigval += sigamp*32767.*cos(theta);
						break;
					case(MC_DAH) :
						if (msamplen > 3*ditlen) {
							msamplen = 0;
							mc_state = MC_DITGAP;
						}
						else
							sigval += sigamp*32767.*cos(theta);
						break;
					case(MC_DITGAP) :
						if (msamplen > ditlen) {
							msamplen = 0;
							if (--currMcharlen == 0) { // end of character
								mc_state = MC_CHARGAP;
							}
							else {
								currMchar++;
								mc_state = (*currMchar == '.') ? MC_DIT : MC_DAH;
							}
							break;
					case(MC_CHARGAP) :
						if (msamplen > 2 * ditlen) { // additional gap time above DITGAP
							msamplen = 0;

							while ( (z = (rand()&63)) > 39);
							currMchar = cw_table[9 + z].rpr;
							currMcharlen = strlen(currMchar);
							encodebuffer[encodepos++] = (wchar_t)cw_table[9 + z].chr;
							if (encodepos > 80) encodepos = 0;
							encodebuffer[encodepos] = 0;
							

							if (--wordlen == 0) {
								mc_state = MC_WORDGAP;
								wordlen = rand() & 7;
							}
							else {
								mc_state = (*currMchar == '.') ? MC_DIT : MC_DAH;
							}
						}
									 break;
					case(MC_WORDGAP) :
						if (msamplen > 4 * ditlen) { // additional gap time above CHARGAP
							msamplen = 0;
							// Note: currMchar and currMcharlen already set at end of prior CHARGAP
							mc_state = (*currMchar == '.') ? MC_DIT : MC_DAH;
						}
									 break;
					default:
						break;
						} // end of morse case
					}
					break;
				default:
					break;
				};

//				sigval = biquad(sigval,BQhipass2k,&filterstate[0]);
//				sigval = biquad(sigval,BQhipass2k,&filterstate[2]);

//				sigval = sigval + delay(sigval,80); // add in a delay of 80 samples (10ms)

				dbuffr[i] = knobs[KNOB_INGAIN].value * scaleFactor * sigval;			// gain adjustment

				//yval = (int)(dbuffr[i<<1]/128.);
				//SetPixel(hdc,i,(256+128)-yval,wcolor);
				//MoveToEx(hdc,i,256,NULL);
				//LineTo(hdc,i,256-yval);
			}


#ifdef PLAYBACK_SIGNAL
			/* Play signal buffer */
			// rewrite wa.whout. Use index (1 - wa.ibuf) because buffers already flipped, above
			// rewrite abuff;
			for (i = 0; i < MATH_BUFFER_SIZE; i++) {
//				abuff[i << 1] = 32768.*dbuffr[i];   // write left channel out of stereo pair
				abuff[i << 1] = 8192.*spychannel[i];   // write left channel out of stereo pair

				abuff[(i << 1) + 1] = abuff[i << 1];   //copy to right channel of stereo pair
			}
			memcpy( wa.whout[wa.ibuf].lpData, abuff, wa.whin[wa.ibuf].dwBufferLength);
			mmres = waveOutWrite(wa.hwo, &wa.whout[wa.ibuf], sizeof(wa.whout[wa.ibuf]));
#endif


			if (++wa.ibuf >= WAVEAUDIO_NBUFS)
				wa.ibuf = 0;

			// call morse decoder
			// cw_decoder.rx_process(dbuffr, MATH_BUFFER_SIZE);


			 //Apply windowing function
			 for(i=0;i<FFT_SIZE;i++) {
//				 int j;
//				 j = i+(MATH_BUFFER_SIZE>>1);
//				 if (j >= MATH_BUFFER_SIZE) j-= MATH_BUFFER_SIZE;
			 	wbuff[i]= wincomp * dbuffr[i] * dwin[i];
//				cbuffr[i] *= dwin[j];
			 }

			//void DisplayTriggeredWaveform(HDC hdc, int x, int y, int w, int h, double *data, int dataLen, double ymin, double ymax, int grid, int erase, int color,
			//struct button *triggerModeButton, struct knob *threshKnob, struct button *armButton, int timecompressfactor )
			DisplayTriggeredWaveform(hdc,128+8+512,16,512,256,demodbuff /*spychannel*/ /*dbuffr*/ ,FFT_SIZE,-1.0,1.0,0,1,0,
				&buttons[BUTTON_trigger_mode], &knobs[KNOB_trig_thresh], &buttons[BUTTON_trigger_arm], knobs[KNOB_Tscale].value);
			
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

			double fcoef = knobs[KNOB_FTc].value;// 95;
			double tcoef = knobs[KNOB_TTc].value;

			for (i = 0; i < FFT_SIZE >> 1; i++) { //time domain filter before magnitude calculation
				filterr[i] = fcoef*filterr[i] + (1. - fcoef)*fbuffr[i];
				filteri[i] = fcoef*filteri[i] + (1. - fcoef)*fbuffi[i];
			}



			for(i=0;i< FFT_SIZE>>1 ;i++) {
				scrbuff[i]  = tcoef*scrbuff[i]  + (1. - tcoef)* (20 * log10(1E-15+sqrt(fbuffr[i] * fbuffr[i] + fbuffi[i] * fbuffi[i]))); //time average of log of FFT Magnitude
				scrbuffb[i] = tcoef*scrbuffb[i] + (1. - tcoef)* (20 * log10(1E-15+sqrt(filterr[i] * filterr[i] + filteri[i] * filteri[i]))); //time average of log of Magnitude of time average
//				mag[i] = sqrt(fbuffr[i] * fbuffr[i] + fbuffi[i] * fbuffi[i]);
			}
#endif
			marktimeStart();
#if 1
//			ypos = 16;
			xpos = 128;
			//void DisplayWaveform(HDC hdc, int x, int y, int w, int h, double *data, int dataLen, double ygain, int grid, int erase=1, int color)
			DisplayWaveform(hdc, xpos, ypos, FFT_SIZE, 128, scrbuff, FFT_SIZE >> 1, -100, 0., 10);
//			DisplayWaveform(hdc, xpos, ypos, FFT_SIZE, 128, mag, FFT_SIZE >> 1, -1, 1.0, .1);
			DisplayWaveform(hdc, xpos, ypos, FFT_SIZE, 128, scrbuffb, FFT_SIZE >> 1, -100, 0., 0, 0, RGB(255, 0, 0));

			// Draw markers at 1000Hz +/- 10hz
			int x;
			prevObj = SelectObject(hdc, redPen);
			MoveToEx(hdc, x = (int)(xpos + FFT_SIZE*(500 - 10) / (SAMPLE_FREQ / 2)), ypos, NULL);
			LineTo(hdc, x, ypos+128);

			MoveToEx(hdc, x = (int)(xpos + FFT_SIZE*(500 + 10) / (SAMPLE_FREQ / 2)), ypos, NULL);
			LineTo(hdc, x, ypos+128);
			SelectObject(hdc, prevObj);
			ypos += 128;
#endif

#if 1
//			int ypos = 128+16;
			//DisplayHScrolling(HDC hdc, int px, int py, int w,	int h,	double *data,	int dlen)
			DisplayVScrolling(hdc, xpos, ypos, FFT_SIZE, 128, scrbuff, FFT_SIZE >> 1,-80,0);
			ypos += 128;
			ypos += 8; // gap

			marktimeEnd();
#if 0
			 //mm.draw(hdc, 8+1024-2, 128 + 8, 256, 3200., 2cr, RGB(200, 100, 100));
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
#else
			ypos += 256 + 8;
			xpos = 128;
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

			 // Coherent decode
			 // Data in dbuffr
			 // build display at ypos
			 dval = coherent_decode_block(dbuffr, demodbuff, MATH_BUFFER_SIZE);
//			 DisplayStripChart(hdc, xpos, ypos, 896, 128, &dval, 1, -20, 20, 5);
			 DisplayStripChart(hdc, xpos, ypos, 896, 128, &dval, 1, -20, 20,5);

			 for(i=0;i<MATH_BUFFER_SIZE;i+=DECIMATE_RATIO)
				cw_decoder.decode_stream_and_update_smpcntr(demodbuff[i],DECIMATE_RATIO);


			 swprintf_s(strval, L"Ftrack %6.2lf", tracker_freq);
			 BitBlt(hdc, 600, ypos, 64, 16, 0, 0, 0, WHITENESS);
			 TextOut(hdc, 600, ypos, strval, lstrlenW(strval));

			 DisplayPhasor(hdc, xpos + 900, ypos, 128, &phaseMeter[0], 3);

			 DisplayMeterBar(hdc, 2, xpos + 1032, ypos, 0, 128, &meter[0], 1,-47, 3, 0.0);
			 DisplayMeterBar(hdc, 2, xpos + 1032+10, ypos, 0, 128, &meter[1], 1, -47, 3, 0.0);

			 // Meter[] values have energies, meter[1] is total post AGC, meter[2] is demodulated
			 double snr = 0.707*sqrt(meter[2]) / (sqrt(meter[1]) - 0.707*sqrt(meter[2]));
			 snr = fabs(snr);
			 DisplayMeterBar(hdc, 2, xpos + 1032 + 20, ypos, 0, 128, &snr, 1, -20, 20, 0.0,1);

			 ypos += 128;
			 ypos += 8;

			// draw histogram of meter #6, in phase component of synchronous decoder.  
			 mm.drawh(hdc, xpos + 964, ypos, 64, 6);

			// draw histogram of meter #7, content of asynchronous demodulator inside morse decoder, which is after the narrow demodulator filter
			 mm.drawh(hdc, xpos + 964, ypos + 64, 64, 7);

			// Leak down histogram 7
			// Set max bin of histogram 7 to value of 3.
			 double Tc2 = exp(-1 / (1.*samplerate / block_size));
			 mm.reset(7, 3., Tc2);  // 1s time constant

#if 0
			 //Autocorrelation

			 //make double buffer; calculate and remove DC
			 static double acdc = 0.;
			double tau = 0.200; // 200ms second time constant
			 double acdcCoeff = exp(-1.0 / (tau*samplerate));
			 sum=0.;

			 for(i=0;i<MATH_BUFFER_SIZE;i++) {
				 exbuff[i]= dbuffr[i];//wbuff[i];
				 acdc = acdc*acdcCoeff+(1-acdcCoeff)*exbuff[i];
				 exbuff[i] -= acdc;
			 }
//			 sum = sum / (AUTOCORR_SIZE<<1);
//			 // remove DC
//			 for(i=0;i<AUTOCORR_SIZE<<1;i++) {
//				 exbuff[i] -= sum;
//			 }


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

// 1366 x 768 screen
//			 ypos = YMAX - 200;
			 ypos += 64;
//			DisplayWaveform(hdc,0,828,1024,128,autocorr,AUTOCORR_SIZE,16384./sum,80); // 10ms grid marks
			DisplayWaveform(hdc, 0, ypos, 1024, 128, autocorrFilt, AUTOCORR_SIZE, -sum, sum, 0); // 10ms grid marks

			sum = 0.;
			max = 0.;
			maxi = 0;
			int trigger = 0;
			int bumptrig = 0;
			int peakfind = 0;
			for (i = 0; i < AUTOCORR_SIZE; i++)  {
				sum += autocorrFilt[i];
				if (autocorrFilt[i] < 0) trigger = 1;
				if (trigger) {
					if (autocorrFilt[i] > max) {
						max = autocorrFilt[i];
						maxi = i;
						bumptrig = 1;
						peakfind = 1;
					}
					else if (peakfind) { // one place past the peak, interpolate

					}

					if ((autocorrFilt[i] < 0) && bumptrig) {
						max *= 1.05;
						bumptrig = 0;
					}
				}
			}

			indicatorFreq = SAMPLE_FREQ/maxi;

			//sum = sum/autocorr[0];
			swprintf(pbuff, 80, L"(%ld,%ld) autocorrF=% 5.2f, freq= % 5.2f", (long)(tmin*cpu_freq_factor), (long)(tmax*cpu_freq_factor), autocorrFilt[0], indicatorFreq);
#else
			ypos += 64;
#endif
			PatBlt(hdc, 0, ypos - 48, 800, 48, BLACKNESS);

			SetTextColor(hdc, RGB(255, 100, 50));
			TextOut(hdc, 0, ypos - 48, encodebuffer, wcslen(encodebuffer));

			SetTextColor(hdc, RGB(0, 255, 255));
//			PatBlt(hdc,800,ypos-16,200,16,BLACKNESS);
//			TextOut(hdc,750,ypos-16,pbuff,wcslen(pbuff));

			swprintf(pbuff, 80, L"gen SNR=%5.1f, frq=%5.1f,wpm=%d, cw adap=%ld",
			20 * log10(knobs[KNOB_sigamp].value/(knobs[KNOB_noiseamp].value+1E-12)),
			cw_decoder.frequency, cw_decoder.cw_receive_speed, cw_decoder.cw_adaptive_receive_threshold
			);
			TextOut(hdc, 0, ypos - 16, pbuff, wcslen(pbuff));

			TextOut(hdc, 0, ypos - 32, decodebuffer, wcslen(decodebuffer));

			prevObj=SelectObject(hdc,redPen);
			MoveToEx(hdc,maxi*4,ypos,NULL);
			LineTo(hdc,maxi*4,ypos+128);
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
