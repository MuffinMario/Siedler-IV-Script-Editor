
#include "window.h"
#include "stdafx.h"
#include <iostream>
#include <bitset>
#include <map>
#include <string>
#include <vector>
#include <CommCtrl.h>
#include <TlHelp32.h>
#include "S4EditorLuaEditor.h"



HWND hwndButton = nullptr;
HWND hwndEdit = nullptr;
HWND hwndCombobox = nullptr;

HMENU hmenuBar = nullptr;
HMENU hmenu = nullptr;
// no std::map<int,std::wstring>, we need index of vector for comboBox index
const std::vector<std::pair<unsigned short, std::wstring>> objectsArr{
	{ 19, L"Tropische Dattelpalme" },
	{ 20, L"Tropische Dattelpalme 2" },
	{ 21, L"Dunkelblättrige tropische Palme" },
	{ 22, L"Dunkelblättrige tropische Palme 2" },
	{ 42, L"Verwildertes Trojanisches Pferd" },
	{ 94, L"Kleiner dunkler Fels" },
	{ 98, L"Kleiner rötlicher Fels" },
	{ 97, L"Mittelgroßer Rötlicher Fels" },
	{ 96, L"Großer rötlicher Fels" },
	{ 95, L"Rötlicher Fels" },
	{ 102, L"Kleiner dunkler Stein" },
	{ 115, L"Morbus' übriger Statuenboden" },
	{ 117, L"Trojanisches Pferd im Bau" },
	{ 118, L"Trojanisches Pferd" },
	{ 119, L"Portal" },
	{ 123, L"Planierstöcke" },
	{ 151, L"Dunkler Schneemann" },
	{ 178, L"Halber kleiner Rinderhaufen" },
	{ 194, L"1x Kohlevorkommen Schild" },
	{ 195, L"2x Kohlevorkommen Schild" },
	{ 196, L"3x Kohlevorkommen Schild" },
	{ 197, L"1x Goldvorkommen  Schild" },
	{ 198, L"2x Goldvorkommen  Schild" },
	{ 199, L"3x Goldvorkommen  Schild" },
	{ 200, L"1x Eisenvorkommen  Schild" },
	{ 201, L"2x Eisenvorkommen  Schild" },
	{ 202, L"3x Eisenvorkommen  Schild" },
	{ 203, L"1x Steinvorkommen  Schild" },
	{ 204, L"2x Steinvorkommen  Schild" },
	{ 205, L"3x Steinvorkommen  Schild" },
	{ 206, L"1x Schwefelvorkommen  Schild" },
	{ 207, L"2x Schwefelvorkommen  Schild" },
	{ 208, L"3x Schwefelvorkommen  Schild" },
	{ 216, L"*in dunkles Land umwandeln* (Effekt im Spiel)" },
	{ 223, L"Schloss" },
	{ 224, L"Turm mit Feuer auf der Spitze" },
	{ 225, L"Koloss von Rhodos" },
	{ 226, L"Mosaik Statue" },
	{ 227, L"Mosaik Statue im Bau" },
	{ 228, L"MA MA Hütte" },
	{ 230, L"Tropischer Busch" },
	{ 232, L"P-Runenpyramide" }
};
const int PADDING_WIDTH = 10, PADDING_HEIGHT = 10;
int buttonWidth = 100, buttonHeight = 25;
int comboboxWidth = 200, comboboxHeight = 25;
int windowWidth = 500, windowHeight = 450;
bool canUseRichTextBox = false;

bool overrideOnce = false;

extern char* luaScript;
extern unsigned short* mapProperties;
extern unsigned short* currentSelectedItem;
extern unsigned short* objectIDSpitPlant;
// ARRAY SIZE 0xFE where index = objectID - 1 (index[0] = objectID 1) GROUND = GRASS/ROCK
extern bool* a_canPlaceOnGround;
extern const size_t a_canPlaceOnGroundSize;
// ARRAY SIZE 0xA2 where index = objectID - 11 (index[0] = objectID 11 (first palm))
extern bool* a_canPlaceOnSand;
extern const size_t a_canPlaceOnSandSize;

