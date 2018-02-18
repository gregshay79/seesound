#include "stdafx.h"
//#include "SeeSound.h"
#include <math.h>
#include "DisplayFunctions.h"
#include "knobs.h"

#define BOUND(Y,L,H) (Y=Y<L?L:(Y>H)?H:Y)

class minMaxMeter mm;
double gamma = 2.;// 1.5;
int colorTableInitFlag=0;
extern double pi;

// Draw a linear bar meter
// Linear, size max{sx or sy}, constant fixed width overrides min{sx,sy}.
// *data input as array of values
// If dataLan > 2, this may be min,max or min,avg,max, or something
// draw a tick mark at the value tickmark (defaults to 0.)
// dBflag = 0, linear meter
// dBflag = 1, dB transform on data
// dBflag = 2, data is sum square value, take sqrt() before dB
void DisplayMeterBar(HDC hdc, int dBflag, int x, int y, int sx, int sy, 
				double *data, int dataLen, double min, double max, double tickmark, int drawFromOrigin)
{
	HPEN markerPen;
	HGDIOBJ prevObj;
	HBRUSH mybrush;
	int color;
	TCHAR strval[8];
	int i, ival;
	int itick;
	double scale, v;
	int width = 8;

	v = data[0];
	if (dBflag) {
		if (dBflag == 2)
			v = sqrt(v);
		v = 20 * log10(v + 1E-20); // Add fuzz to protect against -infinity here
	}


	// figure out if horizontal or vertical
	if (sx > sy) {
		sy = width;
		scale = sx / (max - min);
	}
	else {
		sx = width;
		scale = sy / (max - min);
	}


	// Erase previous
	PatBlt(hdc, x, y, sx, sy, BLACKNESS);

	//Draw bounding box and center line 
	prevObj = SelectObject(hdc, GetStockObject(WHITE_PEN));
	MoveToEx(hdc, x, y, NULL);
	LineTo(hdc, x + sx, y);
	LineTo(hdc, x + sx, y + sy);
	LineTo(hdc, x, y + sy);
	LineTo(hdc, x, y);
	
	markerPen = CreatePen(PS_SOLID, 1, RGB(64, 64, 128)); //Tickmark dim blue grey
	SelectObject(hdc, markerPen);
	if (sx > sy) { // horizontal
		MoveToEx(hdc, itick=(int)(x + (0.5+scale*(tickmark-min))), y, NULL);
		LineTo(hdc, itick, y+sy);
	}
	else { // vertical
		MoveToEx(hdc, x, itick = (int)(y + sy - (0.5 + scale*(tickmark - min))), NULL);
		LineTo(hdc, x + sx, itick);
	}
	SelectObject(hdc, prevObj);
	DeleteObject(markerPen);

	////if datalen ==3, 3rd value is the unwrapped phase angle
	//if (dataLen == 3){
	//	//		swprintf_s(strval, L"%2d", (int)((data[2]+pi) / (2 * pi)));
	//	swprintf_s(strval, L"%3.2lf", data[2] / (2 * pi));

	//	TextOut(hdc, x + sz - 32, y + 4, strval, lstrlenW(strval));
	//}

	// Display meter bar with a rectangle
	y += sy;  // move origin to bottom left.  remember to subtract y pixel locations
	mybrush = CreateSolidBrush(RGB(200, 128, 50)); // orange?
	prevObj = SelectObject(hdc, mybrush);

	ival = ((int)((v-min) * scale + 0.5));

	if (sx > sy){ // horizontal
		BOUND(ival, 1, sx - 1);
		if (drawFromOrigin) {

		}
		else
			PatBlt(hdc, x + 1, y - 1, ival, sy - 2, PATCOPY);
	}
	else { // vertical
		BOUND(ival, 1, sy - 1);
		if (drawFromOrigin) {
			ival = y - 1 - ival;
			if (ival > itick)
				PatBlt(hdc, x + 1, itick, sx - 2, ival-itick, PATCOPY);
			else
				PatBlt(hdc, x + 1, ival, sx - 2, itick-ival, PATCOPY);
		}
		else {
			PatBlt(hdc, x + 1, y-1-ival, sx - 2, ival, PATCOPY);
		}
	}

	SelectObject(hdc, prevObj);
	DeleteObject(mybrush);
}



