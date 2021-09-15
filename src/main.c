#include "wad.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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
int bitmap_memory_size;

wad3_t w3;

static void
resizeDIBSection(int width, int height)
{
    BitmapWidth = width;
    BitmapHeight = height;

    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; /* top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    int min_bitmap_memory_size = width*height*4;

    if (bitmap_memory_size < min_bitmap_memory_size) {
        if (bm_memory)
            VirtualFree(bm_memory, 0, MEM_RELEASE);
        bitmap_memory_size = min_bitmap_memory_size*2;
        bm_memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
    }
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

// TODO: separate height calculation from actual rendering
static int
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
    int padding = 5;
    int tx_col = padding;
    int tx_row = padding;
    int max_tx_height = 0;
    for (i = 0; i < w3.textures_len; i++) {
        uint8_t * mip0 = w3.textures[i].mip[0];
        uint8_t * palette = w3.textures[i].palette;
        int nr = w3.textures[i].nHeight;
        int nc = w3.textures[i].nWidth;

        if (tx_col + nc > BitmapWidth) {
            tx_row += max_tx_height + padding;
            tx_col = padding;
            max_tx_height = 0;
        }
#if 0
        if (tx_row > BitmapHeight) {
            break;
        }
#endif

        max_tx_height = (nr > max_tx_height) ? nr : max_tx_height;

        if (tx_row < BitmapHeight) {
            // TODO: find a way to memcpy, would be faster
            for (int r = 0; r < nr; r++) {
                for (int c = 0; c < nc; c++) {
                    int red     = palette[mip0[r*nc + c]*3+0];
                    int green   = palette[mip0[r*nc + c]*3+1];
                    int blue    = palette[mip0[r*nc + c]*3+2];
                    set_pixel(c + tx_col, r + tx_row, red, green, blue);
                }
            }
        }

        tx_col += nc + padding;
    }
    tx_row += max_tx_height + padding;
    return tx_row;
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
    SCROLLINFO si; 

    static BOOL fScroll;

    static int yMinScroll;       // minimum vertical scroll value 
    static int yCurrentScroll;   // current vertical scroll value 
    static int yMaxScroll;       // maximum vertical scroll value 

    (void) yMinScroll;
    (void) yCurrentScroll;
    (void) yMaxScroll;
    (void) fScroll;
    (void) si;

    switch(msg) {

        case WM_CREATE:
        {
            // TODO: add file open menu
            w3 = readWAD(".\\data\\halflife.wad");

            fScroll = FALSE; 

            yMinScroll = 0; 
            yCurrentScroll = 0; 
            yMaxScroll = 0; 

        } break;

        case WM_SIZE:
        {
            int xNewSize = LOWORD(lParam); 
            int yNewSize = HIWORD(lParam); 
            (void) xNewSize;
            (void) yNewSize;

#if 1
            int bmHeight = render_textures();
            yMaxScroll = max(bmHeight - yNewSize, 0); 
            yCurrentScroll = min(yCurrentScroll, yMaxScroll); 
            si.cbSize = sizeof(si); 
            si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS; 
            si.nMin   = yMinScroll; 
            si.nMax   = bmHeight; 
            si.nPage  = yNewSize; 
            si.nPos   = yCurrentScroll; 
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE); 
#endif

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

        case WM_VSCROLL: 
        { 
            int xDelta = 0; 
            int yDelta;     // yDelta = new_pos - current_pos 
            int yNewPos;    // new position 

            switch (LOWORD(wParam)) { 
                // User clicked the scroll bar shaft above the scroll box. 
                case SB_PAGEUP: 
                    yNewPos = yCurrentScroll - 50; 
                    break; 

                // User clicked the scroll bar shaft below the scroll box. 
                case SB_PAGEDOWN: 
                    yNewPos = yCurrentScroll + 50; 
                    break; 

                // User clicked the top arrow. 
                case SB_LINEUP: 
                    yNewPos = yCurrentScroll - 5; 
                    break; 

                // User clicked the bottom arrow. 
                case SB_LINEDOWN: 
                    yNewPos = yCurrentScroll + 5; 
                    break; 

                // User dragged the scroll box. 
                case SB_THUMBPOSITION: 
                    yNewPos = HIWORD(wParam); 
                    break; 

                default: 
                    yNewPos = yCurrentScroll; 
            } 

            // New position must be between 0 and the screen height. 
            yNewPos = max(0, yNewPos); 
            yNewPos = min(yMaxScroll, yNewPos); 

            // If the current position does not change, do not scroll.
            if (yNewPos == yCurrentScroll) 
                break; 

            // Set the scroll flag to TRUE. 
            fScroll = TRUE; 

            // Determine the amount scrolled (in pixels). 
            yDelta = yNewPos - yCurrentScroll; 

            // Reset the current scroll position. 
            yCurrentScroll = yNewPos; 

            // Scroll the window. (The system repaints most of the 
            // client area when ScrollWindowEx is called; however, it is 
            // necessary to call UpdateWindow in order to repaint the 
            // rectangle of pixels that were invalidated.) 
            ScrollWindowEx(hwnd, -xDelta, -yDelta, (CONST RECT *) NULL, 
                (CONST RECT *) NULL, (HRGN) NULL, (PRECT) NULL, 
                SW_INVALIDATE); 
            UpdateWindow(hwnd); 

            // Reset the scroll bar. 
            si.cbSize = sizeof(si); 
            si.fMask  = SIF_POS; 
            si.nPos   = yCurrentScroll; 
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE); 

        } break; 

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
        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
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