extern HANDLE s4;
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
void selectObjectID(unsigned short objectID)
{
	constexpr const unsigned short DIALOG_OVERWRITE_ITEM = 315; // "Dunkle Spuckpflanze"
	*currentSelectedItem = DIALOG_OVERWRITE_ITEM;
	*objectIDSpitPlant = objectID;
}	
void DoSuspendThread(DWORD targetProcessId, DWORD targetThreadId)
{
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if (Thread32First(h, &te))
		{
			do
			{
				if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
				{
					// Suspend all threads EXCEPT the one we want to keep running
					if (te.th32ThreadID != targetThreadId && te.th32OwnerProcessID == targetProcessId)
					{
						HANDLE thread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
						if (thread != NULL)
						{
							SuspendThread(thread);
							CloseHandle(thread);
						}
					}
				}
				te.dwSize = sizeof(te);
			} while (Thread32Next(h, &te));
		}
		CloseHandle(h);
	}
}
void DoResumeThread(DWORD targetProcessId, DWORD targetThreadId)
{
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if (Thread32First(h, &te))
		{
			do
			{
				if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
				{
					// Suspend all threads EXCEPT the one we want to keep running
					if (te.th32ThreadID != targetThreadId && te.th32OwnerProcessID == targetProcessId)
					{
						HANDLE thread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
						if (thread != NULL)
						{
							ResumeThread(thread);
							CloseHandle(thread);
						}
					}
				}
				te.dwSize = sizeof(te);
			} while (Thread32Next(h, &te));
		}
		CloseHandle(h);
	}
}