// Draw phasor diagram
// Always square, size sz
// *data input as array of (re,im) values
// If dataLan > 2, this may be min,max or min,avg,max, or something
// If phmax > pi, would be for an unrolled complexLog()
void DisplayPhasor(HDC hdc, int x, int y, int sz, double *data, int dataLen, double magmax, double phmax)
{
	HPEN markerPen;
	HGDIOBJ prevPen;
	int color;
	TCHAR strval[8];
	int i, yval,xval, center=sz/2;
	double scale = sz/2 / (magmax);

	// Erase previous
	PatBlt(hdc, x, y, sz, sz, BLACKNESS);

	//Draw bounding box and center line 
	prevPen = SelectObject(hdc, GetStockObject(WHITE_PEN));
	MoveToEx(hdc, x, y, NULL);
	LineTo(hdc, x + sz, y);
	LineTo(hdc, x + sz, y + sz);
	LineTo(hdc, x, y + sz);
	LineTo(hdc, x, y);
	//crosshairs dim blue grey
	markerPen = CreatePen(PS_SOLID, 1, RGB(64, 64, 128));
	SelectObject(hdc, markerPen);
	MoveToEx(hdc, x, y + center, NULL);
	LineTo(hdc, x + sz, y + center);
	MoveToEx(hdc, x+center, y, NULL);
	LineTo(hdc, x + center, y + sz);

	SelectObject(hdc, prevPen);
	DeleteObject(markerPen);

	//if datalen ==3, 3rd value is the unwrapped phase angle
	if (dataLen == 3){
//		swprintf_s(strval, L"%2d", (int)((data[2]+pi) / (2 * pi)));
		swprintf_s(strval, L"%3.1lf", data[2]/(2*pi) );

		TextOut(hdc, x+sz-40, y+4, strval, lstrlenW(strval));
	}

	// Display phase vector
	y += sz;  // move origin to bottom left.  remember to subtract y pixel locations
	color = RGB(0, 255, 128);  // default green pen
	markerPen = CreatePen(PS_SOLID, 1, color);
	prevPen = SelectObject(hdc, markerPen);

	xval = ((int)(data[0] * scale + 0.5));
	yval = ((int)(data[1] * scale + 0.5));

	//	if (yval > h) yval=h;
	//	if (yval < 0) yval=0;
	BOUND(yval, -center, center);
	BOUND(xval, -center, center);

	MoveToEx(hdc, x+center, y - center, NULL);
	LineTo(hdc, x + center + xval, y - center - yval);

	SelectObject(hdc, prevPen);
	DeleteObject(markerPen);
}


