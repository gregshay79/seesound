



#define KNOBSIZEX 64
#define KNOBSIZEY 16

#ifndef MAX_KNOB
#define MAX_KNOB 10
#endif

struct knob {
	HWND hwnd;
	RECT pos;
	TCHAR name[8];
	int ival;
	double value;
	int min, max;
	double(*xform)(int v, int min, int max);
};

extern struct knob knobs[];

HWND createKnob(HWND hwnd, struct knob *pK, TCHAR *name, int x, int y, double(*fcn)(int, int, int), int initIval, int min, int max);

void destroyKnobs();
void initKnobs();

double linmap(int v, int min, int max);

double tcmap(int v, int min, int max);

