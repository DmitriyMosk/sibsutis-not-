#include <iostream> 
#include <cassert>
#include <csignal> 
#include <cstring> 
#include <fstream> 
#include <cstdlib>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <thread>

/**
 * Self libs 
 */
#include "AtomicQueue.hpp" 
#include "Audio_player_v2.hpp"
#include "Audio_recorder_v2.hpp" 
#include "xserial.hpp" 
#include "file_open_close.h" 
#include "ToneGenerator.hpp"
#include "ctypes.hpp"
#include "uart.hpp"

/**
 * Inital definitions 
 */
#define BAUDRATE                921600
#define SAMP_RATE               14000   		        //частота дискретизации
#define BITS_PER_SAMP           16		                //бит на семпл (битрейт)
#define UART_FFT_BLOCK_SIZE     2048                    //TODO: docs

#define UART_DT                 char    
#define ATOMIC_FPGA_QUEUE_TYPE  short 	                //тип атомик очереди

#define FPGA_HW_LOG_FILE        "fpga_logs.txt"         //Файл с логами
#define FPGA_HW_SPECTRE_DATA    "gnu_plot_spectre.txt"  //Файл с FFT спектром 
#define FPGA_HW_SHIFT_DATA      "gnu_plot_shift.txt"    //Файл с FFT-shift сигналом

/**
 * Tests 
 */
// #define FPGA_TEST_SYMBOLIC           
// #define FPGA_TEST_SIZED_COM_SHIFT    
// #define FPGA_TEST_TONE_GENERATOR 
// #define FPGA_TEST_ETC
// #define FPGA_TEST_TONE_UART_SHIFT 
#define FPGA_TEST_UART_V2

#ifdef FPGA_TEST_TONE_UART_SHIFT
void fpga_test_tone_uart_shift(); 
#endif 

uint8_t FPGA_APP_MODE;
void    fpga_app_mode_selector(); 

typedef void ASYNC_VOID; 

/**
 *  Пока что без Chose и т.д 
 */

const unsigned int RUNNING_PORT = 20; 
const unsigned int QUEUE_DELAY  = 5; 

enum TEST_MODE { 
    SHIFT,
    FFT,
};            

#define LOG(fmt, ...) do {                      \
    FILE *fp = fopen(FPGA_HW_LOG_FILE, "a");    \
    if (fp != NULL) {                           \
        fprintf(fp, fmt, ##__VA_ARGS__);        \
        fclose(fp);                             \
    }                                           \
} while(false);

#define assertf(condition, fmt, ...) do {       \
    if (!condition) {                           \
        fprintf(stderr, fmt, ##__VA_ARGS__);    \
        fprintf(stderr, "\n");                  \
        assert(condition);                      \
    }                                           \
} while(false);

/**
 * @brief класс для работы с com портом
 * TODO: async support give
 */
xserial::ComPort com; 

AtomicQueue <ATOMIC_FPGA_QUEUE_TYPE> snd_mic_buffer(10'000'000);
AtomicQueue <ATOMIC_FPGA_QUEUE_TYPE> snd_din_buffer(10'000'000);

bool stop_flag = false; 

void signal_handler(int signal) { 
    stop_flag = (signal == SIGINT) ? true : false;
}

/**
 * @brief Инициализация IO буфера 
 */
Audio_recorder	<ATOMIC_FPGA_QUEUE_TYPE> recorder(&snd_mic_buffer, BITS_PER_SAMP, SAMP_RATE); 
Audio_player	<ATOMIC_FPGA_QUEUE_TYPE> player(&snd_din_buffer, BITS_PER_SAMP, SAMP_RATE);

/**
 * @brief Возвращает кол-во элементов в очереди. 
 */
const int elementsInQueueInSec = player.getSampleRate();

#define DEBUG_CLOCKS

