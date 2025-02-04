#include <iostream>
#include <time.h>
#include <csignal> // для обратки сигнала ctrl+c
#include <stdlib.h>
#include <stdlib.h> // rand(), srand()

// for uart-fft-shift application

#include "xserial.hpp"

#include "AtomicQueue.hpp"
#include "Audio_player_v2.hpp"
#include "Audio_recorder_v2.hpp"
#include "CodeDurationTimer.hpp" // для отладки

// for com_port_halfduplex_uart_fft_test_with_hex_data

#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cstring>

#define SAMP_RATE 24000			//частота дискретизации
#define BITS_PER_SAMP 16		//бит на семпл
#define ATOMIC_QUEUE_TYPE short	//тип атомик очереди (лучше всегда ставить short)

AtomicQueue <ATOMIC_QUEUE_TYPE> sndMicBuffer(10'000'000);    //буфер-очередь записи с микрофона
AtomicQueue <ATOMIC_QUEUE_TYPE> sndDinBuffer(10'000'000);    //буфер-очередь для воспроизведения на динамик

Audio_recorder	<ATOMIC_QUEUE_TYPE> recorder(&sndMicBuffer, BITS_PER_SAMP, SAMP_RATE);	// класс звукозаписи
Audio_player	<ATOMIC_QUEUE_TYPE> player(&sndDinBuffer, BITS_PER_SAMP, SAMP_RATE);	// класс звуковоспроизведения

xserial::ComPort com; //класс для работы с com портом

bool stop_flag = 0;	//флаг остановки, измение флага произв обработчик прерывания

void stopSignalTestDirectSound(int signal) {	//обработчик сигнала	
	if (signal == SIGINT) {
		stop_flag = 1;
	}
}

void choose_and_open_comPort() { //выбираем и открываем ком порт
	int portNumber = 0;
	printf("List of available ports:\n");
	com.printListSerialPorts();
	printf("select the required port: ");
	scanf("%d", &portNumber);
	//portNumber = 5; //debug
	com.open(portNumber, 921600);
	if (!com.getStateComPort()) { printf("Error: port not open!\n"); exit(-1); }
	portNumber = com.getNumComPort();	//получить номер открытого порта
	printf("Open port number: %d\n", portNumber);
}

void choose_dalay_before_start(int& delay) {
	const int size = 2;	// 2-e цифры в eводимом числе
	char ch[size+1] = {0};
	bool flag_incorrect_input = 0;	//1 если ошибка
	while (1) {
		printf("Enter a delay from 0 to 30 seconds: ");
		std::cin.ignore(32767, '\n');
		std::cin.get(ch, size+1);
		for (int i = 0; i < size; i++) 
			if (!(ch[i]>= 48 && ch[i] <= 57 || ch[i] == 0)) flag_incorrect_input = 1; //проверка что строка содержит только цифры
		delay = atoi(ch); //перевод строки в число
		if (delay >= 0 && delay <= 30 && flag_incorrect_input == 0) break; //выходим из цикла, если задержка указана корректно
		else printf("Incorrect input. Enter the data correctly\n");
	}
}

const int elementsInQueueInSec = player.getSampleRate(); //кол-во элементов в очереди

void insert_delay_in_atomic(const int delay) {
	char element = 0; //Элемент который кладем в очередь X раз
	for (unsigned int i = 0; i < delay * elementsInQueueInSec; i++) 
		sndDinBuffer.Push(element);
}

