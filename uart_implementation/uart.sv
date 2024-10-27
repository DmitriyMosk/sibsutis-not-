module uart (
    input wire clk, // Входной тактовый сигнал
    input wire rst, // Сброс
    input wire rx, // Вход UART
    output wire tx, // Выход UART
    output wire led // Индикация светодиода
);
    wire [7:0] data_out;        // Данные из UART
    wire valid;                 // Указатель на валидность данных
    wire [7:0] shift_reg_out;   // Выход сменного регистра
    wire [7:0] fifo_out;        // Выход FIFO
    wire fifo_full;             // Индикатор переполнения FIFO
    wire fifo_empty;            // Индикатор пустоты FIFO
    wire pll_clk;               // Выход PLL

    // Инстанцирование PLL
    pll pll_inst (
        .areset(rst),         
        .inclk0(clk),         
        .c0(pll_clk),        
        .locked()            
    );

    // Инстанцирование модуля UART приёма
    uart_rx rx_inst (
        .clk(pll_clk),
        .rst(rst),
        .rx(rx),
        .data_out(data_out),
        .valid(valid),
        .led(led)
    );

    // Инстанцирование сменного регистра
    shift_reg shift_reg_inst (
        .clk(pll_clk),
        .rst(rst),
        .data_in(data_out),
        .load(valid),
        .data_out(shift_reg_out) // Используем выход shift_reg
    );

    // Инстанцирование FIFO
    fifo fifo_inst (
        .clk(pll_clk),
        .rst(rst),
        .data_in(shift_reg_out), // Используем выход сменного регистра
        .wr_en(valid && !fifo_full),
        .rd_en(!fifo_empty),
        .data_out(fifo_out),     // Выход FIFO
        .full(fifo_full),
        .empty(fifo_empty)
    );

    // Инстанцирование модуля UART передачи
    uart_tx tx_inst (
        .clk(pll_clk),
        .rst(rst),
        .start(valid),
        .data_in(fifo_out),      // Используем fifo_out для передачи
        .tx(tx)
    );

endmodule
