/*
Fadenkreuz

A simple open-source crosshairs app for Windows

MIT License

Copyright (c) 2025 Matthias Deeg (X: @matthiasdeeg / Mastodon: @deeg@mastodon.social)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdint.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>  

#include "resource.h"

/*
 * CONSTANTS
 */

// some strings
#define APPNAME				"Fadenkreuz"								
#define WINDOW_CLASSNAME	"FadenkreuzClass"
#define SHAPE_SETTING		"Shape"
#define COLOR_SETTING		"Color"
#define SIZE_SETTING		"Size"
#define THICKNESS_SETTING	"Thickness"
#define X_OFFSET_SETTING	"X-offset"
#define Y_OFFSET_SETTING	"Y-offset"

// hotkey IDs
#define HOTKEY_EXIT					1000						// hotkey ID for exiting the crosshairs app
#define HOTKEY_TOGGLE				1001						// hotkey ID for enabling/disabling the crosshairs
#define HOTKEY_NEXT_SHAPE			1002						// hotkey ID for selecting next crosshairs shape
#define HOTKEY_PREV_SHAPE			1003						// hotkey ID for selecting previous crosshairs shape
#define HOTKEY_INCREASE_SIZE		1004						// hotkey ID for increasing the crosshairs size
#define HOTKEY_DECREASE_SIZE		1005						// hotkey ID for decreasing the crosshairs size
#define HOTKEY_NEXT_COLOR 			1006						// hotkey ID for selecting next crosshairs color
#define HOTKEY_PREV_COLOR 			1007						// hotkey ID for selecting previous crosshairs color
#define HOTKEY_INCREASE_THICKNESS	1008						// hotkey ID for increasing the pen thickness (width) for drawing the crosshairs
#define HOTKEY_DECREASE_THICKNESS	1009						// hotkey ID for decreasing the pen thickness (width) for drawing the crosshairs
#define HOTKEY_INC_X_OFFSET			1010						// hotkey ID for increasing the crosshairs X offset from the screen center
#define HOTKEY_DEC_X_OFFSET			1011						// hotkey ID for decreasing the crosshairs X offset from the screen center
#define HOTKEY_INC_Y_OFFSET			1012						// hotkey ID for increasing the crosshairs Y offset from the screen center
#define HOTKEY_DEC_Y_OFFSET			1013						// hotkey ID for decreasing the crosshairs Y offset from the screen center
#define HOTKEY_CENTER				1014						// hotkey ID for centering the crosshairs
#define HOTKEY_LOAD_SETTINGS		1015						// hotkey ID for loading settings from the Windows registry
#define HOTKEY_SAVE_SETTINGS		1016						// hotkey ID for saving settings from the Windows registry

// crosshairs constants
#define MAX_CROSSHAIRS_SIZE		64								// max. crosshairs size in pixels
#define MAX_PEN_WIDTH			4								// max. pen width for drawing the crosshairs
#define DELAY_OVERLAY_UPDATE	1000							// time interval in milliseconds for updating the overlay window

/*
 * FUNCTION PROTOTYPES
 */
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);  
DWORD WINAPI UpdateOverlay(LPVOID lpParam);
void DrawOverlay(HWND hwnd);
void LoadSettings();
void SaveSettings();

/*
 * GLOBAL VARIABLES
 */
HINSTANCE hInst;												// application instance handle

// crosshairs parameters
int8_t penWidth = 1;											// pen width
int8_t crosshairsSize = 10;										// size of crosshairs
int32_t x_offset = 0;											// crosshairs X offset from screen center
int32_t y_offset = 0;											// crosshairs Y offset from screen center
int32_t max_x_offset = 0;										// max. x offset
int32_t max_y_offset = 0;										// max. y offset
bool crosshairsVisible = true;									// flag for crosshairs visibility


// defined colors
COLORREF TRANSPARENT_COLOR = RGB(0, 0, 0);						// set transparent color
COLORREF COLORS[] = {											// defined crosshairs colors
	RGB(255, 0, 0),			// red
	RGB(0, 255, 0),			// green
	RGB(0, 0, 255),			// blue
	RGB(0, 255, 255),		// cyan
	RGB(255, 255, 0),		// yellow
	RGB(255, 0, 255),		// pink
	RGB(255, 255, 255),		// white
	RGB(128, 128, 128),		// gray
};
uint8_t numColors = sizeof(COLORS) / sizeof(COLORREF);			// number of available colors
int8_t currentColor = 0;										// currently used color (zero-indexed)
uint8_t numShapes = 6;											// number of crosshairs shapes
int8_t currentShape = 0;										// currently used crosshairs shape (zero-indexed)
 
