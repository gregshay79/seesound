
#include "stdafx.h"

#include <windows.h>
#include <math.h>
#include "knobs.h"

#undef UNICODE

LRESULT CALLBACK KnobProc(HWND ws, UINT Message, WPARAM wParam, LPARAM lParam);

WNDPROC original_procedure;
bool toggle = false;

struct knob knobs[MAX_KNOB];

struct button buttons[MAX_BUTTON];

double linmap(int v, int min, int max)
{
	double val = ((double)(v)-min) / (max - min);
	return val;
}

double passthuMap(int v, int min, int max)
{
	return (double)v;
}

double normMap(int v, int min, int max)
{
	double val = ((double)(v)-min) / (max - min);
	return val;
}

double norm4Map(int v, int min, int max)
{
	double val = 4.*((double)(v)-min) / (max - min);
	return val;
}

double norm40Map(int v, int min, int max)
{
	double val = 40.*((double)(v)-min) / (max - min);
	return val;
}

double milMap(int v, int min, int max)
{
	double val = 4.*((double)(v)-min) / (max - min);
	return val/1000.;
}

TCHAR vstr[16];

double tcmapblockSR(int v, int min, int max)
{
	// create first order filter coefficient for 0 to 100ms
	
	double tau;// = v * 1E-3;

	tau = 5.*normMap(v, min, max); // normalize to 0 to 5 seconds
	return exp(-1 / (tau*samplerate/block_size));
}

double tcmapfullSR(int v, int min, int max)
{
	// create first order filter coefficient for 0 to 100ms

	double tau;// = v * 1E-3;

	tau = normMap(v, min, max); // normalize to 0 to 1 second
	return exp(-1 / (tau*samplerate));
}

LPWSTR tcBlockSRDispMap(double val)
{
	double tc;
	tc = (-1.0 / log(val)) / (samplerate/block_size);
	swprintf_s(vstr, L"%3.1fms", tc*1000);
	return vstr;
}

LPWSTR tcFullSRDispMap(double val)
{
	double tc;
	tc = (-1.0 / log(val)) / (samplerate);
	swprintf_s(vstr, L"%3.1fms", tc * 1000);
	return vstr;
}

LPWSTR dBmap(double val)
{
	if (val < 1E-10) {  // covers zero and negatives
		lstrcpyW(vstr, L"-inf");
		return vstr;
	}
	swprintf_s(vstr, L"%3.1fdB", 20 * log10(val));
	return vstr;
}

void makeKnobDisplay(struct knob *pK)
{
	if (pK->dxform)
		swprintf_s(pK->dispStr, L"%s %s", pK->name, pK->dxform(pK->value));
	else
		swprintf_s(pK->dispStr, L"%s %5.4lf", pK->name, pK->value);
}


HWND createKnob(HWND hwnd, struct knob *pK, TCHAR *name, int x, int y, 
				double(*fcn)(int, int, int), LPWSTR(*dispMap)(double val), int min, int max, int initIval)
{

	HINSTANCE inst = (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE);

	pK->pos.left = x;
	pK->pos.right = x + KNOBSIZEX;
	pK->pos.top = y;
	pK->pos.bottom = y + KNOBSIZEY;
	lstrcpyW(pK->name, name);
	pK->xform = fcn;
	pK->dxform = dispMap;
	pK->ival = initIval;
	pK->min = min;
	pK->max = max;
	pK->value = fcn(initIval,min,max);
//	swprintf_s(pK->dispStr, L"%s %5.4lf", pK->name, pK->value);
	makeKnobDisplay(pK);


	// create window here
	pK->hwnd = CreateWindowW(L"Button", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, x, y, KNOBSIZEX, KNOBSIZEY, hwnd, 0, inst, 0);
	//subclassing
	original_procedure = (WNDPROC)SetWindowLong(pK->hwnd, GWL_WNDPROC, (long)KnobProc);
//	UpdateWindow(pK->hwnd);
//	RedrawWindow(pK->hwnd, NULL, NULL, RDW_INVALIDATE);
	return pK->hwnd;
}

void initKnobs()
{
	memset(knobs, 0, MAX_KNOB*sizeof(struct knob));
}

void destroyKnobs()
{
	int i;
	for (i = 0; i < MAX_KNOB; i++){
		if (knobs[i].hwnd)
			DestroyWindow(knobs[i].hwnd);
	}
	for (i = 0; i < MAX_BUTTON; i++){
		if (buttons[i].hwnd)
			DestroyWindow(buttons[i].hwnd);
	}
}

