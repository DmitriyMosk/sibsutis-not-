#include "CodeDurationTimer.hpp"

void CodeDurationTimer::stopTimer() {
	m_endTime = clock_t::now();
	m_durationTime_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(m_endTime - m_startTime);
	m_int_durationTime_ns = m_durationTime_ns.count() /*- m_minimum_execution_time_timer_code*/; // Вычитаем время выполнения кода таймера в "холостую"
}

void CodeDurationTimer::printfCodeExecutionTime() {
	std::cout << "[TIMER] code execution time:\n"
		<< m_int_durationTime_ns / 1E9 << " s\n"
		<< m_int_durationTime_ns / 1E6 << " ms\n"
		<< m_int_durationTime_ns << " ns\n";
}

void CodeDurationTimer::startTimer() {
	m_startTime = clock_t::now(); //Изменяем время старта таймера на текущее время
}

unsigned long int CodeDurationTimer::getCodeExecutionTime() {
	return m_int_durationTime_ns;
}

//void main() {
//	CodeDurationTimer c_timer;
//	c_timer.startTimer();
//	//////////////////////
//	//ТЕСТИРУЕМЫЙ КОД
//	Sleep(120);
//	//////////////////////
//	c_timer.stopTimer();
//	c_timer.printfCodeExecutionTime();
//	std::cout << "received time: " << c_timer.getCodeExecutionTime() << '\n';
//}