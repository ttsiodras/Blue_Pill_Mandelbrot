set pagination off
set prompt off
tar remote :3333
mon reset halt
file tmp/BluePill_TFT16K_Mandelbrot.ino.elf
load
compare-sections
quit