void app_live() {
    signal(SIGINT, signal_handler); 

    /**
     * TODO: docs
     * 
     * rx_tx_blocksize = 2048 [ 1024 - Real; 1024 - Imag ]
     */
    constexpr size_t rx_tx_blocksize = 1024; 
    
    short *tx_buffer = NULL;
    short *rx_buffer = NULL;

    tx_buffer = (short*) malloc(rx_tx_blocksize * sizeof(short)); 
    rx_buffer = (short*) malloc(rx_tx_blocksize * sizeof(short)); 

    memset(tx_buffer, 0, rx_tx_blocksize * sizeof(short)); 
    memset(rx_buffer, 0, rx_tx_blocksize * sizeof(short)); 

    /**
     * TODO: docs
     */
    recorder.start(); 
    player.start(); 

    /**
     * TODO: docs
     */
    size_t writed = 0; 
    size_t readed = 0; 

    char *rx_bytes(NULL), *tx_bytes(NULL);
    rx_bytes = (char*)&rx_buffer[0]; 
    tx_bytes = (char*)&tx_buffer[0];

#ifdef DEBUG_CLOCKS
    #define TIME_SIZE 2
    long my_time[TIME_SIZE];
    my_time[0] = clock();
#endif

    while(!stop_flag) { 
        //LOG("snd_mic_buffer.Size() %u", snd_mic_buffer.Size());
        if (snd_mic_buffer.Size() >= rx_tx_blocksize) { 
            memset(tx_buffer, 0, rx_tx_blocksize * sizeof(short));
            memset(rx_buffer, 0, rx_tx_blocksize * sizeof(short)); 

            for(int i = 0; i < (rx_tx_blocksize); i++) { 
                snd_mic_buffer.Pull(tx_buffer[i]);
                LOG("%i\n", tx_buffer[i]);
            }

            /**
             * 
             */
        
            writed = 0; 
            readed = 0; 
#ifdef DEBUG_CLOCKS
            my_time[0] = clock();
#endif            
            while (readed != rx_tx_blocksize * sizeof(short)) {
                if (writed != rx_tx_blocksize * sizeof(short) && !com.write((char*)(tx_bytes), rx_tx_blocksize * sizeof(short))) {
                // if (writed != rx_tx_blocksize * sizeof(short) && !com.write((char*)(tx_bytes + writed++), 1)) {
                    LOG("Error: Failed to write to COM port for batch \n");
                    break;
                }
                writed = rx_tx_blocksize * sizeof(short);
                readed += com.read(rx_bytes + readed, rx_tx_blocksize * sizeof(short) - readed);
            }
#ifdef DEBUG_CLOCKS
            my_time[1] = clock();
#endif            
            for(int i = 0; i < rx_tx_blocksize; i++) {  
                //std::cout << rx_buffer[i] << std::endl;

                if (rx_buffer[i] * (short) 10 < 36767) { 
                    snd_din_buffer.Push(rx_buffer[i] * (short) 10);
                }              
            }
#ifdef DEBUG_CLOCKS
            LOG("snd_mic_buffer.Size() = %u, time delay = %lu\n", snd_mic_buffer.Size() , my_time[1] - my_time[0]);
#endif 
        }
    }
    /**
     * TODO: docs
     */
    recorder.stop(); 
    player.stop(); 

    /**
     * TODO: docs
     */
    free(rx_buffer); 
    free(tx_buffer); 
}

#ifdef FPGA_TEST_ETC

/**
 * TODO: docs
 */
std::ofstream fpga_spectre_fd(FPGA_HW_SPECTRE_DATA);

/**
 * TODO: docs
 */
std::ofstream fpga_shift_fd(FPGA_HW_SHIFT_DATA);

