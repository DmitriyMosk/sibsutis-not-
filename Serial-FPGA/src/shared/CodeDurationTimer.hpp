#include<iostream>
#include<Windows.h>
#include<chrono>
#include<algorithm>


class CodeDurationTimer {
public:
	void startTimer();
	void stopTimer();
	void printfCodeExecutionTime();
	unsigned long int getCodeExecutionTime();
private:
	using clock_t = std::chrono::high_resolution_clock;
	std::chrono::time_point<clock_t> m_startTime;
	std::chrono::time_point<clock_t> m_endTime;
	std::chrono::nanoseconds m_durationTime_ns;
	unsigned long int m_int_durationTime_ns = 0;
	//static const int m_minimum_execution_time_timer_code{ 0 }; //минимальная задержка выполнения кода таймера ????
};