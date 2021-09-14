#include "resource.h"
#include "wad.h"

#include <windows.h>
#include <wingdi.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    int r, g, b;
} color_t;

color_t bg_color = {0x80, 0x80, 0x80};

const char g_szClassName[] = "myWindowClass";

BITMAPINFO bmi;
int BitmapWidth;
int BitmapHeight;
void * bm_memory;

wad3_t w3;

static void
resizeDIBSection(int width, int height)
{
    if (bm_memory) {
        VirtualFree(bm_memory, 0, MEM_RELEASE);
    }

    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; /* top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    BitmapWidth = width;
    BitmapHeight = height;

    int bitmap_memory_size = width*height*4;
    bm_memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}

static void
set_pixel(int x, int y, int r, int g, int b)
{
    if (x < 0 || y < 0 || x >= BitmapWidth || y >= BitmapHeight)
        return;
    int bytes_per_px = bmi.bmiHeader.biBitCount / 8;
    uint8_t * pixel = (uint8_t *) bm_memory + (y*BitmapWidth + x)*bytes_per_px;
    *pixel++ = b;
    *pixel++ = g;
    *pixel++ = r;
    *pixel++ = 0x00;
}

static void
render_textures()
{
    int bytes_per_px = bmi.bmiHeader.biBitCount / 8;
    uint8_t * row = (uint8_t *) bm_memory;
    int pitch = BitmapWidth * bytes_per_px;
    for (int r = 0; r < BitmapHeight; r++) {
        uint8_t * pixel = (uint8_t *) row;
        for (int c = 0; c < BitmapWidth; c++) {
            *pixel++ = bg_color.b;
            *pixel++ = bg_color.g;
            *pixel++ = bg_color.r;
            *pixel++ = 0x00;
        }
        row += pitch;
    }

    size_t i;
    int tx_col = 0;
    int tx_row = 0;
    int max_tx_height = 0;
    for (i = 0; i < w3.textures_len; i++) {
        uint8_t * mip0 = w3.textures[i].mip[0];
        uint8_t * palette = w3.textures[i].palette;
        int nr = w3.textures[i].nHeight;
        int nc = w3.textures[i].nWidth;
        max_tx_height = (nr > max_tx_height) ? nr : max_tx_height;
        for (int r = 0; r < nr; r++) {
            for (int c = 0; c < nc; c++) {
                int red     = palette[mip0[r*nc + c]*3+0];
                int green   = palette[mip0[r*nc + c]*3+1];
                int blue    = palette[mip0[r*nc + c]*3+2];
                set_pixel(c + tx_col, r + tx_row, red, green, blue);
            }
        }
        tx_col += nc + 2;
        if (tx_col > BitmapWidth) {
            tx_row += max_tx_height + 2;
            tx_col = 0;
        }
        if (tx_row > BitmapHeight) {
            break;
        }
    }
}

static void
updateWindow(HDC context, RECT * WindowRect, int x, int y, int width, int height)
{
    (void) x;
    (void) y;
    (void) width;
    (void) height;
    int WindowWidth = WindowRect->right - WindowRect->left;
    int WindowHeight = WindowRect->bottom - WindowRect->top;

    render_textures();

    if (StretchDIBits(
        context,
        0, 0, BitmapWidth, BitmapHeight,
        0, 0, WindowWidth, WindowHeight,
        bm_memory,
        &bmi,
        DIB_RGB_COLORS, SRCCOPY
    ) == 0) {
        // TODO: handle error
        exit(EXIT_FAILURE);
    }
}

static LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {

        case WM_CREATE:
        {
            w3 = readWAD(".\\data\\halflife.wad");
        } break;

        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(hwnd, &ClientRect);
            int width = ClientRect.right - ClientRect.left; // +1?
            int height = ClientRect.bottom - ClientRect.top; // +1?
            resizeDIBSection(width, height);
            InvalidateRect(hwnd, &ClientRect, TRUE);
        } break;

#if 0
        case WM_LBUTTONDOWN:
        {
            char szFileName[MAX_PATH];
            HINSTANCE hInstance = GetModuleHandle(NULL);

            GetModuleFileName(hInstance, szFileName, MAX_PATH);
            MessageBox(hwnd, szFileName, "This program is:", MB_OK | MB_ICONINFORMATION);
        } break;
#endif

#if 1
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            int x = ps.rcPaint.left;
            int y = ps.rcPaint.top;
            int width = ps.rcPaint.right - ps.rcPaint.left; // +1?
            int height = ps.rcPaint.bottom - ps.rcPaint.top; // +1?
            RECT ClientRect;
            GetClientRect(hwnd, &ClientRect);
            updateWindow(hdc, &ClientRect, x, y, width, height);
            EndPaint(hwnd, &ps);
        } break;
#endif

        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
        } break;

        case WM_DESTROY:
        {
            wad3_destroy(w3);
            PostQuitMessage(0);
        } break;

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

    //  TODO: consider using PeekMessage
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