/*
 * WinMain application entry point
 */
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {  
	MSG msg;  

	// set instance handle
	hInst = hInstance;  

	// check if the app is already running by looking for its window
	if (FindWindow(TEXT(WINDOW_CLASSNAME), TEXT(APPNAME)) != NULL) {
		// exit this app instance
		return 0;
	}

	// load app icon
	HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));

	// define window class
	WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX),
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInst,
		LoadIcon(NULL, IDI_APPLICATION),  
		LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		NULL,
		TEXT(WINDOW_CLASSNAME),
		hIcon
	};  

	// register window class
	if (!RegisterClassEx(&wcex)) {
		return MessageBox(NULL, TEXT("Could not register the window class!"), TEXT("Error"), MB_ICONERROR | MB_OK);  
	}

	// create window
	HWND hWnd = CreateWindowEx(
		WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED,
		wcex.lpszClassName,
		TEXT(APPNAME),
		WS_DISABLED,
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		NULL,
		NULL,
		hInst,
		NULL
	);  

	if (!hWnd) {
		return MessageBox(NULL, TEXT("Could not create the layered window!"), TEXT("Error"), MB_ICONERROR | MB_OK);  
	}

	// register global hotkeys
	RegisterHotKey(hWnd, HOTKEY_EXIT, 0, VK_F9);
	RegisterHotKey(hWnd, HOTKEY_TOGGLE, 0, VK_F1);
	RegisterHotKey(hWnd, HOTKEY_INC_X_OFFSET, 0, VK_F2);
	RegisterHotKey(hWnd, HOTKEY_DEC_X_OFFSET, MOD_CONTROL, VK_F2);
	RegisterHotKey(hWnd, HOTKEY_INC_Y_OFFSET, 0, VK_F3);
	RegisterHotKey(hWnd, HOTKEY_DEC_Y_OFFSET, MOD_CONTROL, VK_F3);
	RegisterHotKey(hWnd, HOTKEY_CENTER, 0, VK_F4);
	RegisterHotKey(hWnd, HOTKEY_NEXT_SHAPE, 0, VK_F5);
	RegisterHotKey(hWnd, HOTKEY_PREV_SHAPE, MOD_CONTROL, VK_F5);
	RegisterHotKey(hWnd, HOTKEY_NEXT_COLOR, 0, VK_F6);
	RegisterHotKey(hWnd, HOTKEY_PREV_COLOR, MOD_CONTROL, VK_F6);
	RegisterHotKey(hWnd, HOTKEY_INCREASE_SIZE, 0, VK_F7);
	RegisterHotKey(hWnd, HOTKEY_DECREASE_SIZE, MOD_CONTROL, VK_F7);
	RegisterHotKey(hWnd, HOTKEY_INCREASE_THICKNESS, 0, VK_F8);
	RegisterHotKey(hWnd, HOTKEY_DECREASE_THICKNESS, MOD_CONTROL, VK_F8);
	RegisterHotKey(hWnd, HOTKEY_LOAD_SETTINGS, 0, VK_F10);
	RegisterHotKey(hWnd, HOTKEY_SAVE_SETTINGS, 0, VK_F11);

	// set max x and y offsets	
	max_x_offset = GetSystemMetrics(SM_CXSCREEN) / 2;
	max_y_offset = GetSystemMetrics(SM_CYSCREEN) / 2;

	// load settings from Windows registry
	HKEY hKey;
	DWORD dwError;
	DWORD dwSize;
	DWORD dwValue;
	DWORD dwType = REG_DWORD;
	DWORD lpdwDisposition;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT(APPNAME), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &lpdwDisposition) == ERROR_SUCCESS) {

		if (lpdwDisposition == REG_CREATED_NEW_KEY) {
			// create default settings
			dwValue = currentShape;
			RegSetValueEx(hKey, TEXT(SHAPE_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));

			dwValue = currentColor;
			RegSetValueEx(hKey, TEXT(COLOR_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));

			dwValue = crosshairsSize;
			RegSetValueEx(hKey, TEXT(SIZE_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));

			dwValue = penWidth;
			RegSetValueEx(hKey, TEXT(THICKNESS_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));

			dwValue = x_offset;
			RegSetValueEx(hKey, TEXT(X_OFFSET_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));

			dwValue = y_offset;
			RegSetValueEx(hKey, TEXT(Y_OFFSET_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));
		} else {
			// load existing settings from Windows registry
			LoadSettings();
		}
		RegCloseKey(hKey);
	}

	// draw the crosshairs overlay
	DrawOverlay(hWnd);

	// create thread for periodically updating the overlay window
	CreateThread(NULL, 0, UpdateOverlay, hWnd, 0, 0);

	// show window
	ShowWindow(hWnd, SW_SHOW);  
	UpdateWindow(hWnd);  

	// message loop
	while (GetMessage(&msg, NULL, 0, 0)) {  
		TranslateMessage(&msg);  
		DispatchMessage(&msg);  
	}  

	return (int)msg.wParam;  
}

