#pragma once 
#include <thread>
#include <atomic> 
#include <chrono>

#include "AtomicQueue.hpp"
#include "xserial.hpp" 

#define UART_DEFAULT_MTU 1024

typedef short uart_dt_t; 
typedef std::chrono::time_point<std::chrono::high_resolution_clock> metrics_chrono_clock_t; 
typedef std::chrono::duration<long double, std::milli> metrics_chrono_clock_diff_t; 

namespace xserial::protocol { 
    typedef struct metrics {
        size_t dt_size; 
        size_t counter; 
        metrics_chrono_clock_t start;
        metrics_chrono_clock_t end;
        metrics_chrono_clock_diff_t avg_20;
    } metrics;

    template<typename T> 
    class uart { 
        public: 
            uint32_t print_metric_clock_delay;  

        public: 
            explicit uart() = default;
            explicit uart(const uint8_t portNumer, const size_t baudrate, const size_t mtuSize, const uint32_t pmcd=2000);
            ~uart();


            uart& operator=(const uart& other);

            bool open(); 
            bool is_open() const; 
            bool close(); 

            AtomicQueue<T>*         rx_buff;
            AtomicQueue<T>*         tx_buff;
        private: 
            xserial::ComPort        serial_;
            uint8_t                 serial_port_number_;
            size_t                  serial_baudrate_;

            std::thread             serial_thread_; 
            std::atomic<bool>       serial_is_open_{false};

            size_t                  serial_mtu_;

            void serial_thread_watchdog();
            bool tx_function_handler(T* mtu_tx);
            bool rx_function_handler(T* mtu_rx);
    }; 
}; 

//явно создаем экземпляр шаблона класса
template class xserial::protocol::uart<uart_dt_t>;



// #ifndef _LIB_UART_SHARED
// #define _LIB_UART_SHARED

// #include "AtomicQueue.hpp" 

// namespace uart { 
//     constexpr size_t block_size = 1024;    

//     // в ms 
//     constexpr int print_metric_clock_delay = 2000;  

//     // флаг управления потоком!!
//     extern std::atomic<bool> _flag_uart_running_port; 

//     typedef void  void_thread; 
//     typedef short uart_dt_t; 


//     typedef struct uart_ctx_t { 
//         std::thread thread; 
//         xserial::ComPort com; 

//         AtomicQueue<uart_dt_t> rx_buff; 
//         AtomicQueue<uart_dt_t> tx_buff; 

//         // ну типа metrics data
//         metrics *md;

//         std::atomic<bool> ready_to_tx; 
//         std::atomic<bool> ready_to_rx;

//         uart_ctx_t() 
//             : rx_buff(block_size+1), tx_buff(block_size+1), ready_to_tx(false), ready_to_rx(false) {} 
//     } uart_ctx_t; 

//     uart_ctx_t* new_uart_connection(uint8_t port_number, size_t baudrate);

//     volatile bool thread_join(uart_ctx_t* ctx);

//     bool ready_to_tx(uart_ctx_t* ctx); 
//     bool ready_to_rx(uart_ctx_t* ctx); 

//     void tx(uart_ctx_t* ctx, uart_dt_t* tmp_tx_buff, bool &op_rx_tx); 
//     void rx(uart_ctx_t* ctx, uart_dt_t* tmp_rx_buff, bool &op_rx_tx); 

//     void_thread controller(uart_ctx_t* ctx);
//     void        controller_end(uart_ctx_t* ctx);   
// }   

// #endif 