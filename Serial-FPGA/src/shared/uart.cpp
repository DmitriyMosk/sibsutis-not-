#include "uart.hpp" 
#include <iostream>
#include <csignal>
#include <cstring> 

std::atomic<bool> uart::_flag_uart_running_port(true);

static void uart_interrupt(int signal) { 
    if (signal == SIGINT) { 
        uart::_flag_uart_running_port.store(false);
    }
}

uart::uart_ctx_t* uart::new_uart_connection(uint8_t port_number, size_t baudrate) { 
    uart_ctx_t* ctx = new uart_ctx_t; 
    ctx->md = new metrics;
    ctx->md->counter = 0;
    ctx->md->avg_20 = metrics_chrono_clock_diff_t(0); 
    ctx->com.open(port_number, baudrate);

    ctx->thread = std::thread(uart::controller, ctx); 

    signal(SIGINT, uart_interrupt); 
    return ctx;
}
 
volatile bool uart::thread_join(uart_ctx_t* ctx) { 
    if (ctx->thread.joinable()) { 
        ctx->thread.join(); 
        return true; 
    } else { 
        return false; 
    }
}

bool uart::ready_to_tx(uart_ctx_t* ctx) { 
    return ctx->ready_to_tx.load() && uart::_flag_uart_running_port.load(); 
}

bool uart::ready_to_rx(uart_ctx_t* ctx) { 
    return ctx->ready_to_rx.load() && uart::_flag_uart_running_port.load(); 
}

void uart::tx(uart_ctx_t* ctx, uart_dt_t* tmp_tx_buff, bool &op_rx_tx) {
    if (ctx->rx_buff.Size() > 0) { 
        return; 
    } else { 
        ctx->ready_to_tx.store(true); 
    }

    if (op_rx_tx)
        return; // если op_rx_tx - true => мы не запускаем ф-ю дальше

    // Если буфер ещё не полон...
    if (ctx->tx_buff.Size() != block_size) { 
        return; 
    } else { 
        // Если буфер полон, то заставляем другой поток прекратить писать в него данные
        ctx->ready_to_tx.store(false); 
    }

    // Предварительно ставим операцию на чтение. данная функция ещё блокирует цикл, так что не страшно
    op_rx_tx = true;

    std::memset(tmp_tx_buff, 0, block_size * sizeof(uart_dt_t));

    for (int i = 0; i < block_size; ++i) { 
        ctx->tx_buff.Pull(tmp_tx_buff[i]);
    }

    char* tx_bytes = reinterpret_cast<char*>(tmp_tx_buff);

    if (!ctx->com.write(tx_bytes, block_size * sizeof(uart_dt_t))) {
        std::cerr << "[" << __func__ << "] Thread error occurred (com.write failed)\n";
        _flag_uart_running_port.store(false); 
   
        ctx->ready_to_rx.store(false); 
        ctx->ready_to_tx.store(false); 
        return; 
    }
}   

void uart::rx(uart_ctx_t* ctx, uart_dt_t* tmp_rx_buff, bool &op_rx_tx) {
    if (!op_rx_tx)
        return; // если op_rx_tx - false => мы не запускаем ф-ю дальше

    std::memset(tmp_rx_buff, 0, block_size * sizeof(uart_dt_t));

    size_t readed = 0; 
    char* rx_bytes = reinterpret_cast<char*>(tmp_rx_buff);

    while (readed != block_size * sizeof(uart_dt_t)) {
        readed += ctx->com.read(rx_bytes + readed, block_size * sizeof(uart_dt_t) - readed);
    }

    for (int i = 0; i < block_size; ++i) { 
        if (!ctx->rx_buff.Push(tmp_rx_buff[i])) {
            std::cerr << "[" << __func__ << "] Thread error occurred (ctx->rx_buff.Push out of range) " << readed << "\n";
            _flag_uart_running_port.store(false); 
            
            ctx->ready_to_tx.store(false); 
            ctx->ready_to_rx.store(false); 
            break;
        }
    }

    op_rx_tx = false; 

    ctx->ready_to_rx.store(true); 
}

uart::void_thread uart::controller(uart_ctx_t* ctx) {
    if (!ctx->com.getStateComPort()) { 
        std::cerr << "[" << __FUNCTION__ << "] Thread error occurred (com is invalid)\n";
        return; 
    }

    std::cout << "[" << __FUNCTION__ << "] in running state.\n";

    ctx->md->dt_size = sizeof(uart_dt_t) * block_size;

    ctx->ready_to_tx.store(true);
    ctx->ready_to_rx.store(false); 

    uart_dt_t tmp_rx_buff[block_size]; 
    uart_dt_t tmp_tx_buff[block_size]; 

    // false - tx
    // true  - rx
    // костыль для синхры двух функций...
    bool op_rx_tx = false;  

    auto last_print_time = std::chrono::high_resolution_clock::now();

    while (_flag_uart_running_port.load() && ctx->com.getStateComPort()) { 
        ctx->md->start = std::chrono::high_resolution_clock::now();
        tx(ctx, tmp_tx_buff, op_rx_tx);
        rx(ctx, tmp_rx_buff, op_rx_tx); 
        ctx->md->end = std::chrono::high_resolution_clock::now();

        if (ctx->md->counter > 20) { 
            ctx->md->counter = 1; 
            ctx->md->avg_20 = std::chrono::duration_cast<metrics_chrono_clock_diff_t>(ctx->md->end - ctx->md->start);
        } else { 
            ctx->md->counter++;
            ctx->md->avg_20 += std::chrono::duration_cast<metrics_chrono_clock_diff_t>(ctx->md->end - ctx->md->start); 
        }

        std::chrono::duration<long double, std::milli> time_diff = ctx->md->end - last_print_time;

        if (time_diff.count() > uart::print_metric_clock_delay && ctx->md->counter == 20) { 
            std::cout 
                << "Tx->Rx delay (20): " << ctx->md->avg_20.count() / ctx->md->counter 
                << " ms " << (ctx->md->dt_size * 20 / 1024) / (ctx->md->avg_20.count() / ctx->md->counter) 
                << " KB/ms" << std::endl;

            last_print_time = ctx->md->end;
        }
    }
    _flag_uart_running_port.store(false); 

    uart::controller_end(ctx);

    std::cout << "[" << __FUNCTION__ << "] Thread successfully completed\n"; 
}

void uart::controller_end(uart_ctx_t* ctx) { 
    ctx->ready_to_tx.store(false);
    ctx->ready_to_rx.store(false); 

    ctx->com.close(); 
    delete ctx->md;
    delete ctx; 
}