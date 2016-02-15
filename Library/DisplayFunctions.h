
#define HISTMAX 64
#define IMAX 8

void DisplayHScrolling(HDC hdc,int px,int py,int w,int h,double *data,int dlen);
void DisplayVScrolling(HDC hdc, int px, int py, int w, int h, double *data, int dlen);


void DisplayWaveform(HDC hdc,int x,int y,int w,int h,double *data,int dataLen,double ygain,int grid = 0,int erase=1,int color=0);


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