/*
 * Window procedure
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	LPWINDOWPOS lpwp;

	// process received message
	switch (message) {  
		case WM_NCHITTEST:
			return HTCAPTION;  

		case WM_NCRBUTTONDOWN:
			DestroyWindow(hWnd);  
			break;  

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_HOTKEY:
			switch (wParam) {
				case HOTKEY_EXIT:
					DestroyWindow(hWnd);  
					break;

				case HOTKEY_TOGGLE:
					crosshairsVisible ^= 1;
					DrawOverlay(hWnd);
					break;

				case HOTKEY_INCREASE_SIZE:
					crosshairsSize += 2;
					if (crosshairsSize > MAX_CROSSHAIRS_SIZE) {
						crosshairsSize = MAX_CROSSHAIRS_SIZE;
					}
					DrawOverlay(hWnd);
					break;

				case HOTKEY_DECREASE_SIZE:
					crosshairsSize -= 2;
					if (crosshairsSize < 1) {
						crosshairsSize = 1;
					}
					DrawOverlay(hWnd);
					break;
				
				case HOTKEY_NEXT_COLOR:
					// select next color, cycle through all colors
					currentColor += 1;
					if (currentColor >= numColors) {
						currentColor = 0;
					}
					DrawOverlay(hWnd);
					break;

				case HOTKEY_PREV_COLOR:
					// select previous color, cycle through all colors
					currentColor -= 1;
					if (currentColor < 0) {
						currentColor = numColors - 1;
					}
					DrawOverlay(hWnd);
					break;
				
				case HOTKEY_NEXT_SHAPE:
					// select next shape, cycle through all shapes
					currentShape += 1;
					if (currentShape >= numShapes) {
						currentShape = 0;
					}
					DrawOverlay(hWnd);
					break;

				case HOTKEY_PREV_SHAPE:
					// select previous shape, cycle through all shapes
					currentShape -= 1;
					if (currentShape < 0) {
						currentShape = numShapes - 1;
					}
					DrawOverlay(hWnd);
					break;

				case HOTKEY_INCREASE_THICKNESS:
					penWidth += 1;
					if (penWidth > MAX_PEN_WIDTH) {
						penWidth = MAX_PEN_WIDTH;
					}
					DrawOverlay(hWnd);
					break;

				case HOTKEY_DECREASE_THICKNESS:
					penWidth -= 1;
					if (penWidth < 1) {
						penWidth = 1;
					}
					DrawOverlay(hWnd);
					break;

				case HOTKEY_INC_X_OFFSET:
					x_offset += 1;
					if (x_offset > max_x_offset) {
						x_offset = max_x_offset;
					}
					DrawOverlay(hWnd);
					break;

				case HOTKEY_DEC_X_OFFSET:
					x_offset -= 1;
					if (x_offset < (-1 * max_x_offset)) {
						x_offset = (-1 * max_x_offset);
					}
					DrawOverlay(hWnd);
					break;

				case HOTKEY_INC_Y_OFFSET:
					y_offset += 1;
					if (y_offset > max_y_offset) {
						y_offset = max_y_offset;
					}
					DrawOverlay(hWnd);
					break;

				case HOTKEY_DEC_Y_OFFSET:
					y_offset -= 1;
					if (y_offset < (-1 * max_y_offset)) {
						y_offset = (-1 * max_y_offset);
					}
					DrawOverlay(hWnd);
					break;

				case HOTKEY_CENTER:
					// reset offsets to zero
					x_offset = 0;
					y_offset = 0;
					DrawOverlay(hWnd);
					break;

				case HOTKEY_LOAD_SETTINGS:
					LoadSettings();
					DrawOverlay(hWnd);
					break;

				case HOTKEY_SAVE_SETTINGS:
					SaveSettings();
					DrawOverlay(hWnd);
					break;
			}
			break;  

		case WM_WINDOWPOSCHANGED:
			lpwp = (LPWINDOWPOS)lParam;
			SetWindowPos(lpwp->hwnd, lpwp->hwndInsertAfter, lpwp->x, lpwp->y, lpwp->cx, lpwp->cy, lpwp->flags);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);  
	}  
	return 0;  
}  

/*
 * Update overlay window
 */