void app_test(TEST_MODE tmode) { 
    assert(fpga_shift_fd.is_open()); 
    assert(fpga_spectre_fd.is_open());
    
    size_t readed_count = 0; 

    int *tx_words(NULL), *rx_words(NULL); 
	UART_DT *rx_bytes(NULL), *tx_bytes(NULL);
    
    file_read_hex("test_input_hex.txt", &tx_words, &readed_count); 

    rx_words = new int[readed_count * sizeof(int)]; 
    std::memset(rx_words, 0, readed_count * sizeof(int)); 

    LOG("readed_count=%lu\n", readed_count); 

    /**
     * First bytes data
     */
    rx_bytes = (UART_DT*)&rx_words[0]; 
    tx_bytes = (UART_DT*)&tx_words[0]; 
 
	size_t writed = 0;
	size_t readed = 0;
    com.flushRxAndTx(); 
    while (readed != readed_count * sizeof(int)) {
        if (writed != readed_count * sizeof(int) && !com.write((UART_DT*)(tx_bytes + writed++), 1)) {
            std::cerr << "Error: Failed to write to COM port for batch " << '\n';
            break;
        }

		readed += com.read(rx_bytes + readed, readed_count * sizeof(int) - readed);
    }
    com.flushRxAndTx(); 
    std::ofstream* cur_stream = (tmode == TEST_MODE::SHIFT) ? &fpga_shift_fd : &fpga_spectre_fd;

	for (size_t i = 0 ; i < readed_count; i+=2) {
        *cur_stream << rx_words[i] << "\n";
	}

    /**
     * 
     */
    free(tx_words);
    delete[] rx_words; 

    fpga_shift_fd.close();
    fpga_spectre_fd.close();

    LOG("%s is completed correctly! Exit...\n\n\n\n", __FUNCTION__); 

    std::ofstream file_spectre("fft_spectre_signal.gp"); 
    std::ofstream file_shift("fft_shift_signal.gp"); 

    switch(tmode) {  
        case TEST_MODE::FFT: 
            if (file_spectre.is_open()) { 
                file_spectre << "set terminal wxt\n";
                file_spectre << "plot '" << FPGA_HW_SPECTRE_DATA << "' using 1:2 with lines\n"; 
                file_spectre.close(); 
            }

            if (system("gnuplot -persist fft_spectre_signal.gp") != 0) { 
                LOG("Gnuplot error!\n"); 
            } else { 
                LOG("Gnuplot execited!\n"); 
            } 

            break; 
        case TEST_MODE::SHIFT: 
            if (file_shift.is_open()) { 
                file_shift << "set terminal wxt\n";
                file_shift << "plot '" << FPGA_HW_SHIFT_DATA << "' using 1 with lines\n"; 
                file_shift.close(); 
            }

            if (system("gnuplot -persist fft_shift_signal.gp") != 0) { 
                LOG("Gnuplot error!\n"); 
            } else { 
                LOG("Gnuplot execited!\n"); 
            } 

            break; 
        default: 
            break; 
    }
}

void app_test_with_atomic(TEST_MODE tmode) { 
    signal(SIGINT, signal_handler); 

    int *data(nullptr);

    assert(fpga_shift_fd.is_open()); 
    assert(fpga_spectre_fd.is_open());
    
    size_t readed_count = 0; 
    
    file_read_hex("test_input_hex.txt", &data, &readed_count); 
    
    assertf(readed_count == 2048, "(size_t) readed_count = %lu", readed_count);

    constexpr size_t rx_tx_blocksize = 2048; 
    
    /**
     * Записываем в ATOMIC
     */
    ATOMIC_FPGA_QUEUE_TYPE tx_buffer[rx_tx_blocksize];
    ATOMIC_FPGA_QUEUE_TYPE rx_buffer[rx_tx_blocksize];

    for (int i = 0; i < rx_tx_blocksize; i++) { 
        tx_buffer[i] = data[i];
    }

    /**
     * validate
     */

    for (int i = 0; i < rx_tx_blocksize; i++) { 
        assertf(tx_buffer[i] == data[i], "[i] = %d tx_buffer = %d; data = %d", i, tx_buffer[i], data[i]);
    }

    /**
     * 
     */
    UART_DT *rx_bytes(NULL), *tx_bytes(NULL);

    rx_bytes = (UART_DT *)&rx_buffer[0]; 
    tx_bytes = (UART_DT *)&tx_buffer[0]; 

    player.start();
    recorder.start(); 

    // std::thread th1(uart_din_thread_fn, com);
    // std::thread th2(uart_mic_thread_fn, com); 

    // th1.join(); 
    // th2.join(); 


    // char val; 

    // while(!stop_flag) { 

        

    //     while (snd_mic_buffer.Pull(val)) {	//получаем байт или семлп с микро
	// 		snd_din_buffer.Push(val);		//закидываем байт или семлп на динамик
	// 	}

    //     // snd_mic_buffer.Pull(val); 
    //     // //LOG("%d \n", val);
    //     // snd_din_buffer.Push(val);
    // }

    player.stop(); 
    recorder.stop(); 
}

