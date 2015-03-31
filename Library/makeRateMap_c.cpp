/*
 *=============================================================
%MAKERATEMAP_C: A C implementation of MPC's makeRateMap matlab code
%--------------------------------------------------
%  ratemap = makeRateMap_c(x,fs,lowcf,highcf,numchans,frameshift,ti,compression)
% 
%  x           input signal
%  fs          sampling frequency in Hz (8000)
%  lowcf       centre frequency of lowest filter in Hz (50)
%  highcf      centre frequency of highest filter in Hz (3500)
%  numchans    number of channels in filterbank (32)
%  frameshift  interval between successive frames in ms (10)
%  ti          temporal integration in ms (8)
%  compression type of compression ['cuberoot','log','none'] ('cuberoot')
%
%  e.g. ratemap = makeRateMap_c(x,8000,50,3850,32,10);
%
%
%  You should compile makeRateMap_c.c before using it.
%  In Matlab command line, type: mex makeRateMap_c.c
%
%  For more detail on this implementation, see
%  http://www.dcs.shef.ac.uk/~ning/resources/ratemap/
%
%  Ning Ma, University of Sheffield
%  n.ma@dcs.shef.ac.uk, 08 Dec 2005
%

 * $Test: 22 Dec 2005 $
 *  Tested using a Linux PC Pentium4 2.0G Hz. This implementation
 *  takes 0.29 secs to compute a 64-channel ratemap for a 
 *  signal with a duration of 3.7654 secs and a sampling rate 
 *  of 20K. Matlab code takes 9.56 secs for the same signal.
 *
 * You should compile makeRateMap_c.c first.
 * In Matlab command line, type: mex makeRateMap_c.c
 * This will generate a C library file in the same folder 
 * (.mexglx on Linux, .dll on Windows)
 *
 * If this is your first time to run mex, you will be asked
 * to choose a compiler when running the mex command. I recommand
 * gcc compiler on Linux and Microsoft Visual C++ compiler on
 * Windows as they generate faster code. If you don't have these
 * compilers, the Matlab C compiler is also fine.
 *=============================================================
 */

#include "stdafx.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
//#include "mex.h"
//#include "matrix.h"


/*=======================
 * Input arguments
 *=======================
 */
#define IN_x            prhs[0]
#define IN_fs           prhs[1]
#define IN_lowcf        prhs[2]
#define IN_highcf       prhs[3]
#define IN_numchans     prhs[4]
#define IN_frameshift   prhs[5]
#define IN_ti           prhs[6]
#define IN_compression  prhs[7]

/*=======================
 * Output arguments
 *=======================
 */
#define OUT_ratemap     plhs[0]

/*=======================
 * Useful Const
 *=======================
 */
#define BW_CORRECTION   1.019
#define VERY_SMALL_NUMBER  1e-200
#define LOG10_VERY_SMALL_NUMBER  -200

#define MAX_NUM_CHANNELS 512

#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif

/*=======================
 * Utility functions
 *=======================
 */
#define getMax(x,y)     ((x)>(y)?(x):(y))
#define getRound(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

#define erb(x)          (24.7*(4.37e-3*(x)+1.0))
#define HzToErbRate(x)  (21.4*log10(4.37e-3*(x)+1.0))
#define ErbRateToHz(x)  ((pow(10.0,((x)/21.4))-1.0)/4.37e-3)


/*=======================
 * Main Function 
 *=======================
 */
//--------------------------------------------------
//  makeRateMap(ratemap,numframesExport,x,fs,lowcf,highcf,numchans,
//				   frameshift,ti,compression)
// 
//  ratemap		output matrix
//  numframesExport  Number of output frames
//  x           input signal
//  fs          sampling frequency in Hz (default 8000)
//  lowcf       centre frequency of lowest filter in Hz (default 50)
//  highcf      centre frequency of highest filter in Hz (default 3500)
//  numchans    number of channels in filterbank (default 32)
//  frameshift  interval between successive frames in ms (default 10)
//  ti          temporal integration in ms (default 8)
//  compression type of compression ['cuberoot','log','none'] (default 'cuberoot')
//
//  Pass in a value of -1 to get a parameter default value.
//  e.g. ratemap = makeRateMap_c(x,8000,50,3850,32,10);
//

double cfarray[MAX_NUM_CHANNELS];

double p1rs[MAX_NUM_CHANNELS], p2rs[MAX_NUM_CHANNELS], p3rs[MAX_NUM_CHANNELS], p4rs[MAX_NUM_CHANNELS],
	   p1is[MAX_NUM_CHANNELS], p2is[MAX_NUM_CHANNELS], p3is[MAX_NUM_CHANNELS], p4is[MAX_NUM_CHANNELS]; 
double senv[MAX_NUM_CHANNELS],css[MAX_NUM_CHANNELS],sns[MAX_NUM_CHANNELS];

void makeRateMap(double *ratemap,int *numframesExport, double *x, int nsamples, int fs, 
				 double lowcf, double highcf, int numchans, double frameshift, double ti, int compression_type)