DWORD WINAPI UpdateOverlay(LPVOID lpParam) {
    HWND targetHwnd = (HWND)lpParam;

    while (true) {
		// create WINDOWPOS structure
        WINDOWPOS pos = {};
        pos.hwnd = targetHwnd;
        pos.hwndInsertAfter = HWND_TOP;
        pos.x = 0;
        pos.y = 0;
        pos.cx = 0;
        pos.cy = 0;
        pos.flags = SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSENDCHANGING;

        // send WM_WINDOWPOSCHANGED message
        SendMessage(targetHwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&pos);

        // wait for some time before sending the next message
        Sleep(DELAY_OVERLAY_UPDATE);
    }
    return 0;
}

/*
 * Draw crosshairs on overlay window
 */
void DrawOverlay(HWND hwnd) {
	// calculate center coordinates
	int32_t screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int32_t screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int32_t centerX = (screenWidth / 2) + x_offset;
	int32_t centerY = (screenHeight / 2) + y_offset;

    // create a memory DC
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    // create a compatible bitmap for the memory DC
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    // make the background transparent
    HBRUSH hBrush = CreateSolidBrush(TRANSPARENT_COLOR);
    RECT fillRect = {0, 0, screenWidth, screenHeight};
    FillRect(hdcMem, &fillRect, hBrush);
    DeleteObject(hBrush);

	// draw crosshairs
	if (crosshairsVisible) {
		// draw the crosshairs
		HPEN hPen = CreatePen(PS_SOLID, 1, COLORS[currentColor]);
		HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPen);
		hBrush = CreateSolidBrush(COLORS[currentColor]);
		HBRUSH hBrushNegative = CreateSolidBrush(TRANSPARENT_COLOR);

		RECT rect;
		RECT negativeRect;
		int sideLength = crosshairsSize / 2;

		// draw selected crosshairs shape
		switch (currentShape) {
			case 0:
				rect = {centerX - crosshairsSize, centerY - penWidth, centerX + crosshairsSize, centerY + penWidth};
				FillRect(hdcMem, &rect, hBrush);

				rect = {centerX - penWidth, centerY - crosshairsSize, centerX + penWidth, centerY + crosshairsSize};
				FillRect(hdcMem, &rect, hBrush);
				break;

			case 1:
				rect = {centerX - crosshairsSize, centerY - penWidth, centerX + crosshairsSize, centerY + penWidth};
				FillRect(hdcMem, &rect, hBrush);

				rect = {centerX - penWidth, centerY - crosshairsSize, centerX + penWidth, centerY + crosshairsSize};
				FillRect(hdcMem, &rect, hBrush);

				negativeRect = {centerX - penWidth, centerY - penWidth, centerX + penWidth, centerY + penWidth};
				FillRect(hdcMem, &negativeRect, hBrushNegative);
				break;

			case 2:
				rect = {centerX - crosshairsSize, centerY - penWidth, centerX + crosshairsSize, centerY + penWidth};
				FillRect(hdcMem, &rect, hBrush);

				rect = {centerX - penWidth, centerY - crosshairsSize, centerX + penWidth, centerY + crosshairsSize};
				FillRect(hdcMem, &rect, hBrush);

				negativeRect = {centerX - sideLength, centerY - sideLength, centerX + sideLength, centerY + sideLength};
				FillRect(hdcMem, &negativeRect, hBrushNegative);
				break;

			case 3:
				rect = {centerX - crosshairsSize, centerY - penWidth, centerX + crosshairsSize, centerY + penWidth};
				FillRect(hdcMem, &rect, hBrush);

				rect = {centerX - penWidth, centerY - crosshairsSize, centerX + penWidth, centerY + crosshairsSize};
				FillRect(hdcMem, &rect, hBrush);

				negativeRect = {centerX - sideLength, centerY - sideLength, centerX + sideLength, centerY + sideLength};
				FillRect(hdcMem, &negativeRect, hBrushNegative);

				rect = {centerX - penWidth, centerY - penWidth, centerX + penWidth, centerY + penWidth};
				FillRect(hdcMem, &rect, hBrush);
				break;

			case 4:
				DeleteObject(hPen);
				hPen = CreatePen(PS_SOLID, penWidth, COLORS[currentColor]);
				SelectObject(hdcMem, hPen);

				SelectObject(hdcMem, hBrushNegative);
				rect = {centerX - crosshairsSize, centerY - crosshairsSize, centerX + crosshairsSize, centerY + crosshairsSize};
				Ellipse(hdcMem, rect.left, rect.top, rect.right, rect.bottom);

				SelectObject(hdcMem, hBrush);
				rect = {centerX - penWidth, centerY - penWidth, centerX + penWidth, centerY + penWidth};
				FillRect(hdcMem, &rect, hBrush);
				break;

			case 5:
				SelectObject(hdcMem, hBrush);
				rect = {centerX - penWidth, centerY - penWidth, centerX + penWidth, centerY + penWidth};
				FillRect(hdcMem, &rect, hBrush);
				break;
		}

		// select old pen
		SelectObject(hdcMem, hOldPen);

		// cleanup
		DeleteObject(hPen);
		DeleteObject(hBrush);
		DeleteObject(hBrushNegative);
	}

    // use UpdateLayeredWindow to transfer the bitmap to the layered window
    POINT ptPos = {0, 0};
    SIZE sizeWnd = {screenWidth, screenHeight};
    POINT ptSrc = {0, 0};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    HDC hdcWindow = GetDC(hwnd);
    UpdateLayeredWindow(hwnd, hdcWindow, &ptPos, &sizeWnd, hdcMem, &ptSrc, RGB(0, 0, 0), &blend, ULW_COLORKEY);

    // cleanup
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    ReleaseDC(hwnd, hdcWindow);
}

