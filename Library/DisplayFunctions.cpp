#include "stdafx.h"
//#include "SeeSound.h"
#include <math.h>
#include "DisplayFunctions.h"

#define BOUND(Y,L,H) (Y=Y<L?L:(Y>H)?H:Y)

class minMaxMeter mm;

void DisplayWaveform(HDC hdc,int x,int y,int w,int h,double *data,int dataLen,double ygain,int grid)
{
	HPEN greenPen;
	HGDIOBJ prevPen;
	int i,yval,yoff = h/2;
	double yscale = ygain*h/2./32768.;
	double xscale = ((double)(w))/dataLen;

//	greenPen = CreatePen(PS_SOLID,1,RGB(0,255,128)); // green pen

	// Erase previous
	PatBlt(hdc,x,y,w,h,BLACKNESS);


	//Draw bounding box and center line 
	prevPen= SelectObject(hdc,GetStockObject(WHITE_PEN));
	MoveToEx(hdc,x,y,NULL);
	LineTo(hdc,x+w,y);
	LineTo(hdc,x+w,y+h);
	LineTo(hdc,x,y+h);
	LineTo(hdc,x,y);
	MoveToEx(hdc,x,y+yoff,NULL);
	LineTo(hdc,x+w,y+yoff);

#if 1
	if (grid > 0) {
		// Draw graticule
		greenPen = CreatePen(PS_DOT,1,RGB(80,0,0)); // dotted light greyblue pen
		prevPen=SelectObject(hdc,greenPen);
		//Draw vertical grid lines
		for (i=grid;i<w/xscale;i+=grid) {
			MoveToEx(hdc,i*xscale,y+1,NULL);
			LineTo(hdc,i*xscale,y+h-1);
		}
		////Draw horizontal grid lines
		//for (i=grid;i<h;i+=grid) {
		//	int ypos;
		//	ypos = y+yoff+i*yscale;
		//	if (ypos>y+h)continue;
		//	MoveToEx(hdc,x+1,ypos,NULL);
		//	LineTo(hdc,x+w-1,ypos);
		//	ypos = y+yoff-i*yscale;
		//	MoveToEx(hdc,x+1,ypos,NULL);
		//	LineTo(hdc,x+w-1,ypos);
		//}

		SelectObject(hdc,prevPen);
		DeleteObject(greenPen);
	}
#endif

	// Display waveform
	greenPen = CreatePen(PS_SOLID,1,RGB(0,255,128)); // green pen
	prevPen=SelectObject(hdc,greenPen);
	yval=yoff-((int)(data[0]*yscale));
//	if (yval > h) yval=h;
//	if (yval < 0) yval=0;
	BOUND(yval,0,h);
	MoveToEx(hdc,x,y+yval,NULL);
	for(i=0;i<dataLen;i++) {
		yval = (int)(data[i]*yscale);
		yval = yoff-yval;
		BOUND(yval,0,h);
//		if (yval > h) yval=h;
//		if (yval < 0) yval=0;
		//SetPixel(hdc,i,(256+128)-yval,wcolor);
		LineTo(hdc,(int)(x+i*xscale),y+yval);
	}

	SelectObject(hdc,prevPen);
	DeleteObject(greenPen);
}

