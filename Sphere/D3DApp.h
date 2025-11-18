#pragma once
#include<iostream>
#include<optional>
#include<windows.h>

class D3DApp
{
protected:
	int Width;
	int Height;
	HWND hwnd;
	HINSTANCE hinst;
	MSG msg;
	RECT wr;
	double FrameTime;
	double SecondsPerCount;
	int FrameCount;

public: 
	float FPS = NULL;

public:
	D3DApp(HINSTANCE hinstance, const char* WindowName, int width, int height, bool Windowed);
	~D3DApp();
	HWND GetHWND();
	void PollWindowEvents();
	void Run();

private:
	void Registorclass();
	void CalculateElapsedFrames();
};