/*
 * Load settings from Windows registry
 */
void LoadSettings() {
	HKEY hKey;
	DWORD dwValue;
	DWORD dwSize;
	DWORD dwType = REG_DWORD;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT(APPNAME), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
		// load settings
		dwSize = 4;
		RegQueryValueEx(hKey, TEXT(SHAPE_SETTING), 0, &dwType, (LPBYTE)&dwValue, &dwSize);
		currentShape = (int8_t)dwValue;
		if ((currentShape < 0) || (currentShape >= numShapes)) {
			currentShape = 0;
		}

		dwSize = 4;
		RegQueryValueEx(hKey, TEXT(COLOR_SETTING), 0, &dwType, (LPBYTE)&dwValue, &dwSize);
		currentColor = (int8_t)dwValue;
		if ((currentColor < 0) || (currentColor >= numColors)) {
			currentColor = 0;
		}

		dwSize = 4;
		RegQueryValueEx(hKey, TEXT(SIZE_SETTING), 0, &dwType, (LPBYTE)&dwValue, &dwSize);
		crosshairsSize = (int8_t)dwValue;
		if ((crosshairsSize < 0) || (crosshairsSize >= MAX_CROSSHAIRS_SIZE)) {
			crosshairsSize = 10;
		}

		dwSize = 4;
		RegQueryValueEx(hKey, TEXT(THICKNESS_SETTING), 0, &dwType, (LPBYTE)&dwValue, &dwSize);
		penWidth = (int8_t)dwValue;
		if ((penWidth < 1) || (penWidth >= MAX_PEN_WIDTH)) {
			penWidth = 1;
		}

		dwSize = 4;
		RegQueryValueEx(hKey, TEXT(X_OFFSET_SETTING), 0, &dwType, (LPBYTE)&dwValue, &dwSize);
		x_offset = (int32_t)dwValue;
		if ((x_offset < max_x_offset) || (x_offset >= max_x_offset)) {
			x_offset = 0;
		}

		dwSize = 4;
		RegQueryValueEx(hKey, TEXT(Y_OFFSET_SETTING), 0, &dwType, (LPBYTE)&dwValue, &dwSize);
		y_offset = (int32_t)dwValue;
		if ((y_offset < max_y_offset) || (x_offset >= max_y_offset)) {
			y_offset = 0;
		}
	}
}

/*
 * Save current settings to Windows registry
 */
void SaveSettings() {
	HKEY hKey;
	DWORD dwValue;
	DWORD dwSize;
	DWORD dwType = REG_DWORD;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT(APPNAME), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
		dwValue = currentShape;
		RegSetValueEx(hKey, TEXT(SHAPE_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));

		dwValue = currentColor;
		RegSetValueEx(hKey, TEXT(COLOR_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));

		dwValue = crosshairsSize;
		RegSetValueEx(hKey, TEXT(SIZE_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));

		dwValue = penWidth;
		RegSetValueEx(hKey, TEXT(THICKNESS_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));

		dwValue = x_offset;
		RegSetValueEx(hKey, TEXT(X_OFFSET_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));

		dwValue = y_offset;
		RegSetValueEx(hKey, TEXT(Y_OFFSET_SETTING), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));
	}
}
