// syncDecode.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <Windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

extern  double hilbertKernel128[];
extern  double hilbertKernel64[];
extern  double LPFKernel64[];
extern  double BP500Kernel64[];


extern double pi;
//double pi = 3.141592653589793238462643383;

double complexLog(double re, double im)
{
	double imabs = fabs(im);
	double reabs = fabs(re);

	if ((re >= 0)&&(re >= imabs))
			return atan(im / re);
	else {
		if (im >= 0.) {
			if (im > reabs)
				return pi / 2 - atan(re / im);
			else
				return pi + atan(im / re);
		}
		else { // im < 0
			if (im < -reabs)
				return -pi / 2 + atan(re / -im);
			else
				return -pi + atan(im / re);
		}
	}
}

double complexLogApprox(double mr, double mi)
{
	double err;
	double aplen = fabs(mr) + fabs(mi);
	if (mr > 0.) {  // 1st and 4th quadrants
		err = 0.5*mi / aplen;
	}
	else
		if (mi > 0) // 2nd quadrant
			err =  1 - 0.5*mi / aplen;
		else        // 3rd quadrant
			err = -1 - 0.5*mi / aplen;

	return pi*err;
}

struct firstate {
	int nsize, niter;
	double *wp;
	double *memory;

};

struct firstate HilbertFilterState;
struct firstate lowpassState;
struct firstate BPState;

void fir_init(struct firstate *state, int n)
{
	state->nsize = n;
	state->niter = 0;
	
	state->memory = (double *)malloc(n * 2 * sizeof(double));
	memset(state->memory, 0, n * 2 * sizeof(double));
	state->wp = state->memory + (n-1); // point to last position
}

void fir_free(struct firstate *state)
{
	if (state->memory)
		free(state->memory);
}

double fir(double data, double *kernel, struct firstate* state)
{
	double acc = 0.;
	int i;
	double *rp = state->wp;
	*state->wp = data;
	*(state->wp + state->nsize) = data;

	for (i = 0; i < state->nsize; i+=4){
		acc += *rp++ * *kernel++;
		acc += *rp++ * *kernel++;
		acc += *rp++ * *kernel++;
		acc += *rp++ * *kernel++;
	}

	if (--state->wp < state->memory){
		state->wp = state->memory + (state->nsize-1);
	}

	return acc;
}

// Special purpose fir filter execution, assuming the kernel is a hilbert
// kernel (even coefficients are zero).
double fir_hilbert(double data, double *kernel, struct firstate* state)
{
	double acc = 0.;
	int i;
	double *rp = state->wp;
	*state->wp = data;
	*(state->wp + state->nsize) = data;
	kernel++; rp++; // skip the first zero coefficient

	for (i = 0; i < state->nsize; i += 8){
		acc += *rp * *kernel;
		rp += 2; kernel += 2;
		acc += *rp * *kernel;
		rp += 2; kernel += 2;
		acc += *rp * *kernel;
		rp += 2; kernel += 2;
		acc += *rp * *kernel;
		rp += 2; kernel += 2;
	}

	if (--state->wp < state->memory){
		state->wp = state->memory + (state->nsize - 1);
	}

	return acc;
}
struct zstate {
	double *memory;
	double *ptr;
};

double zdelay(double data, struct zstate *state, int dly)
{
	double retval;

	if (!state->memory){
		state->memory = (double *)malloc(dly*sizeof(double));
		memset(state->memory, 0, dly*sizeof(double));
		state->ptr = state->memory + (dly - 1);
	}

	retval = *state->ptr;
	*state->ptr = data;
	
	if (--state->ptr < state->memory)
		state->ptr = state->memory + dly - 1;

	return retval;
}


