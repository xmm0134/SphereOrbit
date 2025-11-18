#pragma once
#include<windows.h>


class Timer 
{
private:
	double m_SecondsPerCount;
	double m_DeltaTime;

	__int64 m_Prevtime;
	__int64 m_BaseTime;
	__int64 m_PausedTime;
	__int64 m_CurrTime;
	__int64 m_StopTime;

	bool Paused;
public:

	Timer();
	double GetDeltaTime() const;
	double TotalTime() const;
	
	void Tick();
	void Reset();
	void Start();
	void Stop();

};