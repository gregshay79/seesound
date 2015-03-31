#include "stdafx.h"
#include <math.h>
#include <stdio.h>

#define MAXN 4096
#define MAXLOGN 12


double xinr[MAXN],xini[MAXN];
double xoutr[MAXN],xouti[MAXN];
int bits[MAXN];
double wr[MAXN],wi[MAXN];
//int LOGN,N;

void fft(double *xinr,double *xini,double *xoutr,double *xouti,int n, int forward);


void initw(int n)
{
	static int initflag=0;
	double pi,theta;
	int k;

if (initflag) return;
pi = 3.141592653589793238462643383;
theta = 2.*pi/((double)n);
for(k=0;k<n;k++)
	{
	wr[k]=cos(theta*k);
	wi[k]=sin(theta*k);
	}
initflag++;
}

void bitrev(int log2n)
{
	static int initflag=0;
	int i,j,m;

if (initflag) return;
bits[0]=0;
m=1;
for(i=0;i<=log2n-1;i++)
	{
	for(j=0;j<=m-1;j++)
		{
		bits[j] <<= 1;
		bits[j+m]=bits[j]+1;
		}
	m <<= 1;
	}
initflag++;
} /* End of bitrev */

#if 0
void fft_main(n,int forward)
{
	int i,j,k,flag;
	char dtype;
	double x;

flag = 0;
/* input size of array */
scanf("%c\n",&dtype);
if ((dtype != 'r') && (dtype != 'c'))
	{
//	fprintf(stderr,"Incompatible input data file, expect real or complex\n");
	flag = 1;
	}
scanf("%d \n",&LOGN);
if ((LOGN<2) || (LOGN>MAXLOGN) )
	{
	flag = 1;
//	fprintf(stderr,"Error, size of array, %d, out of range.\n",LOGN);
	}

N = 1 << LOGN;
if (N>MAXN) 
	{
//	fprintf(stderr,"Error, maximum size of array is set to %d\n",MAXN);
	flag = 1;
	}

if (flag == 0)
  {
/* Initialize omega array and bit reversal array */

initw();
bitrev();

/* Read in data */
if (dtype == 'r')
	for(i=0;i<N;i++)
	{
	scanf("%f ",&x);
	xinr[i] = x;
	xini[i] = 0.;
	}
if (dtype == 'c')
	for(i=0;i<N;i++)
	{
	scanf("%f ",&x);
	xinr[i] = x;
	scanf("%f ",&x);
	xini[i] = x;
	}

/* Perform fft */

fft(xinr,xini,xoutr,xouti,n,forward);

/* Output data */

//printf("c\n");
//printf("%d \n",LOGN);
for(i=0;i<N;i++)
	printf("%4.15f %4.15f\n",xoutr[i],xouti[i]);

  } /* end of if flag == 0 clause */
} /* End of main */
#endif



void fft(double *xinr,double *xini,double *xoutr,double *xouti,int n,int forward)
{
	int i,j,k,istart,c,d;
	double ar,ai,br,bi,fn;
	int joff,jpnt,ioff,jwpnt,jbk;
	int log2n = -1;

j=n;
while(j) {
	j= j>>1;
	log2n++;
}
/* Initialize omega array and bit reversal array */
initw(n);
bitrev(log2n);

for(i=0;i<=n-1;i++)
	{
	xoutr[i]=xinr[bits[i]];
	xouti[i]=xini[bits[i]];
	}
joff=n/2;
jpnt=n/2;
jbk=2;
ioff=1;
for(i=1;i<=log2n;i++)
	{
	for(istart = 0;istart<=n-1;istart+=jbk)
		{
		jwpnt=0;
		for(k=istart;k<=istart+ioff-1;k++)
			{
			c=k+ioff;
			d=jwpnt+joff;
			if (forward == 1)
			   {
			   ar=xoutr[k]+xoutr[c]*wr[jwpnt]-xouti[c]*wi[jwpnt];
			   ai=xouti[k]+xoutr[c]*wi[jwpnt]+xouti[c]*wr[jwpnt];
			   br=xoutr[k]+xoutr[c]*wr[d]-xouti[c]*wi[d];
			   bi=xouti[k]+xoutr[c]*wi[d]+xouti[c]*wr[d];
			   }
			else
			   {
			   ar=xoutr[k]+xoutr[c]*wr[jwpnt]+xouti[c]*wi[jwpnt];
			   ai=xouti[k]+xouti[c]*wr[jwpnt]-xoutr[c]*wi[jwpnt];
			   br=xoutr[k]+xoutr[c]*wr[d]+xouti[c]*wi[d];
			   bi=xouti[k]+xouti[c]*wr[d]-xoutr[c]*wi[d];
			   }
			xoutr[k]=ar;
			xouti[k]=ai;
			xoutr[c]=br;
			xouti[c]=bi;
			jwpnt += jpnt;
			} /* End of k loop */
		} /* End of istart loop */
	jpnt >>= 1;
	ioff = jbk;
	jbk <<= 1;
	} /* End of i loop */

if(forward == 1)
	fn = ((double)n)/2.;
else
	fn = 2.;   

for (i=0;i<=n-1;i++)
	{
	xoutr[i] = xoutr[i]/fn;
	xouti[i] = xouti[i]/fn;
	}
	

} /* End of fft subroutine */

