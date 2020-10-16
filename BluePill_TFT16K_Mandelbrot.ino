#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS     PA2
#define TFT_RST    PA4
#define TFT_DC     PA3

// The STM32F103C8 has only 20K of SRAM. This means that a full screen, which
// contains 160x80x2 = 25600 bytes, doesn't fit in memory; and therefore we
// cannot perform fast memory-to-screen SPI transfers for a full screen.
//
// There is a workaround (template parameter "entireScreen"), where we compute
// the two screen parts in sequence, and "blit" them one on each turn. The
// visual effect of that is a bit disturbing, though - so we instead default to
// a smaller 80x80 resolution (that does fit in memory).
//
// Helps with the frame rate, too ;-)

const int MAXX=80;
const int MAXY=80;
uint16_t screen[MAXX*MAXY];

// The number of iterations of the Mandelbrot computation, after which
// we consider the pixel as belonging in the "lake".
#define ITERATIONS 240

uint16_t lookup[ITERATIONS+1];

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

void palette()
{
    // Prepare the color palette used to smoothly move across
    // the color domains. I believe I got this code from
    // somewhere, during the DOS days. Ahh, memories - sigh.
    int i, ofs = 0;
    uint16_t r,g,b;

    static_assert( ITERATIONS < 256, "Up to 255 iterations supported" );

    // The screen I got from Banggood seems to be inverted;
    // FFFF is black, 0000 is white. Revert the intensity
    // values (31-x for red and blue, 63-x for green),
    // then shift them into position 5R-6G-5B.
#define MAP() { \
        r >>= 3; r = 31-r; r <<= 3; \
        g >>= 2; g = 63-g; g <<= 2; \
        b >>= 3; b = 31-b; b <<= 3; \
        lookup[i+ofs] = (r << 8) + (g << 3) + (b >> 3); \
    }
    for (i = 0; i < 16; i++) {
        r = 16*(16-abs(i-16));
        g = 0;
        b = 16*abs(i-16);
        MAP();
    }
    ofs = 16;
    for (i = 0; i < 16; i++) {
        r = 0;
        g = 16*(16-abs(i-16));
        b = 0;
        MAP();
    }
    ofs = 32;
    for (i = 0; i < 16; i++) {
        r = 0;
        g = 0;
        b = 16*(16-abs(i-16));
        MAP();
    }
    ofs = 48;
    for (i = 0; i < 16; i++) {
        r = 16*(16-abs(i-16));
        g = 16*(16-abs(i-16));
        b = 0;
        MAP();
    }
    ofs = 64;
    for (i = 0; i < 16; i++) {
        r = 0;
        g = 16*(16-abs(i-16));
        b = 16*(16-abs(i-16));
        MAP();
    }
    ofs = 80;
    for (i = 0; i < 16; i++) {
        r = 16*(16-abs(i-16));
        g = 0;
        b = 16*(16-abs(i-16));
        MAP();
    }
    ofs = 96;
    for (i = 0; i < 16; i++) {
        r = 16*(16-abs(i-16));
        g = 16*(16-abs(i-16));
        b = 16*(16-abs(i-16));
        MAP();
    }
    ofs = 112;
    for (i = 0; i < 16; i++) {
        r = 16*(8-abs(i-8));
        g = 16*(8-abs(i-8));
        b = 16*(8-abs(i-8));
        MAP();
    }
    // In theory, there's a gap in the palette from color 128
    // up to ITERATIONS; but our zooming here is so tiny that
    // this doesn't impact us.

    // But do set the palette for the bailout color (ITERATIONS)
    lookup[ITERATIONS] = 0xFFFF; // ...the Mandelbrot lake is black.
}

// Macros configuring fixed-point operations
#define FRACT_BITS 26
#define SCALE (1<<FRACT_BITS)
#define FLOAT2FIXED(x) ((x) * SCALE)
#define MULFIX(a,b)  ((((uint64_t)(a))*(b)) >> FRACT_BITS)

// Precompute outside loops
const int32_t FOUR = FLOAT2FIXED(4.0);

