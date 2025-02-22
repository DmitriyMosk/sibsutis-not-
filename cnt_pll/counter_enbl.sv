module counter_enbl # (parameter N=5)               // объявляем значение параметра равным 5
    (
        input clk,                                  // входы по умолчанию имеют тип wire
        input reset,
        input enable,
        output reg [N-1:0] counter                  // объявляем тип выхода  шина reg
	);
    always @(posedge clk) begin                     // по положительному фронту генератора clk
        if (reset) begin                            // если reset=1
            counter <= 0;                           // сброс счетчика в 0
        end 
        else if (enable) begin                      // в противном случае если enable=1
            counter <= counter + 1;                 // увеличиваем значение счетчика на 1
        end
    end
endmodule