
#include "window.h"
#include "stdafx.h"
#include <iostream>
#include <bitset>
#include "S4EditorLuaEditor.h"


HWND hwndButton = nullptr;
HWND hwndEdit = nullptr;

HMENU hmenuBar = nullptr;
HMENU hmenu = nullptr;

int buttonWidth = 100, buttonHeight = 25;
int windowWidth = 500, windowHeight = 250;
bool canUseRichTextBox = false;
extern char* luaScript;
extern unsigned short* mapProperties;
/*
* Writes string to script memory region
* example:
int code = writeNewLuaString("function new_game()\r\ndbg.stm(\"test\") \r\nend");
if (code > 0)
{
cout << "Can't write more than 65534 characters!" << endl;
}
else if (code < 0)
{
cout << "Adress of script not initalized!" << endl;
}
*/

int writeNewLuaString(const char* newLuaString)
{
	if (luaScript != nullptr)
	{
		size_t len = strlen(newLuaString);
		if (len > 0xFFFE) return 1;
		//fuck functions we doing it ourself quick and dirty
		for (size_t i = 0; i < len; i++)
		{
			*(luaScript + i) = *(newLuaString + i);
		}
		*(luaScript + len) = 0x0;
		return 0;
	}
	return -1;
}
void toggleMapProperty(unsigned int propertyBit,bool enable)
{
	unsigned short current_flags = *mapProperties;
	/*
		1. zero the bit u want
		2. add bit property (none if 0, 1 if 1)

		i didnt want to do ifs and i am disappointed i have to use addition instead of  logical stuff
	*/
	*mapProperties = (current_flags & ((unsigned short)0xFFFF - (1 << propertyBit))) + ((enable ? 1 : 0) << propertyBit);
}


