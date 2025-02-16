#include <iostream> 
#include <cassert>
#include <csignal> 
#include <cstring> 
#include <fstream> 
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
/**
 * Self libs 
 */
#include "AtomicQueue.hpp" 
#include "Audio_player_v2.hpp"
#include "Audio_recorder_v2.hpp" 
#include "xserial.hpp" 
#include "protocols/uart.hpp"
#include "file_open_close.h" 
#include "ToneGenerator.hpp"

/**
 * TODO: docs
 */
#define BAUDRATE                921600
//частота дискретизации (да, это максимум)
#define SAMP_RATE               10240
//бит на семпл (битрейт)		        
#define BITS_PER_SAMP           16		       

xserial::protocol::uart<uart_dt_t> serial_ctx; 

//TODO: docs                       
AtomicQueue <uart_dt_t> snd_mic_buffer(10'000'000);
AtomicQueue <uart_dt_t> snd_din_buffer(10'000'000);

// класс звукозаписи
Audio_recorder  <uart_dt_t> recorder(&snd_mic_buffer, BITS_PER_SAMP, SAMP_RATE);	
// класс звуковоспроизведения
Audio_player    <uart_dt_t> player(&snd_din_buffer, BITS_PER_SAMP, SAMP_RATE);	

/**
 * Help message
 */
static void argumentHelp() {
    std::cout << "\n\n\nSelect app mode: \n" << \
        "\tfpga_shift_app.exe -mode={some_mode}\n" << \
        "\t\t{player}: playing binary file\n" << \
        "\t\t{shifter}: connect microphone or select tone freq and have fun!\n";
    std::cout << "Example running: fpga_shift_app.exe -mode=player" << std::endl;
}

static void argumentHelpPlayer() {
    std::cout << "\n\n\nInsert file path with flag -f={some_path} \n" << \
        "Example running: fpga_shift_app.exe -mode=player -f=./my_voice.bin" << std::endl;
}

/**
 * @brief Возвращает кол-во элементов в очереди. 
 */
const int elementsInQueueInSec = player.getSampleRate();

constexpr unsigned int RUNNING_PORT = 20; 
constexpr unsigned int QUEUE_DELAY  = 0; 

/**
 * 
 */
volatile int signal_int;

void sigint_handler(int) {
    signal_int = 1;
}


/**
 * TODO: docs
 */
static void input_harmonics_generator(long double &ref_freq) { 
    printf("Insert harmonic frequency [Hz]: "); 

    long double freq = 0; 
    std::cin >> freq; 

    ref_freq = freq; 
}

static void input_filename(std::string &ref_fname_source, std::string &ref_fname_shifted) { 
    printf("Insert filename: ");

    std::string filename;
    std::cin >> filename;

    ref_fname_shifted = filename + "_shifted.bin";
    ref_fname_source = filename + ".bin";
}

static void input_chose_port(int &portNum) { 
	printf("List of available ports:\n");
	xserial::ComPort::printListSerialPorts();
	printf("select the required port: ");
	scanf("%d", &portNum);
}

static void choose_dalay_before_start(int& delay) {
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

/**
 * Просто запись в .bin и чтение из него
 */
void binary_player  (std::string fname); 

/**
 * -> uart
 */
void uart_shifter   (bool request_file_record, bool use_tone_generator); 

int main(int argc, char* argv[]) { 
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        std::cerr << "Ошибка регистрации обработчика SIGINT\n";
        return 1;
    }

    // serial_ctx = xserial::protocol::uart<uart_dt_t>(14, BAUDRATE, UART_DEFAULT_MTU);
    // serial_ctx.open();

    // Преобразование аргументов командной строки в std::vector<std::string>
    // argv - указатель на начало массива 
    // argc - кол-во элементов => argv + argc - конец масива
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() > 1 && args.at(1).compare("-mode=player") == 0) { 
        if (args.size() < 3 || args.at(2).find("-f=") == std::string::npos) { 
            argumentHelpPlayer(); 
            return EXIT_SUCCESS;
        } else { 
            binary_player(args.at(2).substr(3));
        }
    } else if (args.size() > 1 && args.at(1).compare("-mode=shifter") == 0) { 
        uart_shifter(
            (args.size() > 2 && (args.at(2).compare("-recorder=enable") == 0)),
            (args.size() > 3 && (args.at(3).compare("-tone=enable") == 0))
        );
    } else { 
        argumentHelp();
        return EXIT_SUCCESS; 
    }

    if (serial_ctx.is_open() && serial_ctx.close()) { 
        printf("Successfully completed %s\n", __FUNCTION__);
    } else if (serial_ctx.is_open()) { 
        printf("Completed with error (uart_thread.joinable() != true) %s\n", __FUNCTION__);
    }

    printf("Exit %s with code 0\n", __FUNCTION__);
    return 0; 
}