int _tmaintest(int argc, _TCHAR* argv[])
{
	double th, re, im, thback,thapprox;
	int i,j;
	double tscale;
	double accum;
	LARGE_INTEGER t, t1;
	int runs = 100000;

	QueryPerformanceFrequency(&t1);
	printf("System sez QPF = %d\n", t1.LowPart);
	tscale = 1. / t1.LowPart;
	tscale = tscale * 1E6;

	accum = 0;
	th = pi / 3.5;
	re = cos(th);
	for (j = 0; j < runs; j++) {
		QueryPerformanceCounter(&t1);
		re = cos(th);
		QueryPerformanceCounter(&t);
		t1.QuadPart = t.QuadPart - t1.QuadPart;
		i = t1.LowPart;
		accum += i;
	}
	printf("Cos(x) took %g usec.\n", accum*tscale/runs);

	im = sin(pi / 3.5);

	accum = 0;
	th = complexLog(re, im);
	for (j = 0; j < runs; j++) {
		QueryPerformanceCounter(&t1);
		th = complexLog(re, im);
		QueryPerformanceCounter(&t);
		t1.QuadPart = t.QuadPart - t1.QuadPart;
		i = t1.LowPart;
		accum += i;
	}
	printf("ComplexLog took %g usec.\n", accum*tscale/runs);

	accum = 0;
	th = complexLogApprox(re, im);
	for (j = 0; j < runs; j++) {
		QueryPerformanceCounter(&t1);
		th = complexLogApprox(re, im);
		QueryPerformanceCounter(&t);
		t1.QuadPart = t.QuadPart - t1.QuadPart;
		i = t1.LowPart;
		accum += i;
	}

	QueryPerformanceCounter(&t1);
	printf("ComplexLogApprox took %g usec.\n", accum*tscale/runs);
	QueryPerformanceCounter(&t);
	t1.QuadPart = t.QuadPart - t1.QuadPart;
	i = t1.LowPart;
	printf("printf took %g usec.\n", i*tscale);

	QueryPerformanceCounter(&t1);
	QueryPerformanceCounter(&t);
	t1.QuadPart = t.QuadPart - t1.QuadPart;
	i = t1.LowPart;
	printf("QueryPerformanceCounter itself took %g usec.\n", i*tscale);

	QueryPerformanceCounter(&t1);
	Sleep(100);
	QueryPerformanceCounter(&t);
	t1.QuadPart = t.QuadPart - t1.QuadPart;
	i = t1.LowPart;
	printf("Sleep(100) took %g usec. which implies the QPC is about %g Hz\n", i*tscale,(double)i/.1);


	//for (i = 0; i < 360; i++) {
	//	th = i * 2 * pi / 360 - pi;
	//	re = cos(th);
	//	im = sin(th);
	//	thback = complexLog(re, im);
	//	thapprox = complexLogApprox(re, im);
	//	printf("%d,%g,%g,%g,%g,%g\n", i-180, th, re, im, thback,thapprox);
	//}

	return 0;
}