LRESULT CALLBACK KnobProc(HWND ws, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	static POINT anchor, cp;
	static bool captured = false;
	int ix;
	int ival;
	int dx, dy, distance, trigger = 0;
//	TCHAR msgstr[16];
	for (ix = 0; ix < MAX_KNOB; ix++) {
		if ((knobs[ix].hwnd == ws) || (knobs[ix].hwnd == NULL))
			break;
	}

	if (knobs[ix].hwnd) {
		switch (Message){

		case WM_PAINT:
			hdc = BeginPaint(ws, &ps);
			BitBlt(hdc, 0, 0, KNOBSIZEX, KNOBSIZEY, 0, 0, 0, WHITENESS);
			TextOut(hdc, 0, 0, knobs[ix].dispStr, lstrlenW(knobs[ix].dispStr));
			EndPaint(ws, &ps);
			//		}
			break;

		case WM_LBUTTONDOWN:
			anchor.x = LOWORD(lParam);
			anchor.y = HIWORD(lParam);
			SetCapture(ws);
			captured = true;
			break;

		case WM_LBUTTONUP:
			if (captured) {
				ReleaseCapture();
				captured = false;
			}
			break;

		case WM_MOUSEMOVE:
			if (captured) {
				cp.x = (short int)(LOWORD(lParam));
				cp.y = (short int)(HIWORD(lParam));
				dx = cp.x - anchor.x;
				dy = cp.y - anchor.y;
				distance = dx - dy;
				anchor = cp;
				ival = knobs[ix].ival + distance;
				ival = ival < knobs[ix].min ? knobs[ix].min : ival;
				ival = ival > knobs[ix].max ? knobs[ix].max : ival;
				knobs[ix].ival = ival;
				knobs[ix].value = knobs[ix].xform(ival, knobs[ix].min, knobs[ix].max);
				makeKnobDisplay(&knobs[ix]);
				//				swprintf_s(knobs[ix].dispStr, L"%s %5.4lf", knobs[ix].name, knobs[ix].value);
				hdc = GetDC(ws);
				BitBlt(hdc, 0, 0, KNOBSIZEX, KNOBSIZEY, 0, 0, 0, WHITENESS);
				TextOut(hdc, 0, 0, knobs[ix].dispStr, lstrlenW(knobs[ix].dispStr));
				ReleaseDC(ws, hdc);
			}
			break;
		}
	}
	return CallWindowProc(original_procedure, ws, Message, wParam, lParam);
}


// Button Proc takes action depending on type of button
// type = 0, momentary value = 0, push value = 1
// type = 1, multi value click through values 0,1,2..n with n string names
//   Note: PON/POFF value = 0 or 1 toggles, is a degenerate case of type 1, nValues = 1
LRESULT CALLBACK ButtonProc(HWND ws, UINT Message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	static POINT anchor, cp;
	static bool captured = false;
	struct button *pB;
	int ix;
	int ival;
	int dx, dy, distance, trigger = 0;
	//	TCHAR msgstr[16];
	for (ix = 0; ix < MAX_KNOB; ix++) {
		if ((buttons[ix].hwnd == ws) || (buttons[ix].hwnd == NULL))
			break;
	}

	if (buttons[ix].hwnd) {
		pB = &buttons[ix];

		switch (Message){

		case WM_PAINT:
			hdc = BeginPaint(ws, &ps);
			if ((pB->type == 0) && (pB->value == 1)) {
				BitBlt(hdc, 0, 0, BUTTONSIZEX, BUTTONSIZEY, 0, 0, 0, BLACKNESS);
			} else
				BitBlt(hdc, 0, 0, BUTTONSIZEX, BUTTONSIZEY, 0, 0, 0, WHITENESS);
			TextOut(hdc, 0, 0, pB->name[pB->value], lstrlenW(pB->name[pB->value]));
			EndPaint(ws, &ps);
			break;

		case WM_LBUTTONDOWN:
			if (pB->value + 1 >= pB->nValues)
				pB->value = 0;
			else
				pB->value++;
			RedrawWindow(pB->hwnd, NULL, NULL, RDW_INVALIDATE);

			break;

		case WM_LBUTTONUP:
			if (pB->type == BUTTON_TYPE_MOMENTARY) {
				pB->value = 0;
				RedrawWindow(pB->hwnd, NULL, NULL, RDW_INVALIDATE);
			}
			break;
		}
	}
	return CallWindowProc(original_procedure, ws, Message, wParam, lParam);
}

HWND createButton(HWND hwnd, int x, int y, struct button *pB, TCHAR **name, int type, int nValues)
{
	int i;
	HINSTANCE instance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);

	pB->pos.left = x;
	pB->pos.right = x + BUTTONSIZEX;
	pB->pos.top = y;
	pB->pos.bottom = y + BUTTONSIZEY;

	pB->nValues = nValues;
	pB->type = type;
	pB->value = 0;
	
	for (i = 0; i <= nValues; i++) {
		lstrcpy(pB->name[i], name[i]);
	}

	// create window here
	pB->hwnd = CreateWindowW(L"Button", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, x, y, BUTTONSIZEX, BUTTONSIZEY, hwnd, 0, instance, 0);
	//subclassing
	original_procedure = (WNDPROC)SetWindowLong(pB->hwnd, GWL_WNDPROC, (long)ButtonProc);
	return pB->hwnd;
}