// If grid is zero, draw no grid.
// If grid is nonzero and positive, draw an x/y grid at the given increment of input units (before display gain).
// if grid is negative, draw only a y grid 
void DisplayWaveform(HDC hdc,int x,int y,int w,int h,double *data,int dataLen,double ymin, double ymax ,int grid,int erase, int color)
{
	HPEN markerPen;
	HGDIOBJ prevPen;
	int i,yval,ycenter=h/2;
	double yscale = h/(ymax - ymin);
	double xscale = ((double)(w))/dataLen;
	int gridinc = abs(grid);


	// Erase previous
	if (erase)
		PatBlt(hdc,x,y,w,h,BLACKNESS);


	//Draw bounding box 
	prevPen= SelectObject(hdc,GetStockObject(WHITE_PEN));
	MoveToEx(hdc,x,y,NULL);
	LineTo(hdc,x+w,y);
	LineTo(hdc,x+w,y+h);
	LineTo(hdc,x,y+h);
	LineTo(hdc,x,y);

	//MoveToEx(hdc, x, y + ycenter, NULL);
	//LineTo(hdc, x + w, y + ycenter);

#if 1
	if (grid) {
		// Draw graticule
		markerPen = CreatePen(PS_DOT /*PS_DOT*/, 1, RGB(128, 128, 150)); // dotted light greyblue pen
		prevPen=SelectObject(hdc,markerPen);
		SetBkColor(hdc,RGB(0, 0, 0));

		// draw center line 
		MoveToEx(hdc, x, y + ycenter, NULL);
		LineTo(hdc, x + w, y + ycenter);

			//Draw vertical grid lines
		if (grid > 0) {
			for (i = gridinc; i < w / xscale; i += gridinc) {
				MoveToEx(hdc, (int)(x+i*xscale), y + 1, NULL);
				LineTo(hdc, (int)(x+i*xscale), y + h - 1);
			}
		}
		//Draw horizontal grid lines
		for (i = gridinc; i<h / yscale / 2; i += gridinc) {
			int ypos;
			ypos = (int)(y + ycenter + i*yscale);
			if (ypos>y+h)continue;
			MoveToEx(hdc,x+1,ypos,NULL);
			LineTo(hdc,x+w-1,ypos);
			ypos = (int)(y + ycenter - i*yscale);
			MoveToEx(hdc,x+1,ypos,NULL);
			LineTo(hdc,x+w-1,ypos);
		}

		SelectObject(hdc,prevPen);
		DeleteObject(markerPen);
	}
#endif

	// Display waveform
	y += h;
	if (!color)
		color = RGB(0, 255, 128);  // default green pen
	markerPen = CreatePen(PS_SOLID,1,color);  
	prevPen=SelectObject(hdc,markerPen);
	yval = ((int)((data[0]-ymin) * yscale));
//	if (yval > h) yval=h;
//	if (yval < 0) yval=0;
	BOUND(yval,0,h);
	MoveToEx(hdc,x,y-yval,NULL);
	for(i=0;i<dataLen;i++) {
		yval = (int)((data[i]-ymin)*yscale);
//		yval = ycenter - yval;
		BOUND(yval,0,h);
//		if (yval > h) yval=h;
//		if (yval < 0) yval=0;
		//SetPixel(hdc,i,(256+128)-yval,wcolor);
		LineTo(hdc,(int)(x+i*xscale),y-yval);
	}

	SelectObject(hdc,prevPen);
	DeleteObject(markerPen);
}

extern HWND hWnd;
// If grid is zero, draw no grid.
// If grid is nonzero and positive, draw an x/y grid at the given increment of input units (before display gain).
// if grid is negative, draw only a y grid 
int tcstate = 0;
void DisplayTriggeredWaveform(HDC hdc, int x, int y, int w, int h, double *data, int dataLen, double ymin, double ymax, int grid, int erase, int color,
struct button *triggerModeButton, struct knob *threshKnob, struct button *armButton, int timecompressratio)
{
	int i,j,k;

	static int aboveBelow = 0;
	static int triggerState = 2;
	static double tcmem[1024];
//	static int tcstate=0;
	static double datamemory[1024];
	static double prememory[1024];

	static int memPosition = 0;

	// implement time compression logic
	if (timecompressratio > 1) {
		int filloffset = (dataLen / timecompressratio)*tcstate;
		for (int i = 0; i < dataLen / timecompressratio; i++) {
			tcmem[filloffset + i] = data[i*timecompressratio];
		}
		if (++tcstate < timecompressratio)
			return;
	}
	tcstate = 0;// reset for next set of input blocks,
	data = tcmem; // go draw this one

	// Implement trigger logic
	// triggerMode 0 : no trigger, free run
	// triggerMode 1 : auto trigger, use threshold
	// triggerMode 2 : one shot, use threshold, arm button to arm

	// trigger state, data collection state:
	// 0 do nothing
	// 1 waiting for trigger to happen
	// 2 collection data in memory after trigger happens, or always if no trigger mode
	// 3 display memory when full
	// 4 reset to #1 if autotrigger mode, reset to 0 if single shot mode

