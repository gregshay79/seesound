
#define HISTMAX 64
#define IMAX 8

void DisplayHScrolling(HDC hdc,int px,int py,int w,int h,double *data,int dlen);
void DisplayVScrolling(HDC hdc, int px, int py, int w, int h, double *data, int dlen, double dmin, double dmax);
void DisplayStripChart(HDC hdc, int px, int py, int w, int h, double *data, int dlen, double dmin, double dmax, double grid =0.0);


void DisplayWaveform(HDC hdc,int x,int y,int w,int h,double *data,int dataLen,double ymin, double ymax, int grid = 0,int erase=1,int color=0);
void DisplayPhasor(HDC hdc, int x, int y, int sz, double *data, int dataLen, double magmax=1.0, double phmax=3.1415926535);
void DisplayMeterBar(HDC hdc, int dBflag, int x, int y, int sx,int sy, double *data, int dataLen=1, double min=0.0, double max=1.0, double tickmark=0.0, int drawFromZero=0);


class minMaxMeter {

	double mmin[IMAX], mmax[IMAX];
	double hmax[IMAX];
	HPEN nPen[IMAX];// = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }; // 0, 0, 0, 0, 0, 0, 0, 0 };
	float hist[IMAX][HISTMAX];

public:
	minMaxMeter();
	~minMaxMeter();

void reset(int ix, double maxBound=0.);

void set(int ix, double value);

void draw(HDC hdc, int px, int py, int h, double valmax, int ix, COLORREF color);

void drawh(HDC hdc, int px, int py, int h, int ix);

};

extern class minMaxMeter mm;