void comPort_duplex_loop_DirectSound() {		//тест DS на самого себя через uart
	signal(SIGINT, stopSignalTestDirectSound);	//устанавливает обработчик сигнала SignalHandler на обработку SIGIN (ctrl
	int dalay_before_start = 0;					//задержка между записью звука и воспроизведением

	const int tx_rx_BufferSize = SAMP_RATE/5;			//кол-во семплов в буфере
	ATOMIC_QUEUE_TYPE tx_rx_Buffer[tx_rx_BufferSize];	//буфер на приём и передачу uart
	choose_and_open_comPort();							//выбираем и открываем ком порт
	choose_dalay_before_start(dalay_before_start);		//ввод задержки

	recorder.start();		//запуск записи с микро
	player.start();			//запуск плеера
	clock_t timer = clock();// засекает время для вывода инфы в основном цикле
	if (dalay_before_start != 0) insert_delay_in_atomic(dalay_before_start); //Вставляем нулевые элементы в атомик для задержки

	time_t work_time_start = time(NULL);	//засекаем время работы
	time_t total_time_sec = 0;
	fprintf(stderr, "main: start\n");
	while (stop_flag == 0) {
		if (sndMicBuffer.Size() > tx_rx_BufferSize) {
			for (int i = 0; i < tx_rx_BufferSize; i++) { //перекладываем из атомика в буфер на отправку
				sndMicBuffer.Pull(tx_rx_Buffer[i]);
			}
			com.write((char*)tx_rx_Buffer, sizeof(tx_rx_Buffer));	//отправляем семплы
			memset(tx_rx_Buffer, 0, sizeof(tx_rx_Buffer));			//зануляем буфер после отправки (если выдернуть uart устройтсво, то через незануленный буфер будем слышать сами себя)
			com.read((char*)tx_rx_Buffer, sizeof(tx_rx_Buffer));	//получаем семплы
			for (int i = 0; i < tx_rx_BufferSize; i++) {			//перекладываем из принятого буфера в атомик на динамик
				sndDinBuffer.Push(tx_rx_Buffer[i]);
			}
		}
		if (clock() - timer > CLOCKS_PER_SEC * 1) { //вывод инфы раз в Х сек
			total_time_sec = time(NULL) - work_time_start;
			printf("work time: %lld m, %lld s\t\t", total_time_sec / 60, total_time_sec % 60);
			printf("mic size: %d\t", sndMicBuffer.Size());
			printf("audio size: %d\tdelay %.1f sec\n", sndDinBuffer.Size(), (float)sndDinBuffer.Size() / elementsInQueueInSec);
			timer = clock();
		}
	}

	recorder.stop();   //остановка записи
	player.stop();
	printf("comPort_duplex_loop_DirectSound() is completed correctly! Exit...\n\n\n");
}

void comPort_halfduplex_loop_DirectSound() { // тест DS на самого себя через uart в режиме полудуплекса
    signal(SIGINT, stopSignalTestDirectSound); // обработчик сигнала SIGINT (ctrl+c)

    const int tx_rx_BlockSize = 1024; // размер блока для передачи/приёма через UART
    const int uart_wordSize = 2048; // размер блока для передачи/приёма через UART
    const int tx_rx_BufferSize = SAMP_RATE / 5; // размер временного буфера для обработки

    ATOMIC_QUEUE_TYPE tx_rx_Buffer[tx_rx_BufferSize]; // временный буфер типа short
    short uart_Block[tx_rx_BlockSize];    // блок для передачи/приёма UART типа short
	int uart_Word[uart_wordSize];

    choose_and_open_comPort(); // выбираем и открываем COM порт

    recorder.start();  // запуск записи с микрофона
    player.start();    // запуск плеера

    clock_t timer = clock();            // для вывода информации раз в X секунд
    time_t work_time_start = time(NULL); // засекаем время работы
    time_t total_time_sec = 0;

    bool is_reading = false; // флаг для отслеживания текущего режима

    fprintf(stderr, "main: start\n");

    while (stop_flag == 0) {
        // Условие: буфер микрофона должен содержать данных не меньше, чем tx_rx_BlockSize
        if (sndMicBuffer.Size() >= tx_rx_BlockSize) {
            is_reading = false; // переходим в режим записи

            // Перекладываем данные из sndMicBuffer в uart_Block
            for (int i = 0; i < tx_rx_BlockSize; i++) {
                sndMicBuffer.Pull(uart_Block[i]);
            }
 
            memset(uart_Word, 0, sizeof(uart_wordSize));
            for (int i = 0; i < uart_wordSize; i++) {
                uart_Word[i*2]= uart_Block[i];
            }

            // Отправляем блок данных по UART
            com.write((char*)uart_Word, sizeof(uart_wordSize));

            // Очищаем блок после отправки (по аналогии с вашим кодом)
            memset(uart_Word, 0, sizeof(uart_wordSize));

            // Ждём, пока UART получит данные размером tx_rx_BlockSize
            size_t bytesRead = 0;
            is_reading = true; // переходим в режим чтения
            while (bytesRead <= sizeof(tx_rx_BlockSize) && stop_flag == 0) {
				// printf("%u %u\n", sizeof(tx_rx_BlockSize), bytesRead);
				// printf("%d\n", com.readByte());
                unsigned long readed_bytes = com.read((char*)uart_Word, sizeof(uart_wordSize));
				bytesRead += sizeof(uart_wordSize);
				// printf("%u %u\n", sizeof(tx_rx_BlockSize), bytesRead);
            }

            memset(uart_Block, 0, sizeof(tx_rx_BlockSize));
            for (int i = 0; i < tx_rx_BlockSize; i++) {
                uart_Block[i]= uart_Word[i*2];
            }

            // Передаём принятые данные из uart_Block в sndDinBuffer
            for (int i = 0; i < tx_rx_BlockSize; i++) {
                sndDinBuffer.Push(uart_Block[i]);
            } 
        }

        // Выводим информацию о состоянии раз в секунду
        if (clock() - timer > CLOCKS_PER_SEC * 1) {
            total_time_sec = time(NULL) - work_time_start;
            printf("work time: %lld m, %lld s\t\t", total_time_sec / 60, total_time_sec % 60);
            printf("mic size: %d\t", sndMicBuffer.Size());
            printf("audio size: %d\tdelay %.1f sec\t", sndDinBuffer.Size(), (float)sndDinBuffer.Size() / elementsInQueueInSec);
            printf("mode: %s\n", is_reading ? "READING" : "WRITING");
            timer = clock();
        }
    }

    recorder.stop(); // остановка записи
    player.stop();
    printf("comPort_halfduplex_loop_DirectSound() is completed correctly! Exit...\n\n\n");
}