	switch (triggerState) {
	case(0) : // waiting for arm button to re-arm
		if (armButton && armButton->value == 1) {
			triggerState = 1;
		}
		if (triggerModeButton->value == 0) triggerState = 2;
		break;
	case(1) :  //Wait for trigger
		//scan input data for value greater than trigger threshold
		for (i = 0; i < dataLen; i++) {
			if ((!threshKnob) || (aboveBelow==0)&&(data[i] > threshKnob->value) || (triggerModeButton->value == 0)) {
				if (data[i] > threshKnob->value)
					aboveBelow = 1;
					//Here found trigger.
					// use pretrigger length
					k = knobs[KNOB_pretrigger].value;
					if ((j = k - i) > 0) {
						// get 'j' number of samples from end of previous datamemory
						memcpy(datamemory, &prememory[dataLen - j - 1], sizeof(double)*j);
						k -= j;
					}
					else j = 0;

					memcpy(&datamemory[j], &data[i - k], sizeof(double)*(dataLen - (i - k)));
					memPosition = j + dataLen - (i - k);
					triggerState = 2; // next, complete filling memory
					if (armButton) {
						armButton->value = 0;
						RedrawWindow(armButton->hwnd, NULL, NULL, RDW_INVALIDATE);
						UpdateWindow(armButton->hwnd);
					}
					break; // break out of for() loop searching for trigger
			}
			else if (aboveBelow == 1) {
				if (data[i] < threshKnob->value)
					aboveBelow = 0;
			}
		}
		//if (i == dataLen) // no trigger, copy whole data to datamemory
		//	memcpy(prememory, data, sizeof(double)*dataLen); 
		break;
	case(2) : // complete filling of memory
		i = dataLen - memPosition;
		if (i > 0) {
			memcpy(&datamemory[memPosition], data, sizeof(double)*i);
		}
		memPosition += i;
		if (memPosition == dataLen) {
			triggerState = 3;
			memPosition = 0;
		}
		break;
	case(3) :
		break;
	default:
		break;
	}

	memcpy(prememory, data, sizeof(double)*dataLen); // store prememory unconditionally

	//Decide if waiting for trigger, or displaying
	//	if (triggerModeButton && (triggerModeButton->value == 2) && (triggerState == 0))
	//		return; 

	//
	//	markerPen = CreatePen(PS_SOLID,1,RGB(0,255,128)); // green pen

	if (triggerState == 3) {
		DisplayWaveform(hdc, x, y, w, h, datamemory, dataLen, ymin, ymax, grid, erase, color);

	// Decide what to do next
	// one shot mode, 
	if (!triggerModeButton || (triggerModeButton->value == 0)) {
		triggerState = 2; // keep running, no trigger
	}
	else
		if (triggerModeButton->value == 1) {
			triggerState = 1; // go wait for trigger
		}
		else
			if (triggerModeButton->value == 2)
				triggerState = 0; // go wait for re-arm
	}

}

