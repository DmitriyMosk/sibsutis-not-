module test (
  input clk,
  output led1, led2, led3, led4
);

reg [25:0] dig;

// Custom order of LEDs to distinguish from default program
assign {led1, led4, led3, led2} = dig[25:22];

always @(posedge clk) dig <= dig+1;

endmodule