void printInfo(time_t* work_time_start) {
	time_t total_time_sec = time(NULL) - *work_time_start;
	printf("work time: %lld m, %lld s\t\t", total_time_sec / 60, total_time_sec % 60);
	printf("mic size: %d\t", sndMicBuffer.Size());
	printf("audio size: %d\tdelay %.1f sec\n", sndDinBuffer.Size(), (float)sndDinBuffer.Size() / elementsInQueueInSec);
}

void testDirectSound() {						//тест DS на самого себя микрофон - динамик
	signal(SIGINT, stopSignalTestDirectSound);	//устанавливает обработчик сигнала SignalHandler на обработку SIGIN (ctrl+C)
	ATOMIC_QUEUE_TYPE atomic_element;			//текущий байт звуковой информации (необязательно что это семпл, мб часть семпла)
	int dalay_before_start = 0;					//задержка между записью звука и воспроизведением

	//choose_dalay_before_start(dalay_before_start);	//ввод задержки
	//insert_delay_in_atomic(dalay_before_start);		//установка задержки (кидает x нулевых семплов в очередь динамика)1

	recorder.start();	//запуск записи
	player.start();		//запуск плеера

	fprintf(stderr, "main: start\n");

	clock_t timer = clock();				// засекает время для вывода инфы в основном цикле
	time_t work_time_start = time(NULL);	//хранит время начала работы DS

	while (stop_flag == 0) {						//Пока не получен сигнал ctrl+c
		while (sndMicBuffer.Pull(atomic_element)) {	//получаем байт или семлп с микро
			sndDinBuffer.Push(atomic_element);		//закидываем байт или семлп на динамик
		}
		if (clock() - timer > CLOCKS_PER_SEC * 1) {	//вывод инфы раз в Х сек
			printInfo(&work_time_start);			//печатает отладочную информацию
			timer = clock(); //сброс таймера
		}
		Sleep(1);
	}

	recorder.stop();
	player.stop();

	printf("testDirectSound() completed successfully.\n\n");	//сообщение об успешном завершении функции
}