#endif 

#ifdef FPGA_TEST_SYMBOLIC
    void fpga_test_symbolic_buffers() { 
        LOG("Started %s\n\n\n", __FUNCTION__); 

        char testbuff[9] = "abcdefgh";

        com.write(testbuff, 9); 

        size_t readed = 0; 

        char *tx_buff = (char *) malloc(sizeof(char) * 9); 

        while(readed != 9) { 
            readed += com.read(tx_buff, 9); 
        }

        printf("%s", tx_buff); 
    }   
#endif 

#ifdef FPGA_TEST_SIZED_COM_SHIFT
    void fpga_test_sized_com_shift() { 
        LOG("Started %s\n\n\n", __FUNCTION__); 

        int     *rx_tx_data      = nullptr; 
        size_t   rx_tx_data_size = 0; 
    
        file_read_hex("test_input_hex.txt", &rx_tx_data, &rx_tx_data_size); 

        short   *rx_tx_buff = (short *) malloc (rx_tx_data_size * sizeof(short)); 

        printf("%08X\n", rx_tx_data[2]);

        for (int i = 0; i < rx_tx_data_size; i++) { 
            rx_tx_buff[i] = rx_tx_data[i];
        }

        printf("%08X\n", rx_tx_buff[2]);

        // COM принимает char[] => 

        char    *to_buff = (char *) malloc(rx_tx_data_size * sizeof(char));

        //com.write((char*)rx_tx_buff,ка 2048); 


        /**
         * 
         */
        free(rx_tx_buff); 
        free(rx_tx_data); 

        LOG("Complete %s\n\n\n", __FUNCTION__);
    } 
#endif

#ifdef FPGA_TEST_TONE_GENERATOR     
    std::ofstream tone_file("tone500Hz.txt");

    void fpga_test_tone_generator() { 
        /**
         * Чистота тона зависит так же от player (настроек для DirectSound)
         * Например: 
         * 
         * Audio_player	<ATOMIC_FPGA_QUEUE_TYPE> player(&snd_din_buffer, BITS_PER_SAMP * 2, SAMP_RATE * 2);
         * Тут будет звучать красивый тон, ровный
         * 
         * Audio_player	<ATOMIC_FPGA_QUEUE_TYPE> player(&snd_din_buffer, BITS_PER_SAMP, SAMP_RATE);
         * Перфоратор нахой
         */
        signal(SIGINT, signal_handler); 

        constexpr long double   tone_freq       = 500.0;        // 500 Hz
        constexpr size_t        sample_size     = 10'000.0; 

        /**
         * Указатель на буфер с семплами тона
         */
        shortptr_t              buff            = nullptr;   

        /**
         * Создаём "объект"
         */
        ToneGenerator <short> Tone(tone_freq, WAVE_TYPE::SINE, SAMP_RATE, sample_size); 

        /**
         * Копируем адрес в buff
         */
        Tone.Generate(buff); 

        player.start(); 

        while(!stop_flag && buff != nullptr) { 
            for (int i = 0; i < sample_size; i++) { 
                snd_din_buffer.Push(buff[i]); 
            }
        }

        player.stop();

        if (tone_file.is_open()) { 
            for(size_t i = 0; i < sample_size; i++) { 
                tone_file << i << " " << buff[i] << "\n"; 
            }
            tone_file.close(); 
        } else { 
            std::cerr << "Error ofstream tone_file.\n"; 
        }

        // Деструктор Tone автоматом будет вызываться после заверешения функции
    }