uint8_t CoreLoopFixedPoint(double xcur, double ycur)
{
    // Good old fixed-point. The STM32F103C8 is a 32-bit machine, doing
    // single-cycle 32-bit integer multiplication. We can therefore
    // use it to perform nice 6.26 fixed point, which will allow us
    // to zoom in the Mandelbrot set.
    int32_t re, im;
    unsigned k;
    int32_t rez, imz;
    int32_t t1, o1, o2, o3;

    re = FLOAT2FIXED(xcur);
    im = FLOAT2FIXED(ycur);

    rez = 0;
    imz = 0;

    k = 0;
    while (k < ITERATIONS) {
        o1 = MULFIX(rez, rez);
        o2 = MULFIX(imz, imz);
        o3 = MULFIX(rez-imz, rez-imz);
        t1 = o1 - o2;
        rez = t1 + re;
        imz = o1 + o2 - o3 + im;
        if (o1 + o2 > FOUR)
            break;
        k++;
    }

    return k;
}

void mandelFixedPoint(
    double xld, double yld, double xru, double yru, float bufferId)
{
    double xstep, ystep, xcur, ycur;

    xstep = (xru - xld)/MAXX;
    ystep = (yru - yld)/MAXY;

    for (int py=0; py<MAXY; py++) {
        uint16_t *pixel = &screen[py*MAXX];
        xcur = xld;
        ycur = yld + py*ystep;
        for (int px=0; px<MAXX; px++) {
            *pixel++ = lookup[ CoreLoopFixedPoint(xcur, ycur) ];
            xcur += xstep;
        }
    }
    // Fast SPI transfer of the memory buffer to the screen
    // The "bufferId" is a hack: if we are in entireScreen mode, 
    // it is used to blit the two screen buffers, at X offsets
    // 0 and 80 respectively. In the default single-page mode,
    // it just blits in the middle of the screen (X:40).
    tft.drawRGBBitmap(0, (int) (bufferId*MAXY), screen, MAXX, MAXY);
}

template <bool entireScreen>
void mandel()
{
    static int i = -1, di = 1;
    const int ZOOM_STEPS = 70;
    static double coords[ZOOM_STEPS][4];
    double xld, yld, xru, yru;
    static unsigned ms_start, ms_end;

    // Adapt starting viewpoint based on screen size
    if (entireScreen) {
        xld = -1.41; yld=-0.205; xru=-1.32; yru=-0.035;
    } else {
        xld = -1.41; yld=-0.165; xru=-1.32; yru=-0.075;
    }
    if (i == -1) {
        // First run; prepare the array with zoom coordinates
        double targetx = -1.398812, targety = -0.114164;
        for(int j=0; j<ZOOM_STEPS; j++) {
            coords[j][0] = xld;
            coords[j][1] = yld;
            coords[j][2] = xru;
            coords[j][3] = yru;
            xld += di*(targetx - xld)/10.;
            xru += di*(targetx - xru)/10.;
            yld += di*(targety - yld)/10.;
            yru += di*(targety - yru)/10.;
        }
        i = 0;
        if (!entireScreen) {
            tft.setCursor(1, 3);
            tft.setTextColor(ST7735_BLACK, ST7735_WHITE); // white on black
            tft.setTextSize(2);
            tft.setTextWrap(true);
            tft.print("ttsiod\n 2020 ");
        }
    }
    xld = coords[i][0];
    yld = coords[i][1];
    xru = coords[i][2];
    yru = coords[i][3];
    if (entireScreen) {
        // Render in two passes
        mandelFixedPoint(xld, yld, xru, yld+(yru-yld)/2, 0.0);
        mandelFixedPoint(xld, yld+(yru-yld)/2, xru, yru, 1.0);
    } else {
        ms_start = millis();
        mandelFixedPoint(xld, yld, xru, yru, 0.5);
        ms_end = millis();
        static char msg[16];
        double fps = 1000.0/(ms_end - ms_start);
        sprintf(msg, " FPS\n%3d.%1d", (int)fps, (int)(10.*fps) % 10);
        //sprintf(msg, "%u", ms_end - ms_start);
        tft.setCursor(10, 125);
        tft.setTextColor(ST7735_BLACK, ST7735_WHITE); // white on black
        tft.setTextSize(2);
        tft.setTextWrap(true);
        tft.print(msg);
    }

    // Cycle-through the pre-computed zoom locations
    i += di;
    if (i == ZOOM_STEPS-1)
        di = -1;
    if (i == 0)
        di = 1;
}

void setup(void)
{
    tft.initR(INITR_MINI160x80);   // initialize my ST7735 160x80 screen
    tft.fillScreen(ST7735_WHITE);  // Inverted - white is black :-)
    palette();
}

void loop()
{
    #include "config.h" // Emitted by the Makefile; this file
    // defines ENTIRE_SCREEN to true or false based on build target
    mandel<ENTIRE_SCREEN>();
}