//замер скорости передачи и приёма секудны звука при различном битрейте
void uart_library_throughput_test() {
	srand(time(NULL));
	choose_and_open_comPort(); //внутри указать требуемую скорость интерфейса UART
	CodeDurationTimer code_timer;
	const int countTests = 5;
	int audioBitrates[5] = { 8, 16, 24, 32, 48 }; //кбит\с
	const int txRxArraysSize = 100'000;
	byte* txBytes = (byte*)malloc(txRxArraysSize);
	byte* rxBytes = (byte*)malloc(txRxArraysSize);
	
	for (int i = 0; i < txRxArraysSize; i++) { //заполняем массив на передачу случайными байтами
		txBytes[i] = (char)(rand() % 255);
	}

	for (int i = 0; i < countTests; i++) {
		for (int j = 0; j < 3; j++) {
			memset(rxBytes, 0, txRxArraysSize); //зануляем буфер приема
			code_timer.startTimer();
			com.write((char*)txBytes, audioBitrates[i] * 1000);
			com.read((char*)rxBytes, audioBitrates[i] * 1000);
			code_timer.stopTimer();
			std::cout << audioBitrates[i] << " kBytes transferred for "
				<< code_timer.getCodeExecutionTime() / 1E9 << "s \t"
				//<< "bytesToRead: "<< com.bytesToRead() 
				<< "\tdata correctness: is " 
				<< ((memcmp(txBytes, rxBytes, audioBitrates[i] * 1000) == 0) ? "OK!" : "BAD!")	
				<< '\n';
		}
		std::cout << "==============\n";
	}
}