void DisplayStripChart(HDC hdc, int px, int py, int w, int h, double *data, int dlen, double dmin, double dmax, double grid)
{
	int i, val, x, y, c;
	HGDIOBJ prevPen;
	HPEN markerPen;
	double valscale;
	static int lastpos;

	valscale = (h-1.0) / (dmax - dmin);

	//Draw bounding box 
	prevPen = SelectObject(hdc, GetStockObject(WHITE_PEN));
	MoveToEx(hdc, px, py, NULL);
	LineTo(hdc, px + w, py);
	LineTo(hdc, px + w, py + h);
	LineTo(hdc, px, py + h);
	LineTo(hdc, px, py);
	
	// draw horizontal grid lines
	if (grid > 1E-10){
		//draw zero line
		markerPen = CreatePen(PS_SOLID, 1, RGB(64, 64, 128));
		SelectObject(hdc, markerPen);
		MoveToEx(hdc, px, py + h / 2, NULL);
		LineTo(hdc, px + w, py + h / 2);
		SelectObject(hdc, prevPen);
		DeleteObject(markerPen);

		// Draw graticule
		markerPen = CreatePen(PS_DOT , 1, RGB(128, 128, 150)); // dotted light greyblue pen
		prevPen = SelectObject(hdc, markerPen);
		SetBkColor(hdc, RGB(0, 0, 0));
		i = 1;
		while ((y = (int)(i*grid*valscale+0.5)) < h/2) 
			{
				MoveToEx(hdc, px, py+y + h / 2 , NULL);
				LineTo(hdc, px + w, py+y + h / 2);
				MoveToEx(hdc, px, py+ h / 2 - y, NULL);
				LineTo(hdc, px + w, py+ h / 2 - y);
				i++;
			}
		SelectObject(hdc, prevPen);
		DeleteObject(markerPen);
	}


	//scroll left 1 pixel
	BitBlt(hdc, px + 1, py + 1, w - 2, h - 2, hdc, px + 2, py + 1, SRCCOPY);

	//SelectObject(hdc,GetStockObject(WHITE_PEN));
	//MoveToEx(hdc,0,768,NULL);
	y = ((int)((data[0] - dmin)*valscale + 0.5));
	if ((y>0) && (y<h))
		SetPixel(hdc, px + w - 2, py + h - 1 - y,RGB(255,200,50));
}

void DisplayHScrolling(HDC hdc,int px,int py,int w,int h,double *data,int dlen)
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
		SelectObject(hdc,markerPen);
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

