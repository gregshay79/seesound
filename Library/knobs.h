



#define KNOBSIZEX 112
#define KNOBSIZEY 16

#ifndef MAX_KNOB
#define MAX_KNOB 16
#endif

#define BUTTONSIZEX 28
#define BUTTONSIZEY 16

#define MAX_BUTTON 16
#define MAXBUTTONVALUES 8

#define BUTTON_TYPE_MOMENTARY 0
#define BUTTON_TYPE_MULTI 1

#define	KNOB_INGAIN	0
#define KNOB_FTc		1
#define KNOB_TTc		2
#define KNOB_Cpro		3
#define KNOB_Cint		4
#define KNOB_Cdiff		5
#define KNOB_DemodTc	6
#define KNOB_Gamma		7
#define KNOB_freq		8
#define KNOB_trig_thresh 9
#define KNOB_pretrigger	10
#define KNOB_noiseamp 11
#define KNOB_sigamp 12
#define KNOB_Tscale 13

#define BUTTON_input_select 0
#define BUTTON_hold_loop	1
#define BUTTON_trigger_mode	2
#define BUTTON_trigger_arm	3

struct knob {
	HWND hwnd;
	RECT pos;
	int ival;
	double value;
	int min, max;
	double(*xform)(int v, int min, int max);
	LPWSTR(*dxform)(double val);
	TCHAR name[16];
	TCHAR dispStr[32];
};

struct button {
	HWND hwnd;
	RECT pos;
	int value;
	int type;
	int nValues; // if type==2
	TCHAR name[MAXBUTTONVALUES][16];
};

extern struct knob knobs[];
extern struct button buttons[];

extern double samplerate, block_size;

HWND createKnob(HWND hwnd, struct knob *pK, TCHAR *name, int x, int y, 
				double(*fcn)(int, int, int), LPWSTR(*dispMap)(double val), int imin, int imax, int iInit);

HWND createButton(HWND hwnd, int x, int y, struct button *pB, TCHAR **name, int type, int nValues);

void destroyKnobs();
void initKnobs();

LPWSTR dBmap(double val);
LPWSTR tcBlockSRDispMap(double val);
LPWSTR tcFullSRDispMap(double val);
LPWSTR IntegerDispMap(double val);



double linmap(int v, int min, int max);
double normMap(int v, int min, int max);
double norm4Map(int v, int min, int max);
double norm80Map(int v, int min, int max);
double milMap(int v, int min, int max);

double scaleMap(int v, int min, int max);
double passthruMap(int v, int min, int max);
double tcmapblockSR(int v, int min, int max);
double tcmapfullSR(int v, int min, int max);

