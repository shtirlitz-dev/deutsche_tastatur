#include "stdafx.h"
#include "shellapi.h"
#include "deutsch.h"


HINSTANCE gl_hInstance;
TCHAR     gl_WindowClass[]=_T("deutsch");
TCHAR     gl_Title[]=_T("Deutsche Tastatur");

NOTIFYICONDATA NotifyData; // for sys tray
const UINT WM_NOTIFYICON=WM_USER+2;


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KB_HOOK));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= gl_WindowClass;
	wcex.hIconSm		= wcex.hIcon;

	return RegisterClassEx(&wcex);
}


HMODULE gl_kb_hook=0;
BOOL (*InstallHook)(HWND)=0;
BOOL (*UninstallHook)()=0;

bool InitHookDll()
{
	gl_kb_hook=LoadLibrary(L"deutsch_hook.dll");
	if(!gl_kb_hook)
		return false;
	(FARPROC&)   InstallHook=GetProcAddress(gl_kb_hook, "InstallHook");
	(FARPROC&) UninstallHook=GetProcAddress(gl_kb_hook, "UninstallHook");
	
	return !!InstallHook && !!UninstallHook;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
					   HINSTANCE hPrevInstance,
					   LPTSTR    lpCmdLine,
					   int       nCmdShow)
{
	gl_hInstance = hInstance;
	if(!InitHookDll())
		return MessageBox(0, _T("Error: Failed to load 'deutsch_hook.dll'"), gl_Title, MB_ICONSTOP);

	MyRegisterClass(hInstance);

	const int width=300, height=150;

	int cx=GetSystemMetrics(SM_CXSCREEN);
	int cy=GetSystemMetrics(SM_CYSCREEN);
	HWND hWnd=CreateWindow(gl_WindowClass, gl_Title, (WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX|WS_THICKFRAME) ),
		(cx-width)/2, (cy-height)/2, width, height, NULL, NULL, gl_hInstance, NULL);

	if (!hWnd)
		return MessageBox(0, _T("Error: Failed to create main window"), gl_Title, MB_ICONSTOP);

	InstallHook(hWnd);

	ShowWindow(hWnd, SW_HIDE);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	UninstallHook();

	return (int) msg.wParam;
}

void OnCreate(HWND hWnd)
{
	NotifyData.cbSize=sizeof(NOTIFYICONDATA); 
	NotifyData.hWnd=hWnd; 
	NotifyData.uID=1968;
	NotifyData.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP; 
	NotifyData.uCallbackMessage=WM_NOTIFYICON; 
	NotifyData.hIcon=LoadIcon(gl_hInstance, MAKEINTRESOURCE(IDI_KB_HOOK));
	::GetWindowText(hWnd, NotifyData.szTip, sizeof(NotifyData.szTip)); 
	Shell_NotifyIcon(NIM_ADD, &NotifyData);
}

LRESULT OnTrayNotification(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
		{
			static bool dlgshown = false;
			if(dlgshown)
				return TRUE;
			dlgshown = true;
			TCHAR bfr[1000];
			LoadString(gl_hInstance, IDS_ABOUT, bfr, sizeof(bfr)/sizeof(bfr[0]));
			int res = MessageBox(0, bfr, gl_Title, MB_YESNO|MB_ICONASTERISK);
			dlgshown = false;
			if(res == IDNO)
				SendMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
		break;
		}
	}
	return TRUE;
}


void PerformMultimediaCommand( HWND hWnd, LPARAM lParam );

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &NotifyData);
		PostQuitMessage(0);
		return 0;
	case WM_CREATE:
		OnCreate(hWnd);
		break;

	case WM_NOTIFYICON:
		return OnTrayNotification(hWnd, wParam, lParam);
	case WM_SYSCOMMAND:
		if(wParam==SC_MINIMIZE)
		{
			ShowWindow(hWnd, SW_HIDE);
			return 0;
		}
		break;
	case WM_USER+1:
		PerformMultimediaCommand( hWnd, lParam );
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void PerformMultimediaCommand( HWND hWnd, LPARAM lParam )
{
	HWND send_hWnd = HWND_BROADCAST;
	switch( lParam )
	{
	case APPCOMMAND_VOLUME_DOWN:
	case APPCOMMAND_VOLUME_UP:
	case APPCOMMAND_VOLUME_MUTE:
		send_hWnd = hWnd;
		break;
	}
	::SendMessage(send_hWnd, WM_APPCOMMAND, 0, MAKELPARAM(0, lParam ) );
}