#endif

#ifdef FPGA_TEST_UART_V2 

/**
 * Просто моё новое виденье
 */
void fpga_test_uart_v2() { 
    printf("fpga_app::%s is running...\n", __FUNCTION__);
    // создаём переменную контекста uart

    uart::uart_ctx_t* uart_context = uart::new_uart_connection(RUNNING_PORT, BAUDRATE); 
    
    recorder.start();
    player.start(); 

    while(uart::_flag_uart_running_port.load()) { 
        if (uart::ready_to_tx(uart_context) && snd_mic_buffer.Size() > uart::block_size) { 
            for (int i = 0; i < uart::block_size; ++i) { 
                short data; 
                if (snd_mic_buffer.Pull(data)) {
                    uart_context->tx_buff.Push(data); 
                }
            }
        }

        if (uart::ready_to_rx(uart_context)) { 
            for (int i = 0; i < uart::block_size; ++i) { 
                short data;
                if (uart_context->rx_buff.Pull(data)) {
                    
                    snd_din_buffer.Push(data); 
                }
            }
        }
    }

    // для синхронизации завершения. Основной поток, видимо не ждёт
    // Мейби добавить joinable ?

    if (uart::thread_join(uart_context)) { 
        printf("Successfully completed %s\n", __FUNCTION__);
        recorder.stop();
        player.stop(); 
    } else { 
        printf("Completed with error (uart_thread.joinable() != true) %s\n", __FUNCTION__);
    }
}   

#endif 

int main() { 
    /**
     * Нужно ли проверять?
     */
    assert(sizeof(ATOMIC_FPGA_QUEUE_TYPE) < static_cast<size_t>(BITS_PER_SAMP)); 
    
    /**
     * Добавляем нули для задержки
     */
    snd_din_buffer.push_zeros(QUEUE_DELAY * elementsInQueueInSec); 
    

    LOG("fpga_app::%s is running...\n", __FUNCTION__);

    /**
     * Прогонка тестов
     */
    // LOG("Run tests.. app_test()\n\n\n"); 
    // app_test(TEST_MODE::SHIFT); 

    
    //app_test_with_atomic(TEST_MODE::SHIFT);

    #ifdef FPGA_TEST_SYMBOLIC
        fpga_test_symbolic_buffers(); 
        return 0; 
    #endif 

    #ifdef FPGA_TEST_SIZED_COM_SHIFT 
        fpga_test_sized_com_shift(); 
        return 0; 
    #endif  

    #ifdef FPGA_TEST_TONE_GENERATOR     
        fpga_test_tone_generator();
        return 0;
    #endif

    #ifdef FPGA_TEST_UART_V2 
        fpga_test_uart_v2(); 
        return 0; 
    #endif
    /**
     * Tasks: 
     * 
     * - [ ] 
     * - [ ] 
     * 
     */

    /**
     * Main app
     * @brief Тут начинается запуск нашего приложения
     */

    // com.open(RUNNING_PORT, BAUDRATE);
    // assert(com.getStateComPort()); 
    // com.flushRxAndTx();

	// LOG("Open port number: %d\n", com.getNumComPort());

    // app_live(); 

    return 0; 
}

/** 
 * Тут тесты и прочее 
 */
#ifdef FPGA_TEST_TONE_UART_SHIFT
/**
 * Демонстрация сдвига с помощью тона 
 */

void fpga_test_tone_uart_shift() { 

}

#endif 
