// This file is part of the Prject 'Deutsche Tastatur'
// Copyright (c) 2013-2019 Vsevolod Lukyanin
#include "stdafx.h"

HHOOK gl_hkb=0;
extern HINSTANCE gl_hInstance; // dllmain.cpp

HWND gl_hWnd; // main nwhd

int SendCharToInputWindow(WPARAM wParam, DWORD scancode)
{
	HWND exWnd = GetForegroundWindow();
	if(!exWnd)
		return TRUE;

	DWORD exPrID;
	DWORD exThID = GetWindowThreadProcessId(exWnd, &exPrID);
	DWORD ourThID = ::GetCurrentThreadId();
	bool needAttach = ourThID != exThID;

	if(needAttach && !AttachThreadInput(ourThID, exThID, TRUE))
		return TRUE;

	HWND hWnd = GetFocus();
	if(hWnd) {
		// obviously, commented out ways do not work
//		BYTE state[256];
//		BOOL res1 = GetKeyboardState( state );
//		state[VK_MENU] &= 0x7F;
//		state[VK_RMENU] &= 0x7F;
//		BOOL res2 = SetKeyboardState( state );
		//::SendMessage(hWnd, WM_CHAR, wParam, 1|(scancode<<16));
		::PostMessage(hWnd, WM_CHAR, wParam, 1|(scancode<<16));
	}

	if(needAttach)
		AttachThreadInput(ourThID, exThID, FALSE);

	return TRUE;
}

void SendAppCommand( int cmd )
{
	::PostMessage(gl_hWnd, WM_USER+1, 0, cmd );
}

#include <stdio.h>
#include <stdlib.h>

void UnpressAlt()
{
	BYTE keys[256];
	if( GetKeyboardState( keys ) )
	{
		keys[VK_RMENU] &= 0x7F;
		keys[VK_MENU] &= 0x7F;
		SetKeyboardState( keys );
	}
}

static bool IsShift()
{
	return (GetKeyState(VK_SHIFT)<0 || GetKeyState(VK_LSHIFT)<0 || GetKeyState(VK_RSHIFT)<0) ^ (GetKeyState(VK_CAPITAL) & 1);
}

static bool GetCodeMeaning( DWORD code, WCHAR& symbol, int& app_cmd, int& modifier )
{
	switch( code )
	{
		case '1':
			if( IsShift() )
				symbol = L'\xA1'; // espanol up-side-down (!)
			else
				symbol = 185;
			break; // 1
		case '2': symbol = 178; break; // 2
		case '3': symbol = 179; break; // 3
		case '0': symbol = 176; break;
		case 'C': symbol = 169; break; // (c)
		case 'R': symbol = 174; break; // (R)
		case 'E': symbol = 0x20AC; break;
		case 'U': symbol = IsShift()? 220 : 252; break;
		case 'O': symbol = IsShift()? 214 : 246; break;
		case 'A': symbol = IsShift()? 196 : 228; break;
		case 'N': symbol = IsShift()? 209 : 241; break;
		case 'S': symbol = 223; break;
		case 'L': symbol = L'\xA3'; break; // pound

			// multimedia
		case VK_OEM_4: app_cmd = APPCOMMAND_VOLUME_DOWN; break; //   0xDB  //  '[{' for US
		case VK_OEM_6: app_cmd = APPCOMMAND_VOLUME_UP; break; //   0xDD  //  ']}' for US
		case VK_OEM_5: app_cmd = APPCOMMAND_VOLUME_MUTE; break; //   0xDC  //  '\|' for US

		case VK_OEM_2:  //   0xBF   // '/?' for US
			if( IsShift() )
				symbol = L'\xBF'; // espanol up-side-down (?)
			else
				app_cmd = APPCOMMAND_MEDIA_PLAY_PAUSE;
			break;
		case VK_OEM_1: app_cmd = APPCOMMAND_MEDIA_STOP; break; //   0xBA   // ';:' for US
		case VK_OEM_7: app_cmd = APPCOMMAND_MEDIA_PLAY; break; //   0xDE  //  ''"' for US

		case 188: app_cmd = APPCOMMAND_MEDIA_PREVIOUSTRACK; break; // <
		case 190: app_cmd = APPCOMMAND_MEDIA_NEXTTRACK; break; // >

			// modifiers
		case 189: modifier = 1; break; // -
		case 187: modifier = 2; break; // +
		case '6': modifier = 3; break; // ^
		case VK_OEM_3: modifier = 4; break;    // '`~' for US

		default: return false;
	}
	return true;
}