void suspendAllThreads()
{
	DoSuspendThread(GetProcessId(s4), GetCurrentThreadId());
}
void resumeAllThreads()
{
	DoResumeThread(GetProcessId(s4), GetCurrentThreadId());
}
void overridePreviousPlaceableSettings()
{
	//stop "mute" all threads so no other programm can access code page
	suspendAllThreads();
	DWORD oldProtection;

	LPVOID startPage = (LPVOID)((unsigned int)s4 + 0x1000);
	size_t pageSize = 0x44000;
	VirtualProtect(startPage, pageSize, PAGE_EXECUTE_READWRITE, &oldProtection);


	short* addr = (short*)((unsigned int)s4 + 0x134D4);
	
	//this will create a problem when you execute this code while the two adresses are currently running in another thread, this will replace one 2 byte opcode with 2 nop's
	*addr = 0x9090; // nop nop


	for (auto it : objectsArr)
	{
		setObjectOnGroundPlaceable((short)it.first, true);
		setObjectOnSandPlaceable((short)it.first, true);
	}

	

	//reset to default protection
	VirtualProtect(startPage, pageSize, oldProtection, nullptr);

	//unmute
	resumeAllThreads();
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

		AppendMenuW(hmenu, MF_STRING, IDC_MO_CHECK_NEW_WORLD, L"&Neue-Welt Texturen");
		AppendMenuW(hmenu, MF_STRING, IDC_MO_CHECK_MAP_PREVIEW, L"&Verstecke Mapvorschau");

		//On Editor Start: Default Properties
		CheckMenuItem(hmenu, IDC_MO_CHECK_NEW_WORLD, MF_UNCHECKED);
		CheckMenuItem(hmenu, IDC_MO_CHECK_MAP_PREVIEW, MF_UNCHECKED);

		AppendMenuW(hmenuBar, MF_POPUP, (UINT_PTR)hmenu, L"Map-&Optionen");
		SetMenu(hwnd, hmenuBar);

		hwndButton = CreateWindow(
			L"BUTTON",  // Predefined class; Unicode assumed 
			L"Script setzen",      // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
			windowWidth - PADDING_WIDTH - buttonWidth,         // x position 
			windowHeight - PADDING_HEIGHT - buttonHeight - PADDING_HEIGHT - comboboxHeight,         // y position 
			buttonWidth,     // Button width
			buttonHeight,        // Button height
			hwnd,     // Parent window
			(HMENU)IDC_SCRIPT_SET_BUTTON,       // No menu.
			((LPCREATESTRUCT)lParam)->hInstance,
			NULL);
		hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, canUseRichTextBox ? TEXT("RICHEDIT50W") : TEXT("edit"), NULL,
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE |
			ES_AUTOVSCROLL, 
			PADDING_WIDTH, PADDING_HEIGHT, 100, 100, 
			hwnd, 
			(HMENU)IDC_SCRIPT_EDIT_BOX, 
			((LPCREATESTRUCT)lParam)->hInstance, 
			NULL);
		hwndCombobox = CreateWindowEx(WS_EX_CLIENTEDGE,
			WC_COMBOBOX, TEXT(""),
			WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
			PADDING_WIDTH, windowHeight - PADDING_HEIGHT - comboboxHeight, comboboxWidth, comboboxHeight * 10,
			hwnd,
			(HMENU)IDC_OBJECTS_COMBO_BOX, 
			((LPCREATESTRUCT)lParam)->hInstance, 
			NULL);
		for (auto it = objectsArr.begin(); it != objectsArr.end(); it++) {
			SendMessage(hwndCombobox, CB_ADDSTRING, 0, (LPARAM)(it->second.c_str()));
		}


		//SETTINGS 
		NONCLIENTMETRICS metrics;
		metrics.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS),
			&metrics, 0);
		HFONT font = CreateFontIndirect(&metrics.lfMessageFont);
		SendMessage(hwndButton, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));
		SendMessage(hwndCombobox, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE, 0));

		HFONT editFont = CreateFont(14, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
			OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, TEXT("Consolas"));
		SendMessage(hwndEdit, WM_SETFONT, (WPARAM)editFont, MAKELPARAM(TRUE, 0));
		SendMessage(hwndEdit, EM_SETLIMITTEXT, 0xFFFE, NULL);


	}
	break;
	case WM_COMMAND:
	{
		// wParam:
		//	LOWORD: ID of item
		//	HIWORD: notification code
		WORD hwp = HIWORD(wParam);
		WORD lwp = LOWORD(wParam);
		//std::cout << "High wParam: " << HIWORD(wParam) << "Low wParam: " << LOWORD(wParam) << std::endl << "lparam: " << lParam << std::endl;
		if (!overrideOnce && lwp == IDC_OBJECTS_COMBO_BOX)
		{
			overrideOnce = true;
			overridePreviousPlaceableSettings();
		}
		/* ITEM SPECIFIC*/
		if (lwp == IDC_SCRIPT_SET_BUTTON)
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
				MessageBox(hwnd, L"Script erfolgreich ersetzt! \r\nVergesse nicht die Map zu speichern!", L"Siedler IV Lua Editor", MB_OK);
			}
		}
		else if (lwp == IDC_SCRIPT_EDIT_BOX)
		{
			//nothing here 
		}
		else if (lwp == IDC_MO_CHECK_NEW_WORLD)
		{
			bool checked = GetMenuState(hmenu, IDC_MO_CHECK_NEW_WORLD, MF_BYCOMMAND) == MF_CHECKED;

			if (checked) {
				CheckMenuItem(hmenu, IDC_MO_CHECK_NEW_WORLD, MF_UNCHECKED);
			}
			else {
				CheckMenuItem(hmenu, IDC_MO_CHECK_NEW_WORLD, MF_CHECKED);
			}

			toggleMapProperty(MMAP_NEW_WORLD, !checked);
		}
		else if (lwp == IDC_MO_CHECK_MAP_PREVIEW)
		{

			bool checked = GetMenuState(hmenu, IDC_MO_CHECK_MAP_PREVIEW, MF_BYCOMMAND) == MF_CHECKED;

			if (checked) {
				CheckMenuItem(hmenu, IDC_MO_CHECK_MAP_PREVIEW, MF_UNCHECKED);
			}
			else {
				CheckMenuItem(hmenu, IDC_MO_CHECK_MAP_PREVIEW, MF_CHECKED);
			}

			toggleMapProperty(MMAP_HIDE_MAP_PREVIEW, !checked);
		}
		/* NOTIFICATIONS */
		else if (hwp == CBN_SELCHANGE)
		{
			int cbIndex = SendMessage((HWND)lParam, CB_GETCURSEL,NULL,NULL);
			unsigned short objectID = objectsArr.at(cbIndex).first;
			selectObjectID(objectID);
		}
		else
		{
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	}
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
			SetWindowPos(hwndButton, nullptr, newWidth - PADDING_WIDTH - buttonWidth, newHeight - PADDING_HEIGHT - buttonHeight - PADDING_HEIGHT - comboboxHeight, buttonWidth, buttonHeight, NULL);
		}
		if (hwndEdit != nullptr)
		{
			//position: absolute, size: relative
			SetWindowPos(hwndEdit, nullptr, PADDING_WIDTH, PADDING_HEIGHT, newWidth - PADDING_WIDTH,
				newHeight - PADDING_HEIGHT - PADDING_HEIGHT - buttonHeight - PADDING_HEIGHT - comboboxHeight, NULL);
		}
		if (hwndCombobox != nullptr)
		{
			//position: relative, size: absolute
			SetWindowPos(hwndCombobox, nullptr,
				PADDING_WIDTH, newHeight - PADDING_HEIGHT - comboboxHeight,
				comboboxWidth, comboboxHeight * 10,
				NULL);

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