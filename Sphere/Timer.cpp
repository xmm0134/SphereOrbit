#include "Timer.h"

Timer::Timer() : m_SecondsPerCount(0), m_DeltaTime(0), m_Prevtime(0), m_BaseTime(0) ,m_PausedTime(0), m_CurrTime(0), m_StopTime(0), Paused(0)
{
	__int64 curtime;
	QueryPerformanceFrequency((LARGE_INTEGER*)&curtime);
	this->m_SecondsPerCount = 1.0f / (double)curtime;
}

double Timer::GetDeltaTime() const
{
	return this->m_DeltaTime;
}

double Timer::TotalTime() const
{
	if (this->Paused)
	{
		return (double)((this->m_StopTime - this->m_PausedTime) - this->m_BaseTime) * this->m_SecondsPerCount;
	}
	else
	{
		return (double)((this->m_CurrTime - this->m_PausedTime) - this->m_BaseTime);
	}
}

void Timer::Tick()
{
	if (this->m_StopTime)
	{
		this->m_DeltaTime = 0.0f;
		return;
	}

	__int64 Curtime;
	QueryPerformanceCounter((LARGE_INTEGER*)&Curtime);
	this->m_CurrTime = Curtime;

	this->m_DeltaTime = ( this->m_CurrTime - this->m_Prevtime) * this->m_SecondsPerCount;

	this->m_Prevtime = Curtime;
	
	if (m_DeltaTime < 0.0f)
	{
		this->m_DeltaTime = 0.0f;
	}
}

void Timer::Reset()
{
	__int64 Curtime;
	QueryPerformanceCounter((LARGE_INTEGER*)&Curtime);

	this->m_BaseTime = Curtime;
	this->m_Prevtime = Curtime;
	this->m_StopTime = 0;
	this->Paused = false;
}

void Timer::Start()
{
	__int64 StartTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&StartTime);

	if (this->Paused)
	{
		this->m_PausedTime += (this->m_PausedTime - StartTime);

		this->m_Prevtime = StartTime;
		this->m_StopTime = 0;
		this->Paused = 0;

	}

}

void Timer::Stop()
{
	if (!this->m_StopTime) 
	{
		__int64 Curtime;
		QueryPerformanceCounter((LARGE_INTEGER*)&Curtime);

		this->m_StopTime = Curtime;
		this->Paused = true;
	}
}
