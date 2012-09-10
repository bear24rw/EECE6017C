transcript on
if {[file exists rtl_work]} {
	vdel -lib rtl_work -all
}
vlib rtl_work
vmap work rtl_work

vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/lab_3/part_2 {/home/user/school/emb_sys/lab_3/part_2/d_latch.v}

vlog -vlog01compat -work work +incdir+/home/user/school/emb_sys/lab_3/part_2 {/home/user/school/emb_sys/lab_3/part_2/d_latch_tb.v}

vsim -t 1ps -L altera_ver -L lpm_ver -L sgate_ver -L altera_mf_ver -L altera_lnsim_ver -L cycloneii_ver -L rtl_work -L work -voptargs="+acc"  d_latch_tb

add wave *
view structure
view signals
run -all
