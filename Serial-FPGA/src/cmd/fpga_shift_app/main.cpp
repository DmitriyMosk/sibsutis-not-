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
#include <chrono>

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

volatile bool stop_flag = false; 

static void signal_handler(int signal) { 
    stop_flag = (signal == SIGINT) ? true : false;
}

/**
 * TODO: docs
 */
#define BAUDRATE                921600
//частота дискретизации
#define SAMP_RATE               20480 / 2  
//бит на семпл (битрейт)		        
#define BITS_PER_SAMP           16		          
//TODO: docs                       
AtomicQueue <uart::uart_dt_t> snd_mic_buffer(10'000'000);
AtomicQueue <uart::uart_dt_t> snd_din_buffer(10'000'000);

// класс звукозаписи
Audio_recorder  <uart::uart_dt_t> recorder(&snd_mic_buffer, BITS_PER_SAMP, SAMP_RATE);	
// класс звуковоспроизведения
Audio_player    <uart::uart_dt_t> player(&snd_din_buffer, BITS_PER_SAMP, SAMP_RATE);	

/**
 * @brief Возвращает кол-во элементов в очереди. 
 */
const int elementsInQueueInSec = player.getSampleRate();

constexpr unsigned int RUNNING_PORT = 20; 
constexpr unsigned int QUEUE_DELAY  = 0; 

typedef void t_void;

/**
 * TODO: docs
 */

t_void input_handler() { 
    
}

/**
 * TODO: docs
 */
static void input_mode_selector(char &ref_mode) { 
    char tmp_mode;

    printf("Press [T] to select tone generator mode\n");
    printf("Press [V] to select voice mode\n");
    printf("Press [E] to select voice shift mode\n");

    printf("Mode: ");
    std::cin >> tmp_mode;
    tmp_mode = std::toupper(tmp_mode);

    ref_mode = tmp_mode;
}

static void input_harmonics_generator(long double &ref_freq) { 
    printf("Insert harmonic frequency [Hz]: "); 

    long double freq = 0; 
    std::cin >> freq; 

    ref_freq = freq; 
}

static void input_filename(std::string &ref_fname) { 
    printf("Insert filename: ");

    std::string filename;
    std::cin >> filename;

    ref_fname = filename;
}

/**
 * Просто запись в .bin и чтение из него
 */
void binary_player        (std::string fname); 
void binary_uart_recorder (uart::uart_ctx_t *u_ctx); 

/**
 * Генератор тона -> uart
 */
void micro_uart_recorder  (uart::uart_ctx_t *u_ctx); 

int main(int argc, char* argv[]) { 
    if (argc > 1) { 
        //pass
    }

    double val = 0.52; 
    char val_sh = (char)val; 
    std::cout << val_sh << " " << (char)val << std::endl;

    snd_din_buffer.push_zeros(QUEUE_DELAY * elementsInQueueInSec); 

    printf("Running %s\n", __FUNCTION__);
    player.start(); 

    int mode = 0; 

    printf("Select app mode:"
        "\n(1) Binary player"    
        "\n(2) Binary shift recorder"
        "\n(3) Microphone voice shifting\n");

    printf("Mode: ");
    std::cin >> mode; 

    uart::uart_ctx_t* ctx = nullptr;

    std::string fname; 

    switch(mode) { 
        case 1:         
            input_filename(fname);
            binary_player(fname);
            break; 
        case 2: 
            binary_uart_recorder(ctx);
            break; 
        case 3: 
            micro_uart_recorder (ctx);
            break;
        default:
            uart::_flag_uart_running_port.store(false); 
            printf("Unknown app\n");
    }   

    if (ctx && uart::thread_join(ctx)) { 
        printf("Successfully completed %s\n", __FUNCTION__);
    } else { 
        printf("Completed with error (uart_thread.joinable() != true) %s\n", __FUNCTION__);
    }

    player.stop(); 

    printf("Stoped %s\n", __FUNCTION__);

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

    uart::uart_dt_t* buff = new uart::uart_dt_t[uart::block_size];

    while (bin_file.read(reinterpret_cast<char*>(buff), uart::block_size * sizeof(uart::uart_dt_t))) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        for (int i = 0; i < uart::block_size; ++i) { 
            snd_din_buffer.Push(buff[i]);
        }
    }
}

