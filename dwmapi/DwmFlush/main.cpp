//D : DwmFlush test
//R : https://docs.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmflush
//A : gyabo 2021
#include <stdint.h>
#include <vector>
#include <thread>
#include <vector>
#include <windows.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <mmreg.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dwmapi.lib")

static LRESULT WINAPI
window_msg_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto param = wParam & 0xFFF0;
	switch (msg) {
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SYSCOMMAND:
		if (param == SC_MONITORPOWER || param == SC_SCREENSAVE)
			return 0;
		break;
	case WM_IME_SETCONTEXT:
		lParam &= ~ISC_SHOWUIALL;
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main(int argc, char *argv[])
{
	auto name = "DwmFlush";
	auto w = 160 >> 3;
	auto h = 100 >> 3;
	auto sw = w * 64;
	auto sh = h * 64;

	auto instance = GetModuleHandle(NULL);
	auto style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
	auto ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	RECT rc = {0, 0, sw, sh};
	WNDCLASSEX twc = {
		sizeof(WNDCLASSEX), CS_CLASSDC, window_msg_proc, 0L, 0L, instance,
		LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)GetStockObject(BLACK_BRUSH), NULL, name, NULL
	};
	BITMAPINFOHEADER bih = {
		sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0
	};
	BITMAPINFO bi = { bih };
	RegisterClassEx(&twc);
	AdjustWindowRectEx(&rc, style, FALSE, ex_style);

	rc.right -= rc.left;
	rc.bottom -= rc.top;
	auto hwnd = CreateWindowEx(ex_style, name, name, style,
			(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
			rc.right, rc.bottom, NULL, NULL, instance, NULL);
	ShowWindow(hwnd, SW_SHOW);
	SetFocus(hwnd);
	auto hdc = GetDC(hwnd);
	void *pbuffer = nullptr;
	auto hdib = CreateDIBSection(
			NULL, &bi, DIB_RGB_COLORS, (void **)&pbuffer, 0, 0);
	auto hdibdc = CreateCompatibleDC(NULL);
	SaveDC(hdibdc);
	SelectObject(hdibdc, hdib);
	bool isActive = true;

	MSG msg;
	for (uint64_t frame = 0 ; isActive; frame++) {
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				isActive = false;
				break;
			} else {
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}

		static int index = 0;
		uint32_t color[2] = { 0xFFFF0000, 0xFF0000FF, };
		{
			auto col = color[index++ % 2];
			uint32_t *p = (uint32_t *)pbuffer;
			for (int i = 0 ; i < w * h; i++)
				p[i] = (rand() * rand());
		}


		printf("index = %d\n", index % 60);

		if (sw == w && sh == h) {
			BitBlt(hdc, 0, 0, w, h, hdibdc, 0, 0, SRCCOPY);
		} else {
			StretchBlt(hdc, 0, 0, sw, sh, hdibdc, 0, 0, w, h, SRCCOPY);
		}
		//https://docs.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmflush
		DwmFlush();
	}
}
