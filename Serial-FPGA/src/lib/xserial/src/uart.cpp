#include <iostream> 
#include <cstring>

#include "protocols/uart.hpp"
#include "xserial.hpp"

namespace xserial::protocol { 
    template<typename T> 
    uart<T>::uart(const uint8_t portNumer, const size_t baudrate, const size_t mtuSize, const uint32_t pmcd) { 
        serial_port_number_         = portNumer;
        serial_mtu_                 = mtuSize;
        serial_baudrate_            = baudrate; 

        print_metric_clock_delay    = pmcd;

        rx_buff = nullptr; 
        tx_buff = nullptr;
    }

    template<typename T> 
    uart<T>::~uart() { 
        close();

        if (rx_buff != nullptr) { 
            delete rx_buff;
        }
        if (tx_buff != nullptr) { 
            delete tx_buff; 
        }
    }

    template<typename T> 
    uart<T>& uart<T>::operator=(const uart& other) {
        if (this == &other) { 
            return *this;
        }

        if (this->is_open() || other.is_open()) { 
            throw std::invalid_argument("DO NOT COPY OPENED PORT");
        }

        serial_port_number_ = other.serial_port_number_; 
        serial_mtu_         = other.serial_mtu_; 
        serial_baudrate_    = other.serial_baudrate_; 
        
        print_metric_clock_delay = other.print_metric_clock_delay; 

        return *this;
    }

    template<typename T>
    bool uart<T>::open() { 
        if ( serial_.getStateComPort() || is_open() ) { 
            std::cerr << "[" << __FUNCTION__ << "] Thread error occurred (port already opened)\n";
            return false; 
        }

        serial_.open(serial_port_number_, serial_baudrate_); 

        if (!serial_.getStateComPort()) { 
            std::cerr << "[" << __FUNCTION__ << "] Thread error occurred (com is invalid)\n";
            return false; 
        }

        rx_buff = new AtomicQueue<T>(serial_mtu_); 
        tx_buff = new AtomicQueue<T>(serial_mtu_);

        serial_thread_ = std::thread(&uart<T>::serial_thread_watchdog, this);
        serial_is_open_.store(true, std::memory_order_release);

        return true; 
    }

    template<typename T> 
    bool uart<T>::is_open() const { 
        return serial_is_open_.load(std::memory_order_acquire);
    }

    template<typename T>
    bool uart<T>::close() { 
        serial_is_open_.store(false, std::memory_order_release);
        if (serial_thread_.joinable()) { 
            serial_thread_.join(); 
            return true; 
        } else { 
            return false; 
        }
    }

    template<typename T>
    void uart<T>::serial_thread_watchdog() { 
        T* mtu_tx = new T[serial_mtu_];
        T* mtu_rx = new T[serial_mtu_];

        metrics m; 
        auto last_print_time = std::chrono::high_resolution_clock::now();

        // размер блока данных
        m.dt_size = sizeof(T) * serial_mtu_; 

        while (is_open()) { 
            m.start = std::chrono::high_resolution_clock::now();
            if ( tx_function_handler(mtu_tx) ) 
                rx_function_handler(mtu_rx);
            m.end = std::chrono::high_resolution_clock::now();

            if (m.counter > 20) { 
                m.counter = 1; 
                m.avg_20 = std::chrono::duration_cast<metrics_chrono_clock_diff_t>(m.end - m.start);
            } else { 
                m.counter++;
                m.avg_20 += std::chrono::duration_cast<metrics_chrono_clock_diff_t>(m.end - m.start); 
            }

            std::chrono::duration<long double, std::milli> time_diff = m.end - last_print_time;

            // статистику выводим пока не получим 
            // на получение/отправку хотяб одно значение
            if ( time_diff.count() > print_metric_clock_delay && m.counter == 20 && 
                (rx_buff->Size() > 0 || tx_buff->Size() > 0) 
            ) { 
                std::cout 
                    << "Tx->Rx delay (20): " << m.avg_20.count() / m.counter 
                    << " ms " << (m.dt_size * 20 / 1024) / (m.avg_20.count() / m.counter) 
                    << " KB/ms" << std::endl;

                last_print_time = m.end;
            }
        }

        delete[] mtu_rx;
        delete[] mtu_tx;
 
        serial_is_open_.store(false, std::memory_order_release);
        serial_.close(); 
    }   

    template<typename T> 
    bool uart<T>::rx_function_handler(T* mtu_buff) { 
        std::memset(mtu_buff, 0, serial_mtu_ * sizeof(T));

        size_t readed = 0; 
        char* rx_bytes = reinterpret_cast<char*>(mtu_buff);
        
        // Если винда возвращает, что элементов в очереди 0, то serial_.read скипается. Такая вот полумера от 
        // того, что мы бесконечно пишем в буфер, не получая вывода
        while (readed != serial_mtu_ * sizeof(T)) {
            readed += serial_.read(rx_bytes + readed, serial_mtu_ * sizeof(T) - readed);
        }

        for (size_t i = 0; i < serial_mtu_; ++i) { 
            rx_buff->Push(mtu_buff[i]);
        }
        return false;
    }

    template<typename T> 
    bool uart<T>::tx_function_handler(T* mtu_buff) {
        if (tx_buff->Size() >= serial_mtu_ - 1) { 
            for (size_t i = 0; i < serial_mtu_; ++i) { 
                T dat; 
                if (tx_buff->Pull(dat)) { 
                    mtu_buff[i] = dat; 
                }
            }

            if (!serial_.write(reinterpret_cast<char*>(mtu_buff), serial_mtu_ * sizeof(T))) {
                std::cerr << "[" << __func__ << "] Thread error occurred (com.write failed)\n";
                serial_is_open_.store(false, std::memory_order_release);
            }

            std::memset(mtu_buff, 0, serial_mtu_ * sizeof(T)); 

            return true;
        }
        return false;
    }   
}; 