void binary_uart_recorder(uart::uart_ctx_t* u_ctx) { 
    printf("Selected %s\n\n\n\n", __FUNCTION__);

    char mode;
    input_mode_selector(mode);

    std::string str_file_name, std_file_name_shifted; 
    input_filename(str_file_name);

    std_file_name_shifted = str_file_name + "_shifted.bin";
    str_file_name += ".bin";

    std::ofstream file_src(str_file_name, std::ios::binary); 
    std::ofstream file_src_shifted(std_file_name_shifted, std::ios::binary); 

    if (!file_src.is_open() || !file_src_shifted.is_open()) { 
        std::cerr << "[" << __FUNCTION__ << "] file open problem\n";
        return;
    }

    if (mode == 'T') { 
        long double freq;
        input_harmonics_generator(freq); 

        ToneGenerator <uart::uart_dt_t> Tone(freq, WAVE_TYPE::SINE, SAMP_RATE, uart::block_size); 

        u_ctx = uart::new_uart_connection(RUNNING_PORT, BAUDRATE);

        shortptr_t buff = nullptr;
        Tone.Generate(buff); 

        while (uart::_flag_uart_running_port.load()) {
            if(uart::ready_to_tx(u_ctx)) { 
                for (int i = 0; i < uart::block_size; i++) { 
                    short tx_data = buff[i]; 
                    file_src.write(reinterpret_cast<const char*>(&tx_data), sizeof(short)); 

                    snd_din_buffer.Push(tx_data); 
                    u_ctx->tx_buff.Push(tx_data); 
                } 
            }

            if(uart::ready_to_rx(u_ctx)) { 
                for (int i = 0; i < uart::block_size; ++i) { 
                    short data;
                    if (u_ctx->rx_buff.Pull(data)) {
                        file_src_shifted.write(reinterpret_cast<const char*>(&data), sizeof(short)); 
                    }
                }
            }
        }
    } 

    if (mode == 'V') { 

        u_ctx = uart::new_uart_connection(RUNNING_PORT, BAUDRATE);

        recorder.start(); 
        while(uart::_flag_uart_running_port.load()) { 
            if (uart::ready_to_tx(u_ctx) && snd_mic_buffer.Size() > uart::block_size) { 
                for (int i = 0; i < uart::block_size; ++i) { 
                    short data; 
                    if (snd_mic_buffer.Pull(data)) {
                        u_ctx->tx_buff.Push(data); 
                        file_src.write(reinterpret_cast<const char*>(&data), sizeof(data));
                    }
                }
            }

            if (uart::ready_to_rx(u_ctx)) { 
                for (int i = 0; i < uart::block_size; ++i) { 
                    short data;
                    if (u_ctx->rx_buff.Pull(data)) {
                        snd_din_buffer.Push(data); 
                        file_src_shifted.write(reinterpret_cast<const char*>(&data), sizeof(data));
                    }
                }
            }
        }
        recorder.stop(); 
    }

    file_src.close(); 
    file_src_shifted.close(); 
}

void micro_uart_recorder(uart::uart_ctx_t *u_ctx) { 
    printf("Selected %s\n", __FUNCTION__);

    char mode;
    input_mode_selector(mode);

    if (mode == 'T') { 
        // pass
        return;
    }

    if (mode == 'V') { 
        u_ctx = uart::new_uart_connection(RUNNING_PORT, BAUDRATE);

        recorder.start(); 
        while(uart::_flag_uart_running_port.load()) { 
            if (uart::ready_to_tx(u_ctx) && snd_mic_buffer.Size() > uart::block_size) { 
                for (int i = 0; i < uart::block_size; ++i) { 
                    short data; 
                    if (snd_mic_buffer.Pull(data)) {
                        u_ctx->tx_buff.Push(data); 
                    }
                }
            }

            if (uart::ready_to_rx(u_ctx)) { 
                for (int i = 0; i < uart::block_size; ++i) { 
                    short data;
                    if (u_ctx->rx_buff.Pull(data)) {
                        snd_din_buffer.Push(data); 
                    }
                }
            }
        }
        recorder.stop(); 
    }

    if (mode == 'E') {
        u_ctx = uart::new_uart_connection(RUNNING_PORT, BAUDRATE);

        recorder.start(); 
        while(uart::_flag_uart_running_port.load()) { 
            if (snd_mic_buffer.Size() > uart::block_size) { 
                for (int i = 0; i < uart::block_size; ++i) { 
                    short data; 
                    if (snd_mic_buffer.Pull(data)) {
                        snd_din_buffer.Push(data); 
                    }
                }
            }
        }
        recorder.stop(); 
    }
}

/**
 * Защита
 * 
 * 
 * 
 * тема: Защита свидетеля
 *  
 */