//void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
  //double /* *x, *ratemap,*/ *senv;
  int i, j, chan;//, fs, numchans;
  int /*nsamples,*/ nsamples_padded, frameshift_samples, numframes;
  double /*lowcf, highcf, frameshift, ti,*/ lowErb, highErb, spaceErb, cf;
  double a, tpt, tptbw, gain, intdecay, intgain, sumEnv;
  double p0r, p1r, p2r, p3r, p4r, p0i, p1i, p2i, p3i, p4i;

  double a1, a2, a3, a4, a5, cs, sn, u0r, u0i;
  double senv1, oldcs, oldsn, coscf, sincf;
  int frameCount,subFrameCount;
  static int initFlag=0;

  // compression_type : 0=none, 1 = cuberoot, 2=log
  
  /*=========================================
   * input arguments
   *=========================================
   */
//  char compression[20] = "cuberoot";
//  if (nrhs == 8) mxGetString(IN_compression, compression, 19);
    if (compression_type<0) compression_type=1;

//  if (nrhs < 1) { mexPrintf("??? Not enough input arguments.\n"); return; }

//  if (nrhs < 2) fs = 8000;
//  else fs = (int) mxGetScalar(IN_fs);
	if (fs<0)fs=8000;

//  if (nrhs < 3) lowcf = 50;
//  else lowcf = mxGetScalar(IN_lowcf);
	if (lowcf <0) lowcf=50;

//  if (nrhs < 4) highcf = 3500;
//  else highcf = mxGetScalar(IN_highcf);
	if (highcf<0) highcf=3500;

//  if (nrhs < 5) numchans = 32;
//  else numchans = (int) mxGetScalar(IN_numchans);
  if (numchans < 0) numchans = 32;
  
//  if (nrhs < 6) frameshift=10;
//  else frameshift = mxGetScalar(IN_frameshift);
	if (frameshift < 0)
		frameshift = 10;

//  if (nrhs < 7) ti = 8;
//  else ti = mxGetScalar(IN_ti);
	if (ti<0) ti=8;  

  
//  x = mxGetPr(IN_x);
//  i = mxGetN(IN_x);
//  j = mxGetM(IN_x);
//  if (i>1 && j>1) { mexPrintf("??? Input x must be a vector.\n"); return; }

//  nsamples = getMax(i,j);
  frameshift_samples = getRound(frameshift*fs/1000);
  numframes = (int)ceil((double)nsamples / (double)frameshift_samples);
  *numframesExport = numframes;
  nsamples_padded = numframes*frameshift_samples;

  /*=========================================
   * output arguments
   *=========================================
   */
//  OUT_ratemap = mxCreateDoubleMatrix(numchans, numframes, mxREAL);
//  ratemap = mxGetPr(OUT_ratemap);
//  ratemap = new double[numchans*numframes];
  
  lowErb = HzToErbRate(lowcf);
  highErb = HzToErbRate(highcf);
  
  if (numchans > 1)  spaceErb = (highErb-lowErb)/(numchans-1);
  else  spaceErb = 0.0;

  /* Smoothed envelope */
