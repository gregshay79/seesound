// testtrack.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "testtrack.h"
#include <CommCtrl.h>


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND hWndMain;
HWND hTrackWnd, hTrackWnd2;


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;
//	HDC dc;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TESTTRACK, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTTRACK));
	
	//// write something:
	//dc = GetDC(hWndMain);
	//TextOut(dc, 0, 0, L"This is a test", 14);
	//ReleaseDC(hWndMain, dc);

	// try a trackbar from the main window
	hTrackWnd = CreateWindowW(TRACKBAR_CLASS,
		L"Bob",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | /* TBS_AUTOTICKS*/ TBS_NOTICKS | WS_BORDER,
		2,50,
		200,28,
		hWndMain,
		NULL,
		hInst,
		NULL
		);

	//SendMessage(hTrackWnd, TBM_SETBUDDY, 0, 0);
	SendMessage(hTrackWnd, TBM_SETRANGE, 1, MAKELONG(0, 100));
	SendMessage(hTrackWnd, TBM_SETPOS, 1, 50);

	// try a trackbar from the main window
	hTrackWnd2 = CreateWindowW(TRACKBAR_CLASS,
		L"Lewis",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | /* TBS_AUTOTICKS*/ TBS_NOTICKS | WS_BORDER,
		300, 50,
		200, 28,
		hWndMain,
		NULL,
		hInst,
		NULL
		);

	//SendMessage(hTrackWnd, TBM_SETBUDDY, 0, 0);
	SendMessage(hTrackWnd2, TBM_SETRANGE, 1, MAKELONG(0, 200));
	SendMessage(hTrackWnd2, TBM_SETPOS, 1, 50);

	hChildWin = CreateWindowW(BUTTON_C,
		L"Lewis",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | /* TBS_AUTOTICKS*/ TBS_NOTICKS | WS_BORDER,
		300, 50,
		200, 28,
		hWndMain,
		NULL,
		hInst,
		NULL
		);



	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	DestroyWindow(hTrackWnd);
	DestroyWindow(hTrackWnd2);

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTTRACK));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TESTTRACK);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
//   HWND hWndMain;

   hInst = hInstance; // Store instance handle in our global variable

   hWndMain = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWndMain)
   {
      return FALSE;
   }

   ShowWindow(hWndMain, nCmdShow);
   UpdateWindow(hWndMain);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int trackpos;
	WCHAR stxt[32];
	HDC dc;

	switch (message)
	{
	case WM_HSCROLL :
		if ((HWND)lParam == hTrackWnd) {
			trackpos = SendMessage(hTrackWnd, TBM_GETPOS, 0, 0); // Assume immediate return??
			wsprintfW(stxt, L"%3d", trackpos);
			dc = GetDC(hWndMain);
			BitBlt(dc, 205, 55, 24, 16, NULL, 0, 0, WHITENESS);
			TextOutW(dc, 205, 55, stxt, wcslen(stxt));
		}
		else
			if ((HWND)lParam == hTrackWnd2) {
				trackpos = SendMessage(hTrackWnd2, TBM_GETPOS, 0, 0); // Assume immediate return??
				wsprintfW(stxt, L"%3d", trackpos);
				dc = GetDC(hWndMain);
				BitBlt(dc, 505, 55, 24, 16, NULL, 0, 0, WHITENESS);
				TextOutW(dc, 505, 55, stxt, wcslen(stxt));
			}
			else break;

		wsprintfW(stxt, L"0x%08x", lParam);
		TextOutW(dc, 0, 24, stxt, wcslen(stxt));

		ReleaseDC(hWndMain, dc);
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
