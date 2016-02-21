



#define KNOBSIZEX 112
#define KNOBSIZEY 16

#ifndef MAX_KNOB
#define MAX_KNOB 10
#endif

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

extern struct knob knobs[];
extern double samplerate, block_size;

HWND createKnob(HWND hwnd, struct knob *pK, TCHAR *name, int x, int y, 
				double(*fcn)(int, int, int), LPWSTR(*dispMap)(double val), int imin, int imax, int iInit);

void destroyKnobs();
void initKnobs();

LPWSTR dBmap(double val);
LPWSTR tcDispMap(double val);

double linmap(int v, int min, int max);
double normMap(int v, int min, int max);
double norm4Map(int v, int min, int max);

double passthuMap(int v, int min, int max);
double tcmap(int v, int min, int max);