int _tmain_syncdecode(int argc, _TCHAR* argv[])
{
	double th1,th2,w1,w2,wb1,wb2,dw1,dw2;
	double x1, y1, x2, y2;
	double sx;
	double mr, mi;
	static double mrState=0.,miState=0.; //modulation product
	double err,errint,errdiff,lasterr;
	double sr = 8000.;
	double dt = 1 / sr;
	double tscale;
	double noiseAmp = 0.;
	double noiseSig,noiseEnergy=0.;
	double sigEnergy=0.,totalEnergy=0.;
	int i,j;
	int initflag = 1;
	double Cpro, Cint, Cdiff,control,gamma;
	double demodFilterCoeff;
	double twopi = 2 * pi;
	struct zstate zmem1, zmem2;
	LARGE_INTEGER t, t1;

	QueryPerformanceFrequency(&t1);
//	printf("System sez QPF = %d\n", t1.LowPart);
	tscale = 1E6*(1. / t1.LowPart);

	zmem1.memory = 0;
	zmem2.memory = 0;

	if (argc != 7) {
		fprintf(stderr, "Not enough parameters!\n");
		exit(-1);
	}
	swscanf(argv[1], L"%lg", &Cpro);
	swscanf(argv[2], L"%lg", &Cint);
	swscanf(argv[3], L"%lg", &Cdiff);
	swscanf(argv[4], L"%lg", &gamma);
	swscanf(argv[5], L"%lg", &noiseAmp);
	swscanf(argv[6], L"%lg", &demodFilterCoeff);


	fprintf(stderr,"Coefficients: Cpro=%lg, Cint=%lg, Cdiff=%lg,\n \
		gamma=%lg, noiseAmp=%lg, demodFilterTC=%lg\n", Cpro, Cint, Cdiff, gamma, noiseAmp, demodFilterCoeff);

	demodFilterCoeff = exp(-dt / demodFilterCoeff);
	
	fir_init(&HilbertFilterState, 64);
	fir_init(&lowpassState, 64);
	fir_init(&BPState, 64);

	errint = 0.;
	lasterr = 0.;

	th1 = 0.;
	th2 = pi / 4;
	wb1 = 500.;
	dw1 = 0;
	wb2 = 502.;
	dw2 = 0.;

	for (i = 0; i < 30000; i++) {

//		w1 += dw1;
		w1 = wb1 + dw1;
//		w2 += dw2;
		w2 = wb2 + dw2;

		th1 += w1 * 2*pi *dt;
		th2 += w2 * 2*pi *dt;

		if (th1 > twopi) th1 -= twopi;
		if (th2 > twopi) th2 -= twopi;

		// The signal
		x1 = cos(th1);
		sigEnergy += x1*x1;

		noiseSig = noiseAmp*(double)rand()/ 32768.;
		noiseEnergy += noiseSig*noiseSig;

		x1 += noiseSig;


		x1 = fir(x1, BP500Kernel64, &BPState);
		totalEnergy += x1*x1;

//		sx = sin(th1);  // for correctness checking
		y1 = fir_hilbert(x1, hilbertKernel64, &HilbertFilterState);
		x1 = zdelay(x1, &zmem1, 32); //compensating delay to hilbert filter
//		sx = zdelay(sx, &zmem2, 32);


		//if (i == 200){
		//	QueryPerformanceCounter(&t1);
		//	for (j = 0; j < 1024; j++)
		//		y1 = fir_hilbert(x1, hilbertKernel64, &HilbertFilterState);
		//	QueryPerformanceCounter(&t);
		//	t.QuadPart = t.QuadPart - t1.QuadPart;
		//	fprintf(stderr, "fir_hilbert took %g us\n", t.LowPart*tscale/1024);

		//	y1 = fir(x1, hilbertKernel64, &HilbertFilterState);
		//	QueryPerformanceCounter(&t1);
		//	for (j = 0; j < 1024; j++)
		//		y1 = fir(x1, hilbertKernel64, &HilbertFilterState);
		//	QueryPerformanceCounter(&t);
		//	t.QuadPart = t.QuadPart - t1.QuadPart;
		//	fprintf(stderr, "fir took %g us\n", t.LowPart*tscale/1024);
		//}
		//else
			
		//local oscillator
		x2 = cos(th2);
		y2 = -sin(th2); // complex conjugate

		mr = x1*x2 - y1*y2;
		mi = x1*y2 + x2*y1;

		mr = mrState = demodFilterCoeff*mrState + (1-demodFilterCoeff)*mr;
		mi = miState = demodFilterCoeff*miState + (1-demodFilterCoeff)*mi;

		if (i < 128) continue; // skip further processing until the Hilbert filter initializes


		//if (i == 200) {
		//	QueryPerformanceCounter(&t1);
		//	for (j = 0; j < 1024; j++)
		//		err = complexLog(mr, mi);
		//	QueryPerformanceCounter(&t);
		//	t.QuadPart = t.QuadPart - t1.QuadPart;
		//	fprintf(stderr, "complexLog took %g us\n", t.LowPart*tscale / 1024);
		//}
		
		err = complexLog(mr, mi);

		if (initflag) {
			initflag = 0;
			lasterr=err;
		}

		errint += err;
		errdiff = err - lasterr;
		lasterr = err;

		control = Cpro*err + Cint*errint + Cdiff*errdiff;

		dw2 = gamma * control;

//		printf("%lg, %lg, %lg\n", x1, y1, sx);
		printf("step %d, %8lg, %8lg, %8lg, %8lg, %8lg, %8lg\n", i, w2, err*180/pi, errint*180/pi, errdiff*180/pi, control,x1);


	}

	totalEnergy = sqrt(totalEnergy);
	sigEnergy = sqrt(sigEnergy);
	noiseEnergy = totalEnergy - sigEnergy;
	fprintf(stderr,"Total energy = %lg, Signal energy = %lg, Noise = %lg\n", 
		(totalEnergy), (sigEnergy), (noiseEnergy));
	fprintf(stderr, "SNR = %lgdB\n", 20 * log10(sqrt(sigEnergy) / noiseEnergy));

	fir_free(&HilbertFilterState);
	free(zmem1.memory);
	free(zmem2.memory);
	fir_free(&lowpassState);
	return 0;
}