double gamma = 1.5;
int colorTableInitFlag=0;
void DisplayScrolling(HDC hdc,int px,int py,int w,int h,double *data,int dlen)
{
	int i,yval;
	HGDIOBJ prevPen;
	double yscale;
	static int colortable[256];

	if (!colorTableInitFlag){
		for(i=0;i<256;i++) {
			double v,v3;
			int r,g,b;
			v = i/255.;  // v ranges from 0.0 to 1.0
			v3= v*v;//*v;   // v^3
			v3 = (exp(gamma*v) - 1)/(exp(gamma)-1); // normalize to 1
			r = (int)((v*255+v3*255)/2.);	// r = 1/2v^3 + 1/2v
			g = (int)(v3*255.);				// g = v^3
			b=0;
			colortable[i]=RGB(r,g,b);
		}
		colorTableInitFlag++;
	}

	yscale = ((double)h)/dlen;
	
	//Draw bounding box
	prevPen=SelectObject(hdc,GetStockObject(WHITE_PEN));
	MoveToEx(hdc,px,py,NULL);
	LineTo(hdc,px+w,py);
	LineTo(hdc,px+w,py+h);
	LineTo(hdc,px,py+h);
	LineTo(hdc,px,py);

	//scroll to the left 1 pixel
	BitBlt(hdc,px+1,py+1,w-2,h-2,hdc,px+2,py+1,SRCCOPY );

	//SelectObject(hdc,GetStockObject(WHITE_PEN));
	//MoveToEx(hdc,0,768,NULL);

	for(i=0;i<dlen;i++) {
//		int x,y;
//		scrbuff[i]= .7* (10*log(/*sqrt*/(fbuffr[i]*fbuffr[i]+fbuffi[i]*fbuffi[i]))) + .3*scrbuff[i]; //FFT
		yval = (int)data[i]+ 64;
		//yval = (int)(dwin[i]*256);
		if (yval > 255) yval = 255;
		if (yval < 0) yval = 0;
		//MoveToEx(hdc,i+MATH_BUFFER_SIZE,256+128,NULL);
//		x = (i );
//		y = (256+512)-yval;
		SetPixel(hdc,px+w-2,py+h-1-((int)(i*yscale+0.5)),colortable[(int)yval]);
		//SetPixel(hdc,1023,1000-(i<<1)-1,colortable[(int)yval]);
		/*
		SelectObject(hdc,greenPen);
		LineTo(hdc,x,y);

		SelectObject(hdc,GetStockObject(BLACK_PEN));
		MoveToEx(hdc,x,y+1,NULL);
		LineTo(hdc,x,950);
		//MoveToEx(hdc,x-1,y+1,NULL);
		//ineTo(hdc,x-1,950);
		//SetPixel(hdc,x,y,wcolor);
		MoveToEx(hdc,x,y,NULL);
		//MoveToEx(hdc,i,256,NULL);
		//LineTo(hdc,i,256-yval);
		*/
	}

	SelectObject(hdc,prevPen);
}


minMaxMeter::minMaxMeter()
{
	for (int i = 0; i < IMAX; i++) reset(i);
}


void minMaxMeter::reset(int ix,double maxBound)
{
	for (int i = 0; i <= HISTMAX; i++) hist[ix][i] = 0;
	hmax[ix] = maxBound;
}

void minMaxMeter::set(int ix, double value)
{
	int i;
	if (value < mmin[ix]) mmin[ix] = value;
	if (value > mmax[ix]) mmax[ix] = value;
	i = .5 + (value / hmax[ix])*HISTMAX;
	if (i>HISTMAX-1) i = HISTMAX-1;
	hist[ix][i]++;
}

void minMaxMeter::drawh(HDC hdc, int px, int py, int h, int ix)
{
	int i, y;
	HGDIOBJ prevPen;

	// Erase previous
	PatBlt(hdc, px, py, HISTMAX, h, BLACKNESS);
	
	//Draw bounding box
	prevPen = SelectObject(hdc, GetStockObject(WHITE_PEN));
	MoveToEx(hdc, px-1, py, NULL);
	LineTo(hdc, px + HISTMAX, py);
	LineTo(hdc, px + HISTMAX, py + h);
	LineTo(hdc, px-1, py + h);
	LineTo(hdc, px-1, py);



	for (i = 0; i < HISTMAX; i++) {
		MoveToEx(hdc, px + i, py + h,NULL);
		y = hist[ix][i];
		if (y>h) y = h;
		LineTo(hdc, px + i, py + h - y);
	}

	SelectObject(hdc, prevPen);
}

void minMaxMeter::draw(HDC hdc, int px, int py, int h, double valmax, int ix, COLORREF color)
{
	int i,ymax, ymin;
	//HPEN nPen;
	HGDIOBJ prevPen;

	ymin = h*mmin[ix] / valmax; 
	ymin = ymin < 0 ? 0 : ymin;

	ymax = h*mmax[ix] / valmax;
	ymax = ymax > h ? h : ymax;

	if (!nPen[ix]) {
		nPen[ix] = CreatePen(PS_SOLID, 1, color);
	}
	prevPen = SelectObject(hdc, nPen[ix]);


	if (ymin==ymax)
		SetPixel(hdc, px, py + h - ymin, color);
	else {
		MoveToEx(hdc, px, py + h - ymin, NULL);
		LineTo(hdc, px, py + h - ymax);
	}


//	for (i = ymin; i <= ymax; i++) {
//		SetPixel(hdc, px, py + h - i, color);
//	}

	mmin[ix] = 999999999.;
	mmax[ix] = -999999999.;

	SelectObject(hdc, prevPen);
}

minMaxMeter::~minMaxMeter()
{
	for (int i = 0; i < 8;i++)
		if (nPen[i])
			DeleteObject(nPen[i]);
}