AtomicQueue<short> testAtomicQueueShort(100'000);
AtomicQueue<char> testAtomicQueueChar(100'000);
//тест за сколько времени происходит загрузка и выгрузка 1 сек звука в атомик
void testSpeedAtomicShort() {
	srand(time(NULL));
	CodeDurationTimer code_timer;
	const int count_samples = 24000;
	
	short* txSamples = (short*)malloc(sizeof(short) * count_samples);
	short* rxSamples = (short*)malloc(sizeof(short) * count_samples);

	for (int i = 0; i < count_samples; i++) {
		txSamples[i] = (short)(rand() % 32000);
	}

	code_timer.startTimer(); 
	for (int i = 0; i < count_samples; i++) { 
		testAtomicQueueShort.Push(txSamples[i]); //загружаем в атомики
	}
	for (int i = 0; i < count_samples; i++) {
		testAtomicQueueShort.Pull(rxSamples[i]); //выгружаем из атомиков
	}
	code_timer.stopTimer();
	code_timer.printfCodeExecutionTime();
	std::cout << "data correctness: is "
		<< (memcmp(txSamples, rxSamples, count_samples*sizeof(short)) == 0 ? "OK" : "BAD") << '\n';
}

void testSpeedAtomicChar() {
	srand(time(NULL));
	CodeDurationTimer code_timer;
	const int count_samples = 48000;

	char* txSamples = (char*)malloc(count_samples);
	char* rxSamples = (char*)malloc(count_samples);

	for (int i = 0; i < count_samples; i++) {
		txSamples[i] = (char)(rand() % 255);
	}

	code_timer.startTimer();
	for (int i = 0; i < count_samples; i++) {
		testAtomicQueueChar.Push(txSamples[i]); //загружаем в атомики
	}
	for (int i = 0; i < count_samples; i++) {
		testAtomicQueueChar.Pull(rxSamples[i]); //выгружаем из атомиков
	}
	code_timer.stopTimer();
	code_timer.printfCodeExecutionTime();
	std::cout << "data correctness: is "
		<< (memcmp(txSamples, rxSamples, count_samples) == 0 ? "OK" : "BAD") << '\n';
}

FILE *LOG_FILE;

void read_file(const char *file_name, int **words, size_t *readed_size)
{
    FILE *file;
    size_t capacity = 2048;
    *readed_size = 0;

    file = fopen(file_name, "r");
    if (!file) {
        fprintf(stderr, "Error, can't open file(%s)", file_name);
    }
    *words = (int*)malloc(capacity * sizeof(int));
    unsigned address, word;

    while (fscanf(file, "@%4X %8X\n", &address, &word) == 2) {
        (*readed_size)++;
        // printf("readed_size(%u), read @%04X %08X\n", *readed_size, address, word);
        if ((*readed_size) > capacity) {
            int *tmp = (int*)malloc((capacity * sizeof(int)) << 2);
            memcpy(tmp, *words, capacity * sizeof(int));
            free(words);
            *words = tmp;
            capacity = capacity << 2;
        }
        (*words)[(*readed_size) - 1] = word;
    }
    fclose(file);
}

void test(const std::string& file_name)
{
	LOG_FILE = fopen("log.txt", "w"); 
    printf("%s is running...\n\n\n", __FUNCTION__);

    choose_and_open_comPort();

    int *tx_words = NULL;
    int *rx_words = NULL;
	char *rx_bytes = NULL;
	char *tx_bytes = NULL;
    size_t readed_size = 0;

    read_file(file_name.c_str(), &tx_words, &readed_size);
	printf("readed_size=%lu\n", readed_size);
	// fprintf(LOG_FILE, "readed_size=%lu\n", readed_size);

    rx_words = (int*)malloc(readed_size * sizeof(int));
	memset(rx_words, 0, readed_size * sizeof(int));
	rx_bytes = (char*)&rx_words[0];
	tx_bytes = (char*)&tx_words[0];

	// com.flushRxAndTx(); 

	std::cout << "Reset FPGA and press any key." << std::endl; 
	std::cin.get();



    clock_t timer = clock();                
    time_t work_time_start = time(NULL);    
    time_t total_time_sec = 0;

	size_t writed = 0;
	size_t readed = 0;

    while (readed != readed_size * sizeof(int)) {
        if (writed != readed_size * sizeof(int) && !com.write((char*)(tx_bytes + writed++), 1)) {
            std::cerr << "Error: Failed to write to COM port for batch " << '\n';
            break;
        }

		readed += com.read(rx_bytes + readed, readed_size * sizeof(int) - readed);
    }

	for (size_t i = 0 ; i < readed_size; i+=2) {
		// fprintf(LOG_FILE, "@%04x %08x\n", i, rx_words[i]);
		fprintf(LOG_FILE, "%d\n", rx_words[i]);
	}

	free(tx_words);
	free(rx_words);

	fclose(LOG_FILE); 
    printf("%s is completed correctly! Exit...\n\n\n", __FUNCTION__);
}



// test_shift()
// {
// 	mic(2048) -> short -> int
// 	test_shift()
// 	int-> short -> din()

// }

// void hw_fpga_run() { 
// 	LOG("%s is running\n\n\n", __FUNCTION__); 

// 	int buff_delay = 0; 

// 	choose_and_open_comPort(); 
// 	choose_dalay_before_start(buff_delay);

// 	const int samp_rate = 24000; 
// 	const int bits_per_samp = 8; 
	
// 	hw_fpga::instance(samp_rate, bits_per_samp, com, buff_delay); 
// }

int main(int argc, const char* argv[]) { 
	std::cout << "Serial Win 10. v1.0" << std::endl; 

	//SetConsoleCP(1251);
	//SetConsoleOutputCP(1251);

	//testSpeedAtomicShort();		//тест за сколько времени происходит загрузка и выгрузка 1 сек звука в атомик
	//testSpeedAtomicChar();		//тест за сколько времени происходит загрузка и выгрузка 1 сек звука в атомик
	//uart_library_throughput_test();	//проверка скорости на передачу и прием
	//testDirectSound();			// проверка DS

	//comPort_duplex_loop_DirectSound();	//тест DS на самого себя через uart
	

	// if (argc > 1) { 
	// 	const std::string mode = argv[1];

	// 	if (mode == "ffthex") { 
	// 		std::cout << "Mode: ffthex. Param: " << argv[2] << std::endl; 

	// 		if (strlen(argv[2]) > 0) {  
	// 			test(argv[2]);  
	// 			// com_port_halfduplex_uart_fft_test_with_hex_data(argv[2]);  
	// 		} else { 
	// 			std::cerr << "Require fname. strlen(argv[2]) = 0" << std::endl; 
	// 		}
	// 	} 
	// } else { 
	// 	comPort_halfduplex_loop_DirectSound(); 
	// }
	
	// hw_fpga_run(); 

	return 0;
}