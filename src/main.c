#include "resource.h"
#include "wad.h"

#include <windows.h>
#include <wingdi.h>

const char g_szClassName[] = "myWindowClass";

//HBITMAP g_hbmTexture;
HBITMAP g_hbmBall;
wad3_t w3;

static LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {

        case WM_CREATE:
        {
            w3 = readWAD(".\\data\\zhlt.wad");

#if 1
			g_hbmBall = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BALL));
			if(g_hbmBall == NULL)
				MessageBox(hwnd, "Could not load IDB_BALL!", "Error", MB_OK | MB_ICONEXCLAMATION);
#else
            int nWidth = 4;
            int nHeight = 4;
            UINT nPlanes = 1;
            UINT nBitCount = 24;
            unsigned char lpBits[] = {
                0xff, 0x00, 0x00,    0xff, 0x00, 0x00,    0xff, 0x00, 0x00,    0xff, 0x00, 0x00,
                0xff, 0x00, 0x00,    0xff, 0x00, 0x00,    0xff, 0x00, 0x00,    0xff, 0x00, 0x00,
                0xff, 0x00, 0x00,    0xff, 0x00, 0x00,    0xff, 0x00, 0x00,    0xff, 0x00, 0x00,
                0xff, 0x00, 0x00,    0xff, 0x00, 0x00,    0xff, 0x00, 0x00,    0xff, 0x00, 0x00,
            };

            g_hbmBall = CreateBitmap(nWidth, nHeight, nPlanes, nBitCount, lpBits);
			if(g_hbmBall == NULL)
				MessageBox(hwnd, "Could not load IDB_BALL!", "Error", MB_OK | MB_ICONEXCLAMATION);
#endif
        }
        break;

#if 0
        case WM_LBUTTONDOWN:
        {
            char szFileName[MAX_PATH];
            HINSTANCE hInstance = GetModuleHandle(NULL);

            GetModuleFileName(hInstance, szFileName, MAX_PATH);
            MessageBox(hwnd, szFileName, "This program is:", MB_OK | MB_ICONINFORMATION);
        }
        break;
#endif

#if 1
        case WM_PAINT:
        {
            BITMAP bm;
            PAINTSTRUCT ps;

            HDC hdc = BeginPaint(hwnd, &ps);

            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmOld = (HBITMAP) SelectObject(hdcMem, g_hbmBall);

            GetObject(g_hbmBall, sizeof(bm), &bm);

            BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hbmOld);
            DeleteDC(hdcMem);

            EndPaint(hwnd, &ps);
        }
        break;
#endif

        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;

        case WM_DESTROY:
            wad3_destroy(w3);
            PostQuitMessage(0);
        break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI
#ifdef TEST
WinMain_of_program_under_test
#else
WinMain
#endif
(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void) hPrevInstance;
    (void) lpCmdLine;

    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "title of my window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Msg.wParam;
}

#ifdef TEST
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MessageBox(NULL, "Goodbye, cruel world!", "Note", MB_OK);
    return 0;
}
#endif
