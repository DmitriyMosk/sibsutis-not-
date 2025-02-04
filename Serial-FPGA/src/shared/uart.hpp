#ifndef _LIB_UART_SHARED
#define _LIB_UART_SHARED

#include <atomic> 
#include <chrono>
#include <thread>
#include "xserial.hpp" 
#include "AtomicQueue.hpp" 

namespace uart { 
    constexpr size_t block_size = 1024;    

    // в ms 
    constexpr int print_metric_clock_delay = 2000;  

    // флаг управления потоком!!
    extern std::atomic<bool> _flag_uart_running_port; 

    typedef void  void_thread; 
    typedef short uart_dt_t; 
    typedef std::chrono::time_point<std::chrono::high_resolution_clock> metrics_chrono_clock_t; 
    typedef std::chrono::duration<long double, std::milli> metrics_chrono_clock_diff_t; 

    typedef struct metrics {
        size_t dt_size; 
        size_t counter; 
        metrics_chrono_clock_t start;
        metrics_chrono_clock_t end;
        metrics_chrono_clock_diff_t avg_20;
    } metrics;

    typedef struct uart_ctx_t { 
        std::thread thread; 
        xserial::ComPort com; 

        AtomicQueue<uart_dt_t> rx_buff; 
        AtomicQueue<uart_dt_t> tx_buff; 

        // ну типа metrics data
        metrics *md;

        std::atomic<bool> ready_to_tx; 
        std::atomic<bool> ready_to_rx;

        uart_ctx_t() 
            : rx_buff(block_size+1), tx_buff(block_size+1), ready_to_tx(false), ready_to_rx(false) {} 
    } uart_ctx_t; 

    uart_ctx_t* new_uart_connection(uint8_t port_number, size_t baudrate);

    volatile bool thread_join(uart_ctx_t* ctx);

    bool ready_to_tx(uart_ctx_t* ctx); 
    bool ready_to_rx(uart_ctx_t* ctx); 

    void tx(uart_ctx_t* ctx, uart_dt_t* tmp_tx_buff, bool &op_rx_tx); 
    void rx(uart_ctx_t* ctx, uart_dt_t* tmp_rx_buff, bool &op_rx_tx); 

    void_thread controller(uart_ctx_t* ctx);
    void        controller_end(uart_ctx_t* ctx);   
}   

#endif 