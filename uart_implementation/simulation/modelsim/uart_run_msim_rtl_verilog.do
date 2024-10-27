transcript on
if {[file exists rtl_work]} {
	vdel -lib rtl_work -all
}
vlib rtl_work
vmap work rtl_work

vlog -vlog01compat -work work +incdir+F:/fpga/uart_implementation {F:/fpga/uart_implementation/pll.v}
vlog -vlog01compat -work work +incdir+F:/fpga/uart_implementation/db {F:/fpga/uart_implementation/db/pll_altpll.v}
vlog -sv -work work +incdir+F:/fpga/uart_implementation {F:/fpga/uart_implementation/uart.sv}
vlog -sv -work work +incdir+F:/fpga/uart_implementation {F:/fpga/uart_implementation/fifo.sv}
vlog -sv -work work +incdir+F:/fpga/uart_implementation {F:/fpga/uart_implementation/shift_reg.sv}
vlog -sv -work work +incdir+F:/fpga/uart_implementation {F:/fpga/uart_implementation/uart_rx.sv}
vlog -sv -work work +incdir+F:/fpga/uart_implementation {F:/fpga/uart_implementation/uart_tx.sv}