/** 
 * Binary player 
 */

void binary_player(std::string fname) { 
    printf("Selected %s\n", __FUNCTION__);

    std::ifstream bin_file(fname, std::ios::binary); 

    if (!bin_file.is_open()) { 
        std::cerr << "[" << __FUNCTION__ << "] file open problem\n";
        return; 
    }

    std::cout << "[" << __FUNCTION__ << "] started.\n";

    uart_dt_t* buff = new uart_dt_t[UART_DEFAULT_MTU];

    player.start();

    while (bin_file.read(reinterpret_cast<char*>(buff), UART_DEFAULT_MTU * sizeof(uart_dt_t))) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        for (int i = 0; i < UART_DEFAULT_MTU; ++i) { 
            snd_din_buffer.Push(buff[i]);
        }
    }

    player.stop(); 
    delete[] buff;
}

void uart_shifter(bool request_file_record, bool use_tone_generator) { 
    if (serial_ctx.is_open()) { 
        return; 
    }

    std::cout << "Running: " << __FUNCTION__ << "\n" \
        << "Record enabled: " << ((request_file_record) ? "Yes" : "No") << "\n" \
        << "Tone generator enabled: " << ((use_tone_generator) ? "Yes" : "No") << "\n";

    std::ofstream file_out; 
    std::ofstream file_input;

    ToneGenerator<uart_dt_t>* Tone = nullptr;

    if (request_file_record) { 
        std::string str_file_name, std_file_name_shifted; 
        input_filename(str_file_name, std_file_name_shifted);

        std::cout << str_file_name << "\n"; 
        std::cout << std_file_name_shifted << "\n";
        
        file_out.open(std_file_name_shifted, std::ios::binary);
        file_input.open(str_file_name, std::ios::binary); 
        
        assert(file_input.is_open() && "File file_input error opening");
        assert(file_out.is_open() && "File file_out error opening");
    }

    uart_dt_t* buff = nullptr;

    if (use_tone_generator) { 
        long double freq = 0;
        input_harmonics_generator(freq); 

        Tone = new ToneGenerator<uart_dt_t>(freq, WAVE_TYPE::SINE, SAMP_RATE, 1024);

        buff = Tone->Generate(); 
    }
    
    if (!use_tone_generator) { 
        int bufferDelay = 0; 
        choose_dalay_before_start(bufferDelay);
        snd_din_buffer.push_zeros(bufferDelay * elementsInQueueInSec); 
    }

    int runningPort = 0; 
    input_chose_port(runningPort); 

    serial_ctx = xserial::protocol::uart<uart_dt_t>(runningPort, BAUDRATE, UART_DEFAULT_MTU);
    serial_ctx.open();

    if (!serial_ctx.is_open()) { 
        // err 
        return;
    }

    player.start(); 
    recorder.start(); 

    while (serial_ctx.is_open() && !signal_int) { 
        if (use_tone_generator) { 
            for (int i = 0; i < UART_DEFAULT_MTU; i++) { 
                uart_dt_t tx_data = buff[i]; 
                if (request_file_record) { 
                    file_input.write(reinterpret_cast<const char*>(&tx_data), sizeof(uart_dt_t)).flush(); 
                }
                
                snd_din_buffer.Push(tx_data); 
                serial_ctx.tx_buff->Push(tx_data); 
            } 
        } else { 
            uart_dt_t data; 
            if (snd_mic_buffer.Pull(data)) {
                if (request_file_record) { 
                    file_input.write(reinterpret_cast<const char*>(&data), sizeof(uart_dt_t)).flush(); 
                }
                serial_ctx.tx_buff->Push(data); 
            }
        }

        if (serial_ctx.rx_buff->Size() >= UART_DEFAULT_MTU - 1) { 
            //std::cout << serial_ctx.rx_buff->Size() << std::endl;
            for (size_t i = 0; i < UART_DEFAULT_MTU; ++i) {
                uart_dt_t data;
                if (serial_ctx.rx_buff->Pull(data)) {
                    if (request_file_record) { 
                        file_out.write(reinterpret_cast<const char*>(&data), sizeof(uart_dt_t)).flush();
                    }
                    snd_din_buffer.Push(data); 
                }
            }
        }
    }


    player.stop();
    recorder.stop();

    if (file_out.is_open()) { 
        file_out.flush();
        file_out.close(); 
    }

    if (file_input.is_open()) { 
        file_input.flush();
        file_input.close(); 
    }
}

// void binary_uart_recorder(uart::uart_ctx_t* u_ctx) { 
//     printf("Selected %s\n\n\n\n", __FUNCTION__);

//     char mode;
//     input_mode_selector(mode);

//     std::string str_file_name, std_file_name_shifted; 
//     input_filename(str_file_name);

