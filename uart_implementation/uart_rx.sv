module uart_rx (
    input wire clk, // Тактовый сигнал
    input wire rst, // Сброс
    input wire rx, // Вход UART
    output reg [7:0] data_out, // Принятые данные
    output reg valid, // Флаг наличия данных
    output reg led // Индикация светодиода
);
    parameter BAUD_RATE = 9600;
    parameter CLK_FREQ = 50000000; // 50 МГц
    localparam integer BAUD_TICKS = CLK_FREQ / BAUD_RATE;

    reg [15:0] tick_count = 0;
    reg [3:0] bit_count = 0;
    reg [7:0] shift_reg = 8'd0;
    reg receiving = 1'b0;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            tick_count <= 0;
            bit_count <= 0;
            shift_reg <= 8'd0;
            valid <= 0;
            led <= 0;
            receiving <= 0;
        end else begin
            if (!receiving && !rx) begin // Старт-бит
                receiving <= 1;
                tick_count <= 0;
                bit_count <= 0;
            end else if (receiving) begin
                tick_count <= tick_count + 1;
                if (tick_count == BAUD_TICKS) begin
                    tick_count <= 0;
                    shift_reg <= {rx, shift_reg[7:1]}; // Сдвиг битов вправо
                    bit_count <= bit_count + 1;
                    if (bit_count == 8) begin // Все 8 бит данных получены
                        data_out <= shift_reg;
                        valid <= 1;
                        led <= 1; // Включение светодиода
                        receiving <= 0;
                    end else begin
                        valid <= 0;
                        led <= 0;
                    end
                end
            end
        end
    end
endmodule