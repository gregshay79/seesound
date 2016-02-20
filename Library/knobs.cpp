
#include "stdafx.h"

#include <windows.h>
#include <math.h>
#include "knobs.h"

#undef UNICODE

WNDPROC original_procedure;
bool toggle = false;

#if OLDCODE
void draw_on_hButton(HDC hDC, WCHAR *image = L"D:/proj/library/pictures/GregPicMedium.bmp" ){
	//Graphics gx(hdc);
	//gx.SetTextRenderingHint(TextRenderingHintAntiAlias);
	//gx.Clear(Color(20, 20, 20)); //clear it with the same color,to avoid any default hButton1 drawing
	//Image img(image);
	//gx.DrawImage(&img, 0, 0);
	int nWidth = 100, nHeight = 100;
	HGDIOBJ oldobj;

	HDC memDC = CreateCompatibleDC(hDC);
//	HBITMAP memBM = CreateCompatibleBitmap(hDC, nWidth, nHeight);

	HBITMAP hBMP = (HBITMAP)LoadImageW(NULL, image, IMAGE_BITMAP, 100, 100, LR_LOADFROMFILE);
	oldobj = SelectObject(memDC, hBMP);
	BitBlt(hDC, 0, 0, 100, 100, memDC, 0, 0, SRCCOPY);
	
	SelectObject(memDC, oldobj);
	DeleteObject(hBMP);
	ReleaseDC(hwnd,memDC);
}
#endif
#if OLDCODE
LRESULT CALLBACK ButtonProc(HWND ws, UINT Message, WPARAM wParam, LPARAM lParam){
	PAINTSTRUCT ps;
	HDC hdc;

	switch (Message){
	case WM_PAINT:
		if (ws == hButton1){
			hdc = BeginPaint(ws, &ps);
			draw_on_hButton(hdc);
			EndPaint(ws, &ps);
		}
		break;
	case WM_MOUSEMOVE:
		if (toggle == false){
			TRACKMOUSEEVENT event_;
			event_.cbSize = sizeof(TRACKMOUSEEVENT);
			event_.dwFlags = TME_LEAVE | TME_HOVER;
			event_.hwndTrack = hButton1;
			event_.dwHoverTime = 10;
			TrackMouseEvent(&event_);
			toggle = true;
		}
		break;
	case WM_MOUSELEAVE:
		//revert to initial image
		if (toggle){
			toggle = false;
			//update
			hdc = GetDC(hButton1);
			draw_on_hButton(hdc);
			ReleaseDC(hButton1, hdc);
		}
		break;
	case WM_MOUSEHOVER:
		//change to hover image
		if (toggle){
			hdc = GetDC(hButton1);
			draw_on_hButton(hdc, L"D:/proj/library/pictures/eMG.bmp");
			ReleaseDC(hButton1, hdc);
			Beep(500, 200);
		}
		break;
	}
	return CallWindowProc(original_procedure1, ws, Message, wParam, lParam);
}
#endif

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
			//		if (ws == hButton1){
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
			//if (toggle == false){
			//	TRACKMOUSEEVENT event_;
			//	event_.cbSize = sizeof(TRACKMOUSEEVENT);
			//	event_.dwFlags = TME_LEAVE | TME_HOVER;
			//	event_.hwndTrack = ws;
			//	event_.dwHoverTime = 3;
			//	TrackMouseEvent(&event_);
			//	toggle = true;
			//}
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

			//case WM_MOUSELEAVE:
			//	//revert to initial image
			//	if (toggle){
			//		toggle = false;
			//		//update
			//		hdc = GetDC(ws);
			//		draw_on_hButton(hdc);
			//		ReleaseDC(ws, hdc);
			//	}
			//	break;
			//case WM_MOUSEHOVER:
			//	//change to hover image
			//	if (toggle){
			//		hdc = GetDC(ws);
			//		draw_on_hButton(hdc, L"D:/proj/library/pictures/eMG.bmp");
			//		ReleaseDC(ws, hdc);
			//		Beep(500, 200);
			//	}
			//	break;
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

#if OLDCODE
LRESULT CALLBACK ButtonProc(HWND ws, UINT Message, WPARAM wParam, LPARAM lParam){
	PAINTSTRUCT ps;
	HDC hdc;
	static POINT anchor,cp;
	static bool captured = false;
	int dx, dy, distance,trigger=0;
	TCHAR msgstr[8];

	switch (Message){
	case WM_PAINT:
//		if (ws == hButton1){
			hdc = BeginPaint(ws, &ps);
			draw_on_hButton(hdc);
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
		//if (toggle == false){
		//	TRACKMOUSEEVENT event_;
		//	event_.cbSize = sizeof(TRACKMOUSEEVENT);
		//	event_.dwFlags = TME_LEAVE | TME_HOVER;
		//	event_.hwndTrack = ws;
		//	event_.dwHoverTime = 3;
		//	TrackMouseEvent(&event_);
		//	toggle = true;
		//}
		if (captured) {
			cp.x = (short int)(LOWORD(lParam));
			cp.y = (short int)(HIWORD(lParam));
			dx = cp.x - anchor.x;
			dy = cp.y - anchor.y;
			distance = dx - dy;
			wsprintf(msgstr, L"%d", distance);
			hdc = GetDC(ws);
			BitBlt(hdc, 0, 0, 32, 16, 0, 0, 0, WHITENESS);
			TextOut(hdc, 0, 0, msgstr, lstrlenW(msgstr));
			ReleaseDC(ws, hdc);
		}
		break;

	//case WM_MOUSELEAVE:
	//	//revert to initial image
	//	if (toggle){
	//		toggle = false;
	//		//update
	//		hdc = GetDC(ws);
	//		draw_on_hButton(hdc);
	//		ReleaseDC(ws, hdc);
	//	}
	//	break;
	//case WM_MOUSEHOVER:
	//	//change to hover image
	//	if (toggle){
	//		hdc = GetDC(ws);
	//		draw_on_hButton(hdc, L"D:/proj/library/pictures/eMG.bmp");
	//		ReleaseDC(ws, hdc);
	//		Beep(500, 200);
	//	}
	//	break;
	}
	return CallWindowProc(original_procedure1, ws, Message, wParam, lParam);
}
#endif

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
