#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <TlHelp32.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// ================= IDS =================
#define ID_BTN_ANALYZE        1001
#define ID_BTN_QUICK_FIX      1002
#define ID_BTN_FULL_FIX       1003
#define ID_BTN_INCRAESE_VIRTUAL_MEMORY 1004
#define ID_BTN_RAM_CHECK      1005
#define ID_BTN_LOG_ANALYZE    1006
#define ID_BTN_RESET_CONFIG   1007
#define ID_BTN_QUICK_SCAN     1008
#define ID_BTN_FULL_SCAN      1009
#define ID_BTN_CLEAR_OUTPUT   1010

#define ID_BTN_BENCHMARK      2001
#define ID_BTN_SETTINGS       2002
#define ID_BTN_DARK_MODE      2003
#define ID_BTN_COLOR          2004
#define ID_BTN_HELP_and_FAQ   2005
#define ID_BTN_EXIT           2006

// ================= GLOBAL UI =================
HWND g_hOutput = nullptr;

// ================= VISUAL =================
HBRUSH g_bgBrush;
COLORREF g_bgColor = RGB(25, 25, 25);
COLORREF g_textColor = RGB(220, 220, 220);

// ================= LOG =================
void Log(const std::wstring& text)
{
    if (!g_hOutput) return;

    int len = GetWindowTextLengthW(g_hOutput);

    SendMessageW(g_hOutput, EM_SETSEL, len, len);
    SendMessageW(g_hOutput, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
    SendMessageW(g_hOutput, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

// ================= WINDOW PROCEDURE =================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_BTN_ANALYZE:
            Log(L"[Analyze] Clicked");
            break;

        case ID_BTN_QUICK_FIX:
            Log(L"[Quick Fix] Clicked");
            break;

        case ID_BTN_FULL_FIX:
            Log(L"[Full Fix] Clicked");
            break;

        case ID_BTN_INCRAESE_VIRTUAL_MEMORY:
            Log(L"[Virtual Memory] Clicked");
            break;

        case ID_BTN_RAM_CHECK:
            Log(L"[RAM Check] Clicked");
            break;

        case ID_BTN_LOG_ANALYZE:
            Log(L"[Log Analyze] Clicked");
            break;

        case ID_BTN_RESET_CONFIG:
            Log(L"[Reset Config] Clicked");
            break;

        case ID_BTN_QUICK_SCAN:
            Log(L"[Quick Scan] Clicked");
            break;

        case ID_BTN_FULL_SCAN:
            Log(L"[Full Scan] Clicked");
            break;

        case ID_BTN_CLEAR_OUTPUT:
            SetWindowTextW(g_hOutput, L"");
            break;

        case ID_BTN_BENCHMARK:
            Log(L"[Benchmark] Clicked");
            break;

        case ID_BTN_SETTINGS:
            Log(L"[Settings] Clicked");
            break;

        case ID_BTN_DARK_MODE:
            Log(L"[Dark Mode] Clicked");
            break;

        case ID_BTN_COLOR:
            Log(L"[Color] Clicked");
            break;

        case ID_BTN_HELP_and_FAQ:
            Log(L"[Help / FAQ] Clicked");
            break;

        case ID_BTN_EXIT:
            PostQuitMessage(0);
            break;
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}