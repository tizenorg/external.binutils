#objdump: -dr --prefix-addresses
#as: -n32 --defsym tsd=1
#name: MIPS sd n32
#source: ld.s

# Test the sd macro, n32.

.*: +file format .*mips.*

Disassembly of section \.text:
[0-9a-f]+ <[^>]*> sd	a0,0\(zero\)
[0-9a-f]+ <[^>]*> sd	a0,1\(zero\)
[0-9a-f]+ <[^>]*> lui	at,0x1
[0-9a-f]+ <[^>]*> sd	a0,-32768\(at\)
[0-9a-f]+ <[^>]*> sd	a0,-32768\(zero\)
[0-9a-f]+ <[^>]*> lui	at,0x1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[0-9a-f]+ <[^>]*> lui	at,0x2
[0-9a-f]+ <[^>]*> sd	a0,-23131\(at\)
[0-9a-f]+ <[^>]*> sd	a0,0\(a1\)
[0-9a-f]+ <[^>]*> sd	a0,1\(a1\)
[0-9a-f]+ <[^>]*> lui	at,0x1
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,-32768\(at\)
[0-9a-f]+ <[^>]*> sd	a0,-32768\(a1\)
[0-9a-f]+ <[^>]*> lui	at,0x1
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[0-9a-f]+ <[^>]*> lui	at,0x2
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,-23131\(at\)
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label
[0-9a-f]+ <[^>]*> sd	a0,0\(gp\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	small_external_data_label
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common
[0-9a-f]+ <[^>]*> sd	a0,0\(gp\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	small_external_common
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss
[0-9a-f]+ <[^>]*> sd	a0,0\(gp\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	\.sbss
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data\+0x1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data\+0x1
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label\+0x1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label\+0x1
[0-9a-f]+ <[^>]*> sd	a0,0\(gp\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	small_external_data_label\+0x1
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common\+0x1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common\+0x1
[0-9a-f]+ <[^>]*> sd	a0,0\(gp\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	small_external_common\+0x1
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss\+0x1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss\+0x1
[0-9a-f]+ <[^>]*> sd	a0,0\(gp\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	\.sbss\+0x1
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data\+0x8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label\+0x8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_data_label\+0x8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_data_label\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common\+0x8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_common\+0x8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_common\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss\+0x8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.sbss\+0x8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.sbss\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data\+0xffff8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label\+0xffff8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_data_label\+0xffff8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_data_label\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common\+0xffff8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_common\+0xffff8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_common\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss\+0xffff8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.sbss\+0xffff8000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.sbss\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data\+0x10000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label\+0x10000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_data_label\+0x10000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_data_label\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common\+0x10000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_common\+0x10000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_common\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss\+0x10000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.sbss\+0x10000
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.sbss\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data\+0x1a5a5
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label\+0x1a5a5
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_data_label\+0x1a5a5
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_data_label\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common\+0x1a5a5
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_common\+0x1a5a5
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_common\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss\+0x1a5a5
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.sbss\+0x1a5a5
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.sbss\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label
[0-9a-f]+ <[^>]*> addu	at,a1,gp
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	small_external_data_label
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common
[0-9a-f]+ <[^>]*> addu	at,a1,gp
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	small_external_common
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss
[0-9a-f]+ <[^>]*> addu	at,a1,gp
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	\.sbss
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data\+0x1
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data\+0x1
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label\+0x1
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label\+0x1
[0-9a-f]+ <[^>]*> addu	at,a1,gp
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	small_external_data_label\+0x1
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common\+0x1
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common\+0x1
[0-9a-f]+ <[^>]*> addu	at,a1,gp
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	small_external_common\+0x1
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss\+0x1
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss\+0x1
[0-9a-f]+ <[^>]*> addu	at,a1,gp
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_GPREL16	\.sbss\+0x1
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data\+0x8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label\+0x8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_data_label\+0x8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_data_label\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common\+0x8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_common\+0x8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_common\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss\+0x8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.sbss\+0x8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.sbss\+0x8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data\+0xffff8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label\+0xffff8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_data_label\+0xffff8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_data_label\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common\+0xffff8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_common\+0xffff8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_common\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss\+0xffff8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.sbss\+0xffff8000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.sbss\+0xffff8000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data\+0x10000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label\+0x10000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_data_label\+0x10000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_data_label\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common\+0x10000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_common\+0x10000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_common\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss\+0x10000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.sbss\+0x10000
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.sbss\+0x10000
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.data\+0x1a5a5
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.data\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_data_label\+0x1a5a5
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_data_label\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_data_label\+0x1a5a5
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_data_label\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	big_external_common\+0x1a5a5
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	big_external_common\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	small_external_common\+0x1a5a5
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	small_external_common\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.bss\+0x1a5a5
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.bss\+0x1a5a5
[0-9a-f]+ <[^>]*> lui	at,0x0
[ 	]*[0-9a-f]+: R_MIPS_HI16	\.sbss\+0x1a5a5
[0-9a-f]+ <[^>]*> addu	at,at,a1
[0-9a-f]+ <[^>]*> sd	a0,0\(at\)
[ 	]*[0-9a-f]+: R_MIPS_LO16	\.sbss\+0x1a5a5
	\.\.\.