//     std_file_name_shifted = str_file_name + "_shifted.bin";
//     str_file_name += ".bin";

//     std::ofstream file_src(str_file_name, std::ios::binary); 
//     std::ofstream file_src_shifted(std_file_name_shifted, std::ios::binary); 

//     if (!file_src.is_open() || !file_src_shifted.is_open()) { 
//         std::cerr << "[" << __FUNCTION__ << "] file open problem\n";
//         return;
//     }

//     if (mode == 'T') { 
//         long double freq;
//         input_harmonics_generator(freq); 

//         ToneGenerator <uart::uart_dt_t> Tone(freq, WAVE_TYPE::SINE, SAMP_RATE, uart::block_size); 

//         u_ctx = uart::new_uart_connection(RUNNING_PORT, BAUDRATE);

//         shortptr_t buff = nullptr;
//         Tone.Generate(buff); 

//         while (uart::_flag_uart_running_port.load()) {
//             if(uart::ready_to_tx(u_ctx)) { 
//                 for (int i = 0; i < uart::block_size; i++) { 
//                     short tx_data = buff[i]; 
//                     file_src.write(reinterpret_cast<const char*>(&tx_data), sizeof(short)); 

//                     snd_din_buffer.Push(tx_data); 
//                     u_ctx->tx_buff.Push(tx_data); 
//                 } 
//             }

//             if(uart::ready_to_rx(u_ctx)) { 
//                 for (int i = 0; i < uart::block_size; ++i) { 
//                     short data;
//                     if (u_ctx->rx_buff.Pull(data)) {
//                         file_src_shifted.write(reinterpret_cast<const char*>(&data), sizeof(short)); 
//                     }
//                 }
//             }
//         }
//     } 

//     if (mode == 'V') { 

//         u_ctx = uart::new_uart_connection(RUNNING_PORT, BAUDRATE);

//         recorder.start(); 
//         while(uart::_flag_uart_running_port.load()) { 
//             if (uart::ready_to_tx(u_ctx) && snd_mic_buffer.Size() > uart::block_size) { 
//                 for (int i = 0; i < uart::block_size; ++i) { 
//                     short data; 
//                     if (snd_mic_buffer.Pull(data)) {
//                         u_ctx->tx_buff.Push(data); 
//                         file_src.write(reinterpret_cast<const char*>(&data), sizeof(data));
//                     }
//                 }
//             }

//             if (uart::ready_to_rx(u_ctx)) { 
//                 for (int i = 0; i < uart::block_size; ++i) { 
//                     short data;
//                     if (u_ctx->rx_buff.Pull(data)) {
//                         snd_din_buffer.Push(data); 
//                         file_src_shifted.write(reinterpret_cast<const char*>(&data), sizeof(data));
//                     }
//                 }
//             }
//         }
//         recorder.stop(); 
//     }

//     file_src.close(); 
//     file_src_shifted.close(); 
// }

// void uart_shifter(uart::uart_ctx_t *u_ctx) { 
//     printf("Selected %s\n", __FUNCTION__);

//     char mode;
//     input_mode_selector(mode);

//     if (mode == 'T') { 
//         // pass
//         return;
//     }

//     if (mode == 'V') { 
//         u_ctx = uart::new_uart_connection(RUNNING_PORT, BAUDRATE);

//         recorder.start(); 
//         while(uart::_flag_uart_running_port.load()) { 
//             if (uart::ready_to_tx(u_ctx) && snd_mic_buffer.Size() > uart::block_size) { 
//                 for (int i = 0; i < uart::block_size; ++i) { 
//                     short data; 
//                     if (snd_mic_buffer.Pull(data)) {
//                         u_ctx->tx_buff.Push(data); 
//                     }
//                 }
//             }

//             if (uart::ready_to_rx(u_ctx)) { 
//                 for (int i = 0; i < uart::block_size; ++i) { 
//                     short data;
//                     if (u_ctx->rx_buff.Pull(data)) {
//                         snd_din_buffer.Push(data); 
//                     }
//                 }
//             }
//         }
//         recorder.stop(); 
//     }

//     if (mode == 'E') {
//         u_ctx = uart::new_uart_connection(RUNNING_PORT, BAUDRATE);

//         recorder.start(); 
//         while(uart::_flag_uart_running_port.load()) { 
//             if (snd_mic_buffer.Size() > uart::block_size) { 
//                 for (int i = 0; i < uart::block_size; ++i) { 
//                     short data; 
//                     if (snd_mic_buffer.Pull(data)) {
//                         snd_din_buffer.Push(data); 
//                     }
//                 }
//             }
//         }
//         recorder.stop(); 
//     }
// }

/**
 * Защита
 * 
 * 
 * 
 * тема: Защита свидетеля
 *  
 */