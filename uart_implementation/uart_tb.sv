module uart_tb;
    reg clk;
    reg rst;
    reg start;
    reg [7:0] data_in;
    wire tx;
    wire [7:0] data_out;
    wire valid;
    wire led;
    wire [7:0] fifo_out;
    wire fifo_full;
    wire fifo_empty;
    wire pll_clk;

    // Инстанцирование модулей
    uart_rx rx_inst (
        .clk(clk),
        .rst(rst),
        .rx(tx),
        .data_out(data_out),
        .valid(valid),
        .led(led)
    );

    uart_tx tx_inst (
        .clk(clk),
        .rst(rst),
        .start(start),
        .data_in(data_in),
        .tx(tx)
    );

    shift_reg shift_reg_inst (
        .clk(clk),
        .rst(rst),
        .data_in(data_in),
        .load(start),
        .data_out(fifo_out)
    );

    pll pll_inst (
        .clk_in(clk),
        .clk_out(pll_clk)
    );

    fifo fifo_inst (
        .clk(clk),
        .rst(rst),
        .data_in(data_out),
        .wr_en(valid),
        .rd_en(!fifo_empty),
        .data_out(fifo_out),
        .full(fifo_full),
        .empty(fifo_empty)
    );

    // Генерация тактового сигнала
    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end

    // Начальные условия
    initial begin
        rst = 1;
        start = 0;
        data_in = 8'h55;
        #10 rst = 0;
        #20 start = 1;
        #10 start = 0;
    end

    // Мониторинг
    initial begin
        $monitor("Time: %0t | Data Out: %h | Valid: %b | LED: %b | FIFO Out: %h | FIFO Full: %b | FIFO Empty: %b", 
                 $time, data_out, valid, led, fifo_out, fifo_full, fifo_empty);
    end
endmodule
