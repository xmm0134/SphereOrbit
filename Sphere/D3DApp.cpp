#pragma once
#include"D3DApp.h"
#include"imgui.h"
#include"imgui_impl_win32.h"
#include<iostream>
#include<windows.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam)) // handle imgui window events.
		return true;

	switch (message)
	{
	case WM_CLOSE:
		exit(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wparam, lparam);
		break;
	}
}

D3DApp::D3DApp(HINSTANCE hinstance, const char* windowname, int width, int height, bool Windowed)
	: Width(width), Height(height), hwnd(nullptr), hinst(hinstance), msg(), FrameTime(0), SecondsPerCount(0) , FrameCount(0)
{
	this->Registorclass();

	this->Width = width;
	this->Height = height;

	hwnd = CreateWindowEx(NULL, "Class", windowname, NULL, width / 2, height / 2, width, height, NULL, NULL, hinstance, NULL);
	ShowWindow(hwnd, SW_SHOW);

	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	this->SecondsPerCount = 1 / (double)Frequency.QuadPart;

}

D3DApp::~D3DApp()
{

}

HWND D3DApp::GetHWND()
{
	return hwnd;
}

void D3DApp::PollWindowEvents()
{
	while (PeekMessage(&msg, hwnd, NULL, NULL, PM_REMOVE))
	{
		DispatchMessageA(&msg);
		TranslateMessage(&msg);
	}
}

void D3DApp::Run()
{
	this->CalculateElapsedFrames();
}

void D3DApp::Registorclass()
{
	WNDCLASSEX wnd;
	ZeroMemory(&wnd, sizeof(wnd));
	wnd.hInstance = hinst;
	wnd.lpszClassName = "Class";
	wnd.lpfnWndProc = Wndproc;
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.style = CS_HREDRAW | CS_VREDRAW;
	wnd.cbSize = sizeof(wnd);

	RegisterClassExA(&wnd);
}

void D3DApp::CalculateElapsedFrames()
{
	int FrameCount = 0;
	LARGE_INTEGER curtime;
	QueryPerformanceCounter(&curtime);

	double TimeElapsed = (curtime.QuadPart - FrameTime) * SecondsPerCount;

	FrameCount++;

	if (TimeElapsed > 1.0f) 
	{
		FPS = FrameCount / TimeElapsed;
		FrameCount = 0;
		FrameTime = (double)curtime.QuadPart;
	}
}
