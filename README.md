A real-time Mandelbrot zoom on a 1.4$ Blue Pill (STM32F103C8)
=============================================================

Yet another useless but fun hack:  a Mandelbrot engine on a 1.4$ Blue Pill :-)

[![A real-time Mandelbrot on a 1.4$ Blue Pill (STM32F103C8)](contrib/Blue_Pill_Mandelbrot.jpg "A real-time Mandelbrot on a 1.4$ Blue Pill (STM32F103C8)")](https://youtu.be/5875JOnFDLg)

***(Click on picture to play the video)***

Things worthy of note:

- The screen is a 16K IPS color one, driven by an ST7735 controller.

- The calculation obviously uses fixed-point. The STM32F103C8 is a
  32-bit machine, doing single-cycle 32-bit integer multiplication. It can
  therefore be used to perform nice 6.26 fixed point, which allows us to zoom
  in real-time in the Mandelbrot set. The [FRACT_BITS](https://github.com/ttsiodras/Blue_Pill_Mandelbrot/blob/master/BluePill_TFT16K_Mandelbrot.ino#L111) macro defines the number of fractional bits used (out of a total of 32).

- The STM32F103C8 has only 20K of SRAM. This means that a full screen page,
  containing 160x80x2 = 25600 bytes, doesn't fit in memory; and therefore we
  cannot perform fast memory-to-screen SPI transfers for a full screen.

- There is a workaround (template parameter [entireScreen](https://github.com/ttsiodras/Blue_Pill_Mandelbrot/blob/master/BluePill_TFT16K_Mandelbrot.ino#L177)), where the two screen parts are computed in sequence, and 
  are "blit"-ed in turn to the screen. The visual effect of that is
  a bit disturbing, though - so by default a smaller 80x80 resolution
  is used, that does fit in memory. The smaller size helps with the frame
  rate, too ;-)

- The number of iterations of the Mandelbrot computation, after which
  the pixel is considered as belonging in the "lake", is set in the macro
  [ITERATIONS](https://github.com/ttsiodras/Blue_Pill_Mandelbrot/blob/master/BluePill_TFT16K_Mandelbrot.ino#L26).

- `Adafruit_GFX`'s `drawRGBBitmap` is used - which blasts the buffer's
  contents over high-speed SPI. This dramatically improved both speed
  and perceptual quality (no "tearing").

- The `entireScreen` template parameter is evaluated at compile-time;
  the `mandel` function basically exists conceptually in two forms:
  one computing 2 80x80 windows and SPI-ing them in turn to render
  on the entire screen, and one with a single 80x80. To decide which
  form to build, the `Makefile` has two targets: `make small_screen`
  (default) and `make big_screen`.

- In the `small_screen` mode, there was enough room to add a frame
  rate counter. In the video you can see that we start at around 10
  frames per second (FPS) and after zooming 1600 times in the middle
  of a Mandelbrot lake, we end up with 1FPS.

- The Blue Pill is programmed via OpenOCD - which is also used to flash it;
  the `make openocd` sets things up, after which (a) `make upload`
  will just load and run, where as (b) `make flash` will permanently
  write into flash memory. After that, powering up the microcontroller
  with 3.3V *(as shown in the video)* suffices.

Enjoy - and keep tinkering!

P.S. If you want to, you can also checkout my [FPGA/VHDL](https://github.com/ttsiodras/MandelbrotInVHDL)
version of Mandelbrot - as well as my [SSE / OpenMP / CUDA / XaoS](https://github.com/ttsiodras/MandelbrotSSE) one.
