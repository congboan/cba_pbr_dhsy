#include"utils.h"
#include "BattleFireDirect3D12.h"
#include "Scene.h"
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"winmm.lib")

LPCTSTR sWindowClassName = L"Direct3D12RenderWindow";
LPCTSTR sWindowTitle = L"Direct3D 12 Render Window";

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	WNDCLASSEX wndClassEx;
	wndClassEx.cbSize = sizeof(WNDCLASSEX);
	wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
	wndClassEx.lpfnWndProc = WindowProc;
	wndClassEx.cbClsExtra = NULL;
	wndClassEx.cbWndExtra = NULL;
	wndClassEx.hInstance = hInstance;
	wndClassEx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClassEx.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClassEx.hbrBackground = NULL;
	wndClassEx.lpszMenuName = NULL;
	wndClassEx.lpszClassName = sWindowClassName;
	if (!RegisterClassEx(&wndClassEx)) {
		MessageBox(NULL, L"Error registering class", L"Error", MB_OK | MB_ICONERROR);
		return NULL;
	}
	RECT rect;
	rect.left = 0;
	rect.right = 1280;
	rect.bottom = 720;
	rect.top = 0;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	HWND hwnd = CreateWindowEx(NULL,
		sWindowClassName,
		sWindowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left, rect.bottom - rect.top,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!hwnd) {
		MessageBox(NULL, L"Error creating window", L"Error", MB_OK | MB_ICONERROR);
		return NULL;
	}
	ShowWindow(hwnd, nShowCmd);
	UpdateWindow(hwnd);
	InitializeDirect3D12(hwnd, 1280, 720);
	InitScene(1280, 720);

	MSG msg;
	DWORD last_time = timeGetTime();
	while (true) {
		ZeroMemory(&msg, sizeof(MSG));
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
			{
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			DWORD current_time = timeGetTime();
			DWORD deltaTime = current_time - last_time;
			last_time = current_time;
			float deltaTimeInSecond = (float)deltaTime / 1000.0f;
			WaitForPreviousFrame();
			RenderOneFrame(deltaTimeInSecond);
			Direct3DSwapBuffers();
		}
	}
	CleanUpDirect3D12();
	return 0;
}