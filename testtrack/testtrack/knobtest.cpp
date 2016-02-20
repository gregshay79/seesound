
#include "stdafx.h"

#include <windows.h>
#include <math.h>

#include "../../Library/knobs.h"

#undef UNICODE

//globals
static HINSTANCE instance = NULL;
HWND hwnd=NULL; // , hButton1, hButton2;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static TCHAR gszClassName[] = L"Application";


//WNDPROC original_procedure1, original_procedure2;


int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	WNDCLASSEX WndClass;
	MSG Msg;
	instance = hInstance;

	initKnobs();

	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.style = NULL;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = instance;
	WndClass.hIcon = LoadIcon(hInstance, 0);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = CreateSolidBrush(RGB(20, 20, 20));
	WndClass.lpszMenuName = 0;
	WndClass.lpszClassName = gszClassName;
	WndClass.hIconSm = LoadIcon(hInstance, 0);

	RegisterClassEx(&WndClass);
	hwnd = CreateWindowEx(
		0,
		gszClassName,
		L"hButton Subclassing",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		600, 400,
		NULL, NULL,
		instance,
		NULL);

	ShowWindow(hwnd, 1);
	UpdateWindow(hwnd);

	while (GetMessage(&Msg, NULL, 0, 0)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	PAINTSTRUCT ps;
	int i;

	switch (Message) {
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		break;
	case WM_CREATE:
		createKnob(hwnd, &knobs[0], L"Gain", 10, 10, &linmap, 100, 0, 100);
		createKnob(hwnd, &knobs[1], L"Tc", 100, 10, &tcmap, 50, 0, 100);
		break;

	case WM_COMMAND:
		break;

	case WM_CLOSE:
		destroyKnobs();
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