LRESULT CALLBACK DLLWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//std::cout << "window message: " << std::dec << message << std::endl;

	switch (message)
	{
	case WM_CREATE:
		//std::cout << "create!" << std::endl;
		{
			hmenuBar = CreateMenu();
			hmenu = CreateMenu();

			AppendMenuW(hmenu, MF_STRING, CHECK_NEW_WORLD, L"&Neue-Welt Texturen");
			AppendMenuW(hmenu, MF_STRING, CHECK_MAP_PREVIEW, L"&Verstecke Mapvorschau");

			//On Editor Start: Default Properties
			CheckMenuItem(hmenu, CHECK_NEW_WORLD, MF_UNCHECKED);
			CheckMenuItem(hmenu, CHECK_MAP_PREVIEW, MF_UNCHECKED);

			AppendMenuW(hmenuBar, MF_POPUP, (UINT_PTR)hmenu, L"Map-&Optionen");
			SetMenu(hwnd, hmenuBar);

			hwndButton = CreateWindow(
				L"BUTTON",  // Predefined class; Unicode assumed 
				L"Script setzen",      // Button text 
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
				windowWidth - 10 - buttonWidth,         // x position 
				windowHeight - 10 - buttonHeight,         // y position 
				buttonWidth,     // Button width
				buttonHeight,        // Button height
				hwnd,     // Parent window
				(HMENU)10000,       // No menu.
				((LPCREATESTRUCT)lParam)->hInstance,
				NULL);
			hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, canUseRichTextBox?TEXT("RICHEDIT50W"):TEXT("edit"), NULL,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE |
				ES_AUTOVSCROLL, 10, 10, 100,
				100, hwnd, (HMENU)11000, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

			//SETTINGS 
			NONCLIENTMETRICS metrics;
			metrics.cbSize = sizeof(NONCLIENTMETRICS);
			::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS),
				&metrics, 0);
			HFONT font = ::CreateFontIndirect(&metrics.lfMessageFont);
			::SendMessage(hwndButton, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));

			HFONT editFont = CreateFont(14, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
				OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_DONTCARE, TEXT("Consolas"));
			::SendMessage(hwndEdit, WM_SETFONT, (WPARAM)editFont, MAKELPARAM(TRUE, 0));
			::SendMessage(hwndEdit, EM_SETLIMITTEXT, 0xFFFE,NULL);


		}
		break;
	case WM_COMMAND:
		switch (wParam)
		{
		case 10000:
			// ON BUTTON CLICK: WRITE TEXT
		{
			char buf[0xFFFE];
			GetWindowTextA(hwndEdit, buf, 0xFFFE);
			int code = writeNewLuaString(buf);
			if (code < 0)
			{
				MessageBox(hwnd, L"Interner Fehler. Auf Lua-Adresse kann nicht geschrieben werden. Wenn der Fehler bei Neustart nicht behoben wird, kontaktiere mich auf https://s4.muffinmar.io/", L"Fehler!", MB_OK);
			}
			else if (code > 0)
			{
				MessageBox(hwnd, L"Das Script darf nur 65534 Zeichen enthalten", L"Fehler!", MB_OK);
			}
			else
			{
				MessageBox(hwnd, L"Script erfolgreich ersetzt! \r\nVergesse nicht die Map zu speichern!",L"Siedler IV Lua Editor",MB_OK);
			}
		}
		break;
		case 11000:
			// ON EDITBOX EVENT
			break;
		case CHECK_NEW_WORLD:
		{
			bool checked = GetMenuState(hmenu, CHECK_NEW_WORLD, MF_BYCOMMAND) == MF_CHECKED;

			if (checked) {
				CheckMenuItem(hmenu, CHECK_NEW_WORLD, MF_UNCHECKED);
			}
			else {
				CheckMenuItem(hmenu, CHECK_NEW_WORLD, MF_CHECKED);
			}

			toggleMapProperty(MMAP_NEW_WORLD, !checked);
		}
			break;
		case CHECK_MAP_PREVIEW:
		{
			bool checked = GetMenuState(hmenu, CHECK_MAP_PREVIEW, MF_BYCOMMAND) == MF_CHECKED;

			if (checked) {
				CheckMenuItem(hmenu, CHECK_MAP_PREVIEW, MF_UNCHECKED);
			}
			else {
				CheckMenuItem(hmenu, CHECK_MAP_PREVIEW, MF_CHECKED);
			}

			toggleMapProperty(MMAP_HIDE_MAP_PREVIEW, !checked);
		}
		break;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		//PostQuitMessage(0);
		break;
	case WM_QUIT:
		//PostQuitMessage(0);
		break;
	case WM_CLOSE:
		//minimize in corner
		SetWindowPos(hwnd, nullptr, 0, INFINITE, 250, 0, NULL);
		break;
	case WM_SIZE:
	{
		short newWidth = lParam & 0xFFFF;
		short newHeight = (lParam >> 16) & 0xFFFF;

		//update members accordingly
		if (hwndButton != nullptr)
		{
			//position: relative, size: absolute
			SetWindowPos(hwndButton, nullptr, newWidth - 10 - buttonWidth, newHeight - 10 - buttonHeight, buttonWidth, buttonHeight, NULL);
		}
		if (hwndEdit != nullptr)
		{
			//position: absolute, size: relative
			SetWindowPos(hwndEdit, nullptr, 10, 10, newWidth - 20,
				newHeight - 20 - buttonHeight, NULL);
		}

	}
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}




BOOL RegisterDLLWindowClass(const wchar_t* szClassName, HINSTANCE inj_hModule)
{
	WNDCLASSEX wc;
	wc.hInstance = inj_hModule;
	wc.lpszClassName = szClassName;
	wc.lpfnWndProc = DLLWindowProc;
	wc.style = CS_DBLCLKS;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	if (!RegisterClassEx(&wc))
		return 0;
	return 1;
}


HWND initWindow(HINSTANCE hInstance)
{
	//NEED LIBRARY FOR RICHEDIT
	canUseRichTextBox = LoadLibrary(TEXT("Msftedit.dll")) != nullptr;

	const wchar_t* className = L"S4EditorLuaEditorWindow";
	HWND hwnd = nullptr;
	if (RegisterDLLWindowClass(className, hInstance))
	{
		hwnd = CreateWindowEx(
			0,
			className,
			L"Siedler IV Lua Editor",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight + 25, //+25 becuase bar above
			NULL,
			NULL,
			hInstance,
			NULL
		);


	}
	return hwnd;
}