
#include "stdafx.h"

#include <windows.h>
#include <math.h>
#include "knobs.h"

#undef UNICODE

WNDPROC original_procedure;
bool toggle = false;

struct knob knobs[MAX_KNOB];

double linmap(int v, int min, int max)
{
	double val = ((double)(v)-min) / (max - min);
	return val;
}

double tcmap(int v, int min, int max)
{
	// create first order filter coefficient for 0 to 100ms
	double samplerate = 8000.;
	double tau = v * 1E-3;
	return exp(-1 / (samplerate*tau));
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
	TCHAR msgstr[16];
	for (ix = 0; ix < MAX_KNOB; ix++) {
		if ((knobs[ix].hwnd == ws) || (knobs[ix].hwnd==NULL))
			break;
	}

	if (knobs[ix].hwnd) {
		switch (Message){

		case WM_PAINT:
			hdc = BeginPaint(ws, &ps);
			swprintf_s(msgstr, L"%s:%5.4lf", knobs[ix].name, knobs[ix].value);
			BitBlt(hdc, 0, 0, KNOBSIZEX, KNOBSIZEY, 0, 0, 0, WHITENESS);
			TextOut(hdc, 0, 0, msgstr, lstrlenW(msgstr));
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
				swprintf_s(msgstr, L"%s:%5.4lf", knobs[ix].name, knobs[ix].value);
				hdc = GetDC(ws);
				BitBlt(hdc, 0, 0, KNOBSIZEX, KNOBSIZEY, 0, 0, 0, WHITENESS);
				TextOut(hdc, 0, 0, msgstr, lstrlenW(msgstr));
				ReleaseDC(ws, hdc);
			}
			break;
		}
	}
	return CallWindowProc(original_procedure, ws, Message, wParam, lParam);
}

HWND createKnob(HWND hwnd, struct knob *pK, TCHAR *name, int x, int y, double(*fcn)(int, int, int), int initIval, int min, int max)
{

	HINSTANCE inst = (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE);

	pK->pos.left = x;
	pK->pos.right = x + KNOBSIZEX;
	pK->pos.top = y;
	pK->pos.bottom = y + KNOBSIZEY;
	lstrcpyW(pK->name, name);
	pK->xform = fcn;
	pK->ival = initIval;
	pK->min = min;
	pK->max = max;
	pK->value = fcn(initIval,min,max);

	// create window here
	pK->hwnd = CreateWindowW(L"Button", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, x, y, KNOBSIZEX, KNOBSIZEY, hwnd, 0, inst, 0);
	//subclassing
	original_procedure = (WNDPROC)SetWindowLong(pK->hwnd, GWL_WNDPROC, (long)KnobProc);

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
}