void DisplayVScrolling(HDC hdc, int px, int py, int w, int h, double *data, int dlen, double dmin, double dmax)
{
	int i, val,x,y,c;
	int doublepix = 0;
	HGDIOBJ prevPen;
	double xscale;
	double valscale;
	static int colortable[256];

	valscale = 255./(dmax - dmin);

	if (!colorTableInitFlag){
		for (i = 0; i<256; i++) {
			double v, v3;
			int r, g, b;
			v = i / 255.;  // v ranges from 0.0 to 1.0
			v3 = v*v;//*v;   // v^3
			v3 = (exp(gamma*v) - 1) / (exp(gamma) - 1); // normalize to 1
			r = (int)((v * 255 + v3 * 255) / 2.);	// r = 1/2v^3 + 1/2v
			g = (int)(v3*255.);				// g = v^3
			b = g;
			colortable[i] = RGB(r, g, b);
		}
		colorTableInitFlag++;
	}

	xscale = ((double)w) / dlen;
	if (w / dlen == 2) {
		doublepix = 1;
	}

	//Draw bounding box
	prevPen = SelectObject(hdc, GetStockObject(WHITE_PEN));
	MoveToEx(hdc, px, py, NULL);
	LineTo(hdc, px + w, py);
	LineTo(hdc, px + w, py + h);
	LineTo(hdc, px, py + h);
	LineTo(hdc, px, py);

	//scroll down 1 pixel
	BitBlt(hdc, px + 1, py + 2, w - 2, h - 2, hdc, px + 1, py + 1, SRCCOPY);

	//SelectObject(hdc,GetStockObject(WHITE_PEN));
	//MoveToEx(hdc,0,768,NULL);

	for (i = 0; i<dlen; i++) {
		//		int x,y;
		//		scrbuff[i]= .7* (10*log(/*sqrt*/(fbuffr[i]*fbuffr[i]+fbuffi[i]*fbuffi[i]))) + .3*scrbuff[i]; //FFT
		val = (int)(valscale*(data[i]-dmin) + 64.5);
		//yval = (int)(dwin[i]*256);
		if (val > 255) val = 255;
		if (val < 0) val = 0;
		//MoveToEx(hdc,i+MATH_BUFFER_SIZE,256+128,NULL);
		//		x = (i );
		//		y = (256+512)-yval;
//		SetPixel(hdc, px + w - 2, py + h - 1 - ((int)(i*yscale + 0.5)), colortable[(int)val]);
		SetPixel(hdc, x=px + 1 + ((int)(i*xscale + 0.5)), y=py + 1, c=colortable[(int)val]);
		if (doublepix)
			SetPixel(hdc, x + 1, y, c);
		/*
		SelectObject(hdc,markerPen);
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

	SelectObject(hdc, prevPen);
}

minMaxMeter::minMaxMeter()
{
	for (int i = 0; i < IMAX; i++) {
		reset(i);
	}
}

void minMaxMeter::reset(int ix,double maxBound,double Tc)
{

	for (int i = 0; i <= HISTMAX; i++) {
		histfilt[ix][i] = Tc*histfilt[ix][i] + (1 - Tc)*hist[ix][i];
		hist[ix][i] = 0;
	}
	hmax[ix] = maxBound;
}

void minMaxMeter::set(int ix, double value)
{
	int i;
	if (value < mmin[ix]) mmin[ix] = value;
	if (value > mmax[ix]) mmax[ix] = value;
	if (value < 0) return; // the histogram seems to only be written for positive values.
	i = (int)(.5 + (value / hmax[ix])*HISTMAX);
	if (i>HISTMAX-1) i = HISTMAX-1;
	hist[ix][i] += 1.;
}

void minMaxMeter::drawh(HDC hdc, int px, int py, int h, int ix)
{
	int i, y;
	HGDIOBJ prevPen;
	HPEN markerPen;
	float sum = 0;
	float ygain = 1.0;

	// Erase previous
	PatBlt(hdc, px, py, HISTMAX, h, BLACKNESS);
	
	//Draw bounding box
	markerPen = CreatePen(PS_SOLID, 1, RGB(64, 64, 128));
	prevPen = SelectObject(hdc, markerPen);
	
	MoveToEx(hdc, px-1, py, NULL);
	LineTo(hdc, px + HISTMAX, py);
	LineTo(hdc, px + HISTMAX, py + h);
	LineTo(hdc, px-1, py + h);
	LineTo(hdc, px-1, py);


	prevPen = SelectObject(hdc, GetStockObject(WHITE_PEN));
	for (i = 0; i < HISTMAX; i++) sum += histfilt[ix][i];
	ygain = (h / 10.)*HISTMAX / sum;
	for (i = 0; i < HISTMAX; i++) {
		MoveToEx(hdc, px + i, py + h,NULL);
		y = (int)(0.5+ygain*histfilt[ix][i]);//
		if (y>h) y = h;
		LineTo(hdc, px + i, py + h - y);
	}


	// Compute and print median value
	float isum = 0.;
	float thresh2 = sum / 2.;
	float thresh4 = sum / 4.;
	float thresh34 = thresh4 * 3;
	float threshtot = sum*.95;

	float median, lowmed, highmed;
	int im=0, il=0, ih=0, itot=0;

	TCHAR dispStr[16];

	for (i = 0; i < HISTMAX; i++) {
		isum += histfilt[ix][i];
		if (isum < thresh4)
			il = i;
		if (isum < thresh2)
			im = i;
		if (isum < thresh34)
			ih = i;
		if (isum < threshtot)
			itot = i;
	}
	
	median = ((float) im / HISTMAX)*hmax[ix];

	BitBlt(hdc, px+ HISTMAX + 4, py, 64, 64, 0, 0, 0, BLACKNESS);

	swprintf_s(dispStr, L"%8.2g", (((float)i)/HISTMAX)*hmax[ix]);
	TextOut(hdc, px + HISTMAX + 5, py + 1, dispStr, lstrlenW(dispStr));

	swprintf_s(dispStr, L"%5.2g", (ih - il) / (float)itot);
	TextOut(hdc, px + HISTMAX + 5, py + 1 + 16, dispStr, lstrlenW(dispStr));




	SelectObject(hdc, prevPen);
	DeleteObject(markerPen);	
}

void minMaxMeter::draw(HDC hdc, int px, int py, int h, double valmax, int ix, COLORREF color)
{
	int ymax, ymin;
	//HPEN nPen;
	HGDIOBJ prevPen;

	ymin = (int)(h*mmin[ix] / valmax); 
	ymin = ymin < 0 ? 0 : ymin;

	ymax = (int)(h*mmax[ix] / valmax);
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
