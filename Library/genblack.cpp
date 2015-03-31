#include "stdafx.h"
#include <stdio.h>
#include <math.h>

void genblack(double *win, int n)
{
	int i;
	double theta;
	double pi;

pi = 3.141592653589793238462643383;
for(i=0;i<n;i++)
	{
	theta = 2*i*pi/n;
	win[i] = 0.42 - 0.5*cos(theta)+0.08*cos(2*theta);
	}
}

// coefficients for a 500hz high pass biquad
double BQhipass300[5]= {
0.8074423594751792,
-1.6148847189503583,
0.8074423594751792,
-1.5509899803923664,
0.6787794575083504
};

double BQhipass1k[5]= {
 0.6306019374818708,
 -1.2612038749637415,
 0.6306019374818708,
 -1.044815499854966,
 0.47759225007251715
};

double BQhipass2k[5] = {
 /* a0 =*/ 0.3333333333333333,
 /*a1 =*/ -0.6666666666666666,
 /*a2 =*/ 0.3333333333333333,
 /*b1 = */ -1.4802973661668753e-16,
 /*b2 =*/ 0.33333333333333326
};

double BQlopass1k[5] = {
0.10819418755438784,
0.21638837510877568,
0.10819418755438784,
-1.044815499854966,
0.47759225007251715
};


double biquad(double input, double fc[],double z[])
{
static bool initialized=0;
//static double z[2];
	double output;

//	if (!initialized) {
//		initialized=true;
//		z[0]=z[1]=0.0;
//	}

	output = z[1] + fc[0]*input;
	z[1] =   z[0] + fc[1]*input - fc[3]*output;
	z[0] =			fc[2]*input - fc[4]*output;
return(output);
}


#if 0
#define MAX_DELAY 1024

double delay(double input,int dlen)
{
	static bool initalized=0;
	static double z[MAX_DELAY];
	static point = 0;
	double output;
	int i;

if (!initialized) {
	initialized=true;
	for (int i=0;i<MAX_DELAY;i++)
		z[i]=0.0;
}
	

if (dlen > MAX_DELAY) dlen=MAX_DELAY-1;
z[point]=input;

#endif