// #include <iostream>
// #include <cstring> 

// void uart::tx(uart_ctx_t* ctx, uart_dt_t* tmp_tx_buff, bool &op_rx_tx) {
//     if (ctx->rx_buff.Size() > 0) { 
//         return; 
//     } else { 
//         ctx->ready_to_tx.store(true); 
//     }

//     if (op_rx_tx)
//         return; // если op_rx_tx - true => мы не запускаем ф-ю дальше

//     // Если буфер ещё не полон...
//     if (ctx->tx_buff.Size() != block_size) { 
//         return; 
//     } else { 
//         // Если буфер полон, то заставляем другой поток прекратить писать в него данные
//         ctx->ready_to_tx.store(false); 
//     }

//     // Предварительно ставим операцию на чтение. данная функция ещё блокирует цикл, так что не страшно
//     op_rx_tx = true;

//     std::memset(tmp_tx_buff, 0, block_size * sizeof(uart_dt_t));

//     for (int i = 0; i < block_size; ++i) { 
//         ctx->tx_buff.Pull(tmp_tx_buff[i]);
//     }

//     char* tx_bytes = reinterpret_cast<char*>(tmp_tx_buff);

//     if (!ctx->com.write(tx_bytes, block_size * sizeof(uart_dt_t))) {
//         std::cerr << "[" << __func__ << "] Thread error occurred (com.write failed)\n";
//         _flag_uart_running_port.store(false); 
   
//         ctx->ready_to_rx.store(false); 
//         ctx->ready_to_tx.store(false); 
//         return; 
//     }
// }   

// void uart::rx(uart_ctx_t* ctx, uart_dt_t* tmp_rx_buff, bool &op_rx_tx) {
//     if (!op_rx_tx)
//         return; // если op_rx_tx - false => мы не запускаем ф-ю дальше

//     std::memset(tmp_rx_buff, 0, block_size * sizeof(uart_dt_t));

//     size_t readed = 0; 
//     char* rx_bytes = reinterpret_cast<char*>(tmp_rx_buff);

//     while (readed != block_size * sizeof(uart_dt_t)) {
//         readed += ctx->com.read(rx_bytes + readed, block_size * sizeof(uart_dt_t) - readed);
//     }

//     for (int i = 0; i < block_size; ++i) { 
//         if (!ctx->rx_buff.Push(tmp_rx_buff[i])) {
//             std::cerr << "[" << __func__ << "] Thread error occurred (ctx->rx_buff.Push out of range) " << readed << "\n";
//             _flag_uart_running_port.store(false); 
            
//             ctx->ready_to_tx.store(false); 
//             ctx->ready_to_rx.store(false); 
//             break;
//         }
//     }

//     op_rx_tx = false; 

//     ctx->ready_to_rx.store(true); 
// }

// uart::void_thread uart::controller(uart_ctx_t* ctx) {
//     if (!ctx->com.getStateComPort()) { 
//         std::cerr << "[" << __FUNCTION__ << "] Thread error occurred (com is invalid)\n";
//         return; 
//     }

//     std::cout << "[" << __FUNCTION__ << "] in running state.\n";

//     ctx->md->dt_size = sizeof(uart_dt_t) * block_size;

//     ctx->ready_to_tx.store(true);
//     ctx->ready_to_rx.store(false); 

//     uart_dt_t tmp_rx_buff[block_size]; 
//     uart_dt_t tmp_tx_buff[block_size]; 

//     // false - tx
//     // true  - rx
//     // костыль для синхры двух функций...
//     bool op_rx_tx = false;  

//     auto last_print_time = std::chrono::high_resolution_clock::now();

//     while (_flag_uart_running_port.load() && ctx->com.getStateComPort()) { 
//         ctx->md->start = std::chrono::high_resolution_clock::now();
//         tx(ctx, tmp_tx_buff, op_rx_tx);
//         rx(ctx, tmp_rx_buff, op_rx_tx); 
//         ctx->md->end = std::chrono::high_resolution_clock::now();

//         if (ctx->md->counter > 20) { 
//             ctx->md->counter = 1; 
//             ctx->md->avg_20 = std::chrono::duration_cast<metrics_chrono_clock_diff_t>(ctx->md->end - ctx->md->start);
//         } else { 
//             ctx->md->counter++;
//             ctx->md->avg_20 += std::chrono::duration_cast<metrics_chrono_clock_diff_t>(ctx->md->end - ctx->md->start); 
//         }

//         std::chrono::duration<long double, std::milli> time_diff = ctx->md->end - last_print_time;

//         if (time_diff.count() > uart::print_metric_clock_delay && ctx->md->counter == 20) { 
//             std::cout 
//                 << "Tx->Rx delay (20): " << ctx->md->avg_20.count() / ctx->md->counter 
//                 << " ms " << (ctx->md->dt_size * 20 / 1024) / (ctx->md->avg_20.count() / ctx->md->counter) 
//                 << " KB/ms" << std::endl;

//             last_print_time = ctx->md->end;
//         }
//     }
//     _flag_uart_running_port.store(false); 

//     uart::controller_end(ctx);

//     std::cout << "[" << __FUNCTION__ << "] Thread successfully completed\n"; 
// }

// void uart::controller_end(uart_ctx_t* ctx) { 
//     ctx->ready_to_tx.store(false);
//     ctx->ready_to_rx.store(false); 

//     ctx->com.close(); 
//     delete ctx->md;
//     delete ctx; 
// }