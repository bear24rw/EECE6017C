transcript on
if {[file exists rtl_work]} {
	vdel -lib rtl_work -all
}
vlib rtl_work
vmap work rtl_work

vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/project_1/verilog {/home/user/school/emb_sys/project_1/verilog/bcd_sub.v}
vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/project_1/verilog {/home/user/school/emb_sys/project_1/verilog/pulse_led.v}
vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/project_1/verilog {/home/user/school/emb_sys/project_1/verilog/clk_div.v}
vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/project_1/verilog {/home/user/school/emb_sys/project_1/verilog/temp_input.v}
vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/project_1/verilog {/home/user/school/emb_sys/project_1/verilog/state_2_bcd.v}
vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/project_1/verilog {/home/user/school/emb_sys/project_1/verilog/top.v}
vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/project_1/verilog {/home/user/school/emb_sys/project_1/verilog/seven_seg.v}
vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/project_1/verilog {/home/user/school/emb_sys/project_1/verilog/monitor.v}

vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/project_1/verilog/tb {/home/user/school/emb_sys/project_1/verilog/tb/monitor_tb.v}

vsim -t 1ps -L altera_ver -L lpm_ver -L sgate_ver -L altera_mf_ver -L altera_lnsim_ver -L cycloneii_ver -L rtl_work -L work -voptargs="+acc"  monitor_tb

add wave *
view structure
view signals
run -all