bool KeyboardHandler(int code, WPARAM wParam, LPARAM lParam)
{
	if(code!=HC_ACTION)
		return false;
	KBDLLHOOKSTRUCT * kbs=(KBDLLHOOKSTRUCT*) lParam;

/*
	char bfr[100];
	LPCSTR pMsg;
	char msgbfr[20];
	switch (wParam)
	{
		case WM_KEYDOWN:     pMsg = "WM_KEYDOWN"; break;
		case WM_KEYUP:       pMsg = "WM_KEYUP"; break;
		case WM_CHAR:        pMsg = "WM_CHAR"; break;
		case WM_DEADCHAR:    pMsg = "WM_DEADCHAR"; break;
		case WM_SYSKEYDOWN:  pMsg = "WM_SYSKEYDOWN"; break;
		case WM_SYSKEYUP:    pMsg = "WM_SYSKEYUP"; break;
		case WM_SYSCHAR:     pMsg = "WM_SYSCHAR"; break;
		case WM_SYSDEADCHAR: pMsg = "WM_SYSDEADCHAR"; break;
		case WM_UNICHAR:     pMsg = "WM_UNICHAR"; break;
		default: pMsg=_itoa(wParam, msgbfr, 10);
	}

	char mods[30] = "";
	if( GetKeyState(VK_SHIFT)<0 ) strcat_s( mods, "S " );
	if( GetKeyState(VK_LSHIFT)<0 ) strcat_s( mods, "LS " );
	if( GetKeyState(VK_RSHIFT)<0 ) strcat_s( mods, "RS " );
	if( GetKeyState(VK_CONTROL)<0 ) strcat_s( mods, "C " );
	if( GetKeyState(VK_LCONTROL)<0 ) strcat_s( mods, "LC " );
	if( GetKeyState(VK_RCONTROL)<0 ) strcat_s( mods, "RC " );
	if( GetKeyState(VK_MENU)<0 ) strcat_s( mods, "A " );
	if( GetKeyState(VK_LMENU)<0 ) strcat_s( mods, "LA " );
	if( GetKeyState(VK_RMENU)<0 ) strcat_s( mods, "RA " );
	sprintf_s( bfr, "wParam=%s code=%d, scan=%d, flag=%x [%s]\r\n", pMsg, kbs->vkCode, kbs->scanCode, kbs->flags, mods);
	OutputDebugStringA(bfr);
//	CallNextHookEx( gl_hkb, code, wParam, lParam );
	if (kbs->vkCode == VK_RMENU)
	{
		if ( wParam == WM_SYSKEYDOWN )
		{
			UnpressAlt();
			HWND exWnd=GetForegroundWindow();
			if(exWnd)
			{
				DWORD exPrID;
				DWORD exThID=GetWindowThreadProcessId(exWnd, &exPrID);
				DWORD ourThID=::GetCurrentThreadId();
				bool needAttach=ourThID!=exThID;

				if(!needAttach || AttachThreadInput(ourThID, exThID, TRUE))
				{
					UnpressAlt();
					if(needAttach)
						AttachThreadInput(ourThID, exThID, FALSE);
				}
			}

//			INPUT inp;
//			inp.type=INPUT_KEYBOARD;
//			inp.ki.dwExtraInfo = kbs->dwExtraInfo;
//			inp.ki.dwFlags = kbs->flags ^ 0x80;
//			inp.ki.time = kbs->time + 1;
//			inp.ki.wScan = kbs->scanCode;
//			inp.ki.wVk = kbs->vkCode;
//			SendInput( 1, &inp, sizeof(inp);
		return true;
		}
		return false;
	}
	return false;
	*/


	static int modifier = 0;
	static bool alt_down = false;
	static bool pressed_with_right_alt = false;

	bool down;
	if(wParam==WM_SYSKEYDOWN)
		down=true;
	else if(wParam==WM_SYSKEYUP)
		down=false;
	else if(wParam==WM_KEYDOWN)
	{
		if( alt_down )
			pressed_with_right_alt = true;

		if ( modifier ) {
			switch( kbs->vkCode )
			{
			case VK_SHIFT:
			case VK_LSHIFT:
			case VK_RSHIFT:
			case VK_CAPITAL:
				return false; // do not change modifier
			}
		}


		if ( modifier == 5 ) { // rMenu pressed and released
			modifier = 0;
			WCHAR symbol = 0;
			int   dummy1 = 0, dummy2 = 0;
			GetCodeMeaning( kbs->vkCode, symbol, dummy1, dummy2 );
			if ( !symbol )
				return false;
			SendCharToInputWindow(symbol, kbs->scanCode);
			return true;
		}

		LPWSTR pSymb = 0;
		int was_mod = modifier;
		if ( modifier ) {
			switch( kbs->vkCode )
			{
			case 'E': pSymb = L"\xC8\xE8"; break;
			case 'U': pSymb = L"\xD9\xF9"; break;
			case 'I': pSymb = L"\xCC\xEC"; break;
			case 'O': pSymb = L"\xD2\xF2"; break;
			case 'A': pSymb = L"\xC0\xE0"; break;
			}
		}
		modifier = 0;
		if ( !pSymb )
			return false;
		WCHAR q=pSymb[ IsShift()? 0 : 1];
		q+=was_mod-1;
		SendCharToInputWindow(q, kbs->scanCode);
		return true;
	}
	else if (wParam==WM_KEYUP && kbs->vkCode==VK_RMENU)
		down = false;
	else
		return false;

	if( kbs->vkCode == VK_RMENU )
	{
		alt_down = down;
		if ( alt_down ) // pressed
			pressed_with_right_alt = false; // cleared
		else if ( !pressed_with_right_alt )
			modifier = 5; // if nothing was pressed between
		return alt_down; // let alt up go to message loop // true;
	}

	if( !alt_down )
		return false;
//	if(GetKeyState(VK_RMENU)>=0)
//		return false;
	WCHAR symbol = 0;
	int   app_cmd = 0;

	modifier = 0;
	pressed_with_right_alt = true;
	// 165 is Gr Alt
	if ( !GetCodeMeaning( kbs->vkCode, symbol, app_cmd, modifier ) )
		return false;

	if( modifier )
		return true;

	if(down) {
		if( symbol )
			SendCharToInputWindow(symbol, kbs->scanCode);//sym_sets_set[state].chars[num]);
		else if ( app_cmd )
			SendAppCommand( app_cmd );
	}


//	ru LoadKeyboardLayout( StrCopy(Layout,'00000419'),KLF_ACTIVATE);
//	en LoadKeyboardLayout(StrCopy(Layout,'00000409'),KLF_ACTIVATE);

	return true;
}

LRESULT CALLBACK KeyboardLLHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if(KeyboardHandler(code, wParam, lParam))
		return TRUE;
	return CallNextHookEx( gl_hkb, code, wParam, lParam );
}

extern "C" BOOL InstallHook(HWND hWnd)
{
	gl_hWnd = hWnd;
	if(!gl_hkb)
		gl_hkb=SetWindowsHookEx(WH_KEYBOARD_LL,(HOOKPROC)KeyboardLLHookProc,gl_hInstance, 0);
	return gl_hkb!=0;
}
extern "C" BOOL UninstallHook()
{
	BOOL res=TRUE;
	if(gl_hkb)
		res=UnhookWindowsHookEx(gl_hkb);
	gl_hkb=0;
	return res;
}