//  senv = (double*) mxCalloc(nsamples_padded, sizeof(double));
  if (!initFlag) {
	  initFlag=1;
	  for (chan=0;chan<numchans;chan++) {
		senv[chan]=0.;
		p1rs[chan]=0.0; p2rs[chan]=0.0; p3rs[chan]=0.0; p4rs[chan]=0.0;
		p1is[chan]=0.0; p2is[chan]=0.0; p3is[chan]=0.0; p4is[chan]=0.0;
		css[chan]=1.0;
		sns[chan]=0.0;
	  }
  }
  //senv = new double[nsamples_padded];

  tpt = 2 * M_PI / fs;
  intdecay = exp(-(1000.0/(fs*ti)));
  intgain = 1-intdecay;

  for (chan=0; chan<numchans; chan++)
  {
    cf = ErbRateToHz(lowErb+chan*spaceErb);
	cfarray[chan] = cf;
    tptbw = tpt * erb(cf) * BW_CORRECTION;
    a = exp(-tptbw);
    gain = (tptbw*tptbw*tptbw*tptbw)/3;
	subFrameCount= frameCount=0;
	sumEnv=0.;

    /* Update filter coefficiants */
    a1 = 4.0*a; a2 = -6.0*a*a; a3 = 4.0*a*a*a; a4 = -a*a*a*a; a5 = 4.0*a*a;

//    senv1=0.0;

		// Restore channel history 
		p1r=p1rs[chan]; p2r=p2rs[chan]; p3r=p3rs[chan]; p4r=p4rs[chan];
		p1i=p1is[chan]; p2i=p2is[chan]; p3i=p3is[chan]; p4i=p4is[chan];
		senv1 = senv[chan];
		cs=css[chan];
		sn=sns[chan];

//		p1r=0.; p2r=0.; p3r=0.; p4r=0.;
//		p1i=0.; p2i=0.; p3i=0.; p4i=0.;

    /*====================================================================================
     * complex z=x+j*y, exp(z) = exp(x)*(cos(y)+j*sin(y)) = exp(x)*cos(x)+j*exp(x)*sin(y).
     * z = -j * tpti * cf, exp(z) = cos(tpti*cf) - j * sin(tpti*cf)
     *====================================================================================
     */
    coscf = cos(tpt*cf);
    sincf = sin(tpt*cf);
//    cs = 1; sn = 0;
    for (i=0; i<nsamples; i++)
	{      

	// IIR lowpass filters here.. sets the frequency selectivity
	// Must redesign this for finer frequency selectivity
      p0r = cs*x[i] + a1*p1r + a2*p2r + a3*p3r + a4*p4r; 
      p0i = sn*x[i] + a1*p1i + a2*p2i + a3*p3i + a4*p4i;
     
      /* Clip coefficients to stop them from becoming too close to zero */
      if (fabs(p0r) < VERY_SMALL_NUMBER)
        p0r = 0.0F;
      if (fabs(p0i) < VERY_SMALL_NUMBER)
        p0i = 0.0F;
      
      u0r = p0r + a1*p1r + a5*p2r;  // detector output, an FIR filter convolution of the running filtered signal.
      u0i = p0i + a1*p1i + a5*p2i;

      p4r = p3r; p3r = p2r; p2r = p1r; p1r = p0r;
      p4i = p3i; p3i = p2i; p2i = p1i; p1i = p0i;

     /*==========================================
      * Smoothed envelope by temporal integration 
      *==========================================
      */
      //senv1 = senv[i] = sqrt(u0r*u0r+u0i*u0i) * gain + intdecay*senv1;
	  senv1= sqrt(u0r*u0r+u0i*u0i) * /*gain*/ intgain + intdecay*senv1;

      sumEnv += senv1;

	  if (++subFrameCount == frameshift_samples) {
		  subFrameCount = 0;
		  ratemap[chan+frameCount*numchans] = /*intgain*/ gain * sumEnv / frameshift_samples;
		  sumEnv = 0.;
		  frameCount++;
	  }

     /*=========================================
      * cs = cos(tpt*i*cf);
      * sn = -sin(tpt*i*cf);
      *=========================================
      */
      cs = (oldcs=cs)*coscf + (oldsn=sn)*sincf;
      sn = oldsn*coscf - oldcs*sincf;
	  if (cs > 1.0) cs = 1.0;
	  if (sn > 1.0) sn = 1.0;
    } // end of samples loop

	// Save channel history
	p1rs[chan]=p1r; p2rs[chan]=p2r; p3rs[chan]=p3r; p4rs[chan]=p4r;
	p1is[chan]=p1i; p2is[chan]=p2i; p3is[chan]=p3i; p4is[chan]=p4i;
	senv[chan] = senv1;
	css[chan]=cs;
	sns[chan]=sn;

#if 0
    for (i=nsamples; i<nsamples_padded; i++)
    {
      p0r = a1*p1r + a2*p2r + a3*p3r + a4*p4r;
      p0i = a1*p1i + a2*p2i + a3*p3i + a4*p4i;

      u0r = p0r + a1*p1r + a5*p2r;
      u0i = p0i + a1*p1i + a5*p2i;

      p4r = p3r; p3r = p2r; p2r = p1r; p1r = p0r;
      p4i = p3i; p3i = p2i; p2i = p1i; p1i = p0i;

     /*==========================================
      * Envelope
      *==========================================
      * env0 = sqrt(u0r*u0r+u0i*u0i) * gain;
      */

     /*==========================================
      * Smoothed envelope by temporal integration 
      *==========================================
      */
      senv1 = senv[i] = sqrt(u0r*u0r+u0i*u0i) * gain + intdecay*senv1;
    }
#endif
    
   /*==================================================================================
    * we take the mean of the smoothed envelope as the energy value in each frame
    * rather than simply sampling it.
    * ratemap(c,:) = intgain.*mean(reshape(smoothed_env,frameshift_samples,numframes));
    *==================================================================================
    */
#if 0
    for (j=0; j<numframes; j++)
    {
      sumEnv = 0.0;
      for (i=j*frameshift_samples; i<(j+1)*frameshift_samples; i++)
      {
        sumEnv += senv[i];
      }
      ratemap[chan+j*numchans] = intgain * sumEnv / frameshift_samples;
    }
#endif
  } // end of channels loop


  if (compression_type==1)
  {
    for (i=0; i<numchans; i++)
		for (j=0;j<numframes;j++)
      ratemap[i+j*numchans] = pow(ratemap[i+j*numchans], 0.3);
  }
  else if (compression_type==2)
  {
    for (i=0; i<numchans; i++)
		for(j=0;j<numframes;j++)
	{
      if (ratemap[i+j*numchans] > VERY_SMALL_NUMBER) ratemap[i+j*numchans] = log10(ratemap[i+j*numchans]);
      else ratemap[i+j*numchans] = LOG10_VERY_SMALL_NUMBER;
    }
  }

//  mxFree(senv);
//  delete(senv);
  return;
}
