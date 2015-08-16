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
SPISettings settings(24000000, MSBFIRST, SPI_MODE0); // 24 MHz max

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

  uint8_t  x, y;
  uint16_t p, a;
  uint32_t d, scale;

  scale = analogRead(0); // Read pot on A0
  // Set up for fixed-point linear scaling calculation later on:
  scale = map(scale * scale, 0, 1023 * 1023, IMG_HEIGHT, IMG_HEIGHT * 64);

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
  for(y=0; y<128; y++) {                  // For each row...
    for(x=0; x<128; x++) {                // For each column...
        p = polar[y][x];                  // Fetch angle/dist polar data
        d = (scale * (p & 0x7F)) / 128;   // Distance (Y coord)
      if(d < IMG_HEIGHT) {                // Within iris area?
        a = (IMG_WIDTH * (p >> 7)) / 512; // Angle (X coord)
        p = img[d][a];                    // Fetch 16-bit pixel value from map
        SPI.transfer(p >> 8);
        SPI.transfer(p & 0xFF);
      } else { // Out-of-bounds pixel -- show black
        SPI.transfer(0);
        SPI.transfer(0);
      }
    }
  }

  digitalWrite(TFT_CS, HIGH);
  SPI.endTransaction();
}

