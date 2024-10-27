module fifo (
    input wire clk,
    input wire rst,
    input wire [7:0] data_in,
    input wire wr_en,
    input wire rd_en,
    output reg [7:0] data_out,
    output reg full,
    output reg empty
);
    reg [7:0] mem [0:15]; // 16-байтный FIFO
    reg [3:0] wr_ptr = 0;
    reg [3:0] rd_ptr = 0;
    reg [4:0] count = 0;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            wr_ptr <= 0;
            rd_ptr <= 0;
            count <= 0;
            full <= 0;
            empty <= 1;
            data_out <= 8'b0; // сбрасываем data_out на ноль при сбросе
        end else begin
            if (wr_en && !full) begin
                mem[wr_ptr] <= data_in;
                wr_ptr <= (wr_ptr + 1) % 16; // используем модуль 16 для циклического указателя
                count <= count + 1;
            end
            if (rd_en && !empty) begin
                data_out <= mem[rd_ptr];
                rd_ptr <= (rd_ptr + 1) % 16; // используем модуль 16 для циклического указателя
                count <= count - 1;
            end
            full <= (count == 16); // FIFO полон, если счетчик равен 16
            empty <= (count == 0);  // FIFO пуст, если счетчик равен 0
        end
    end
endmodule
