// Iris demo for PJRC Teensy 3.1 with Adafruit 1.5" TFT LCD (product #2088).
// This WILL NOT WORK on normal Arduino or other boards -- there's a huge
// amount of data (in the file eyeData.h) that just won't fit there.

// This is totally based on David Boccabella's animatronic eye concept:
// forums.stanwinstonschool.com/discussion/1470/animatronic-pupil-for-eye
// with some workarounds for a bottleneck he was encountering.

// Basically: SD cards are slow when accessed over SPI.  Faster MCU or
// frequency won't fix this, it's a card bottleneck (4-bit fast parallel
// interface is used for most consumer devices like cameras & card readers).
// Rather than storing the iris image(s) on the card, a single iris image
// resides in program flash memory (would be PROGMEM on a normal Arduino,
// on Teensy3 it's simply 'const') using a polar projection, then two
// tables are used to quickly de-warp this (performing scaling in the
// process) to screen coordinates.  So it's very quick (30-40 fps for
// regular or overclocked Teensy) while still providing the cool and
// realistic muscle contraction effect of the iris.
// More notes in eyeData.h.

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include "eyeData.h"         // Huge graphics tables are here

// TFT control lines:
#define TFT_CS  10
#define TFT_RST  9
#define TFT_DC   8
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Newfangled SPI transaction stuff is used for fast SPI on Teensy 3.1:
SPISettings settings(32000000, MSBFIRST, SPI_MODE0); 

void setup(void) {
  Serial.begin(9600);
  tft.initR(INITR_144GREENTAB);
}

long frames = 0;

void loop() {
  drawEye();
  Serial.println(++frames / (millis() / 1000)); // Print frame rate
}

void drawEye(void) {

  uint8_t  x, y, a, lo;
  uint16_t pixel;
  uint32_t d, scale;

  scale = analogRead(0);               // Read pot on A0
  scale = 256 + (scale * scale) / 256; // Setup for fixed-point math later

  // Set up raw write to full screen:
  SPI.beginTransaction(settings);
  tft.setAddrWindow(0, 0, 127, 127);
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);
  // Can now just issue raw 16-bit pixel values...

  // Currently this does math on and writes to every pixel,
  // even the black pixels in the corner that are never used.
  // Could perhaps do further optimization to skip those,
  // either using several setAddrWindows as needed, or a
  // tighter loop of zero-writes on those pixels...
  // not stressing over this right now, already stupid fast.

  for(y=0; y<128; y++) {               // For each row...
    for(x=0; x<128; x++) {             // For each column...
      d = (dist[y][x] * scale) / 1024; // Fetch polar distance, do scale
      if((d > 0) && (d < 64)) {        // Within iris area?
        a     = angle[y][x];           // Fetch polar angle value for pixel
        pixel = img[d][a];             // Fetch 16-bit pixel value from map
#ifdef PIPELINE
        // Haven't extensively benchmarked this and don't know if SPI
        // pipelining really benefits anything here on the Teensy3, so
        // this is disabled by default.
        while(!(SPSR & _BV(SPIF))); // SPSR and SPDR are of course AVR registers,
        SPDR = pixel >> 8;          // but Teensyduino adapts to Teensy3 equivs
        lo = pixel & 0xFF;          // because Paul S. is awesome that way.
        while(!(SPSR & _BV(SPIF))); // Wouldn't surprise me if he has SPI.transfer()
        SPDR = lo;                  // already doing pipelining dy default.
#else
        // Use normal vanilla transfers by default.
        SPI.transfer(pixel >> 8);
        SPI.transfer(pixel & 0xFF);
#endif
      } else { // Out-of-bounds pixel -- show black
#ifdef PIPELINE
        while(!(SPSR & _BV(SPIF)));
        SPDR = 0;
        while(!(SPSR & _BV(SPIF)));
        SPDR = 0;
#else
        SPI.transfer(0);
        SPI.transfer(0);
#endif
      }
    }
  }
#ifdef PIPELINE
  while(!(SPSR & _BV(SPIF)));
#endif

  digitalWrite(TFT_CS, HIGH);
  SPI.endTransaction();
}

