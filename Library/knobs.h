



#define KNOBSIZEX 112
#define KNOBSIZEY 16

#ifndef MAX_KNOB
#define MAX_KNOB 16
#endif

#define BUTTONSIZE 16
#define MAX_BUTTON 16
#define MAXBUTTONVALUES 4

#define	KNOB_INGAIN	0
#define KNOB_FTc		1
#define KNOB_TTc		2
#define KNOB_Cpro		3
#define KNOB_Cint		4
#define KNOB_Cdiff		5
#define KNOB_DemodTc	6
#define KNOB_Gamma		7
#define KNOB_freq		8
#define KNOB_freeze		9

#define BUTTON_input_select 0

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


double linmap(int v, int min, int max);
double normMap(int v, int min, int max);
double norm4Map(int v, int min, int max);
double norm40Map(int v, int min, int max);
double milMap(int v, int min, int max);


double passthuMap(int v, int min, int max);
double tcmapblockSR(int v, int min, int max);
double tcmapfullSR(int v, int min, int max);

