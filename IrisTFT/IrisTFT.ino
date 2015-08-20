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
// process) to screen coordinates.  So it's very quick (60-80 fps for
// regular or overclocked Teensy) while still providing the cool and
// realistic muscle contraction effect of the iris.

#include <Adafruit_GFX.h>       // Core graphics library
#include <SPI.h>
#include <Adafruit_ST7735.h>    // TFT LCD library
#include <Adafruit_SSD1351.h>   // Adafruit OLED library
//#include <FTOLED.h>             // Freetronics OLED library
//#include <SD.h>                 // Also required if using FTOLED lib
#include "eyeData.h"            // Huge graphics tables are here

// TFT control lines:
#define OLED_CS     10
#define OLED_RST    9
#define BOTH_DC     8 // Data/command line on both displays
#define TFT_CS      7
#define TFT_RST     6
#define DIAL        A6
Adafruit_ST7735     tft(TFT_CS, BOTH_DC, TFT_RST);
#ifdef SSD1351WIDTH // Adafruit OLED library
 Adafruit_SSD1351   oled(OLED_CS, BOTH_DC, OLED_RST);
#else               // Freetronics OLED library
 OLED               oled(OLED_CS, BOTH_DC, OLED_RST);
#endif

// Newfangled SPI transaction stuff is used for fast SPI on Teensy 3.1:
SPISettings settings(24000000, MSBFIRST, SPI_MODE0); // 24 MHz max

void setup(void) {
  Serial.begin(9600);
  oled.begin();
  tft.initR(INITR_144GREENTAB);
}

long frames = 0;

void loop() {
  drawEyeOLED();
  drawEyeTFT();
  Serial.println(++frames / (millis() / 1000)); // Print frame rate
}

#ifndef SSD1351WIDTH
// These are protected functions in FTOLED, need a copy here
inline void writeCommand(byte command) {
  digitalWrite(BOTH_DC, LOW);
  SPI.transfer(command);
  digitalWrite(BOTH_DC, HIGH);
}

inline void writeData(byte data) {
  SPI.transfer(data);
}
#endif

void drawEyeOLED(void) {

  uint8_t  x, y;
  uint16_t p, a;
  uint32_t d, scale, sr;
  uint32_t tmp __attribute__((unused));

  scale = analogRead(DIAL); // Read pot on A0
  // Set up for fixed-point linear scaling calculation later on:
  scale = map(scale * scale, 0, 1023 * 1023, IMG_HEIGHT, IMG_HEIGHT * 64);

  // Set up raw write to full screen:
  SPI.beginTransaction(settings);

#ifdef SSD1351WIDTH // Adafruit library
  oled.writeCommand(SSD1351_CMD_SETCOLUMN);
  oled.writeData(0);
  oled.writeData(127);
  oled.writeCommand(SSD1351_CMD_SETROW);
  oled.writeData(0);
  oled.writeData(127);
  oled.writeCommand(SSD1351_CMD_WRITERAM);
#else // Freetronics library
  digitalWrite(OLED_CS, LOW);
  writeCommand(0x15);
  writeData(0);
  writeData(127);
  writeCommand(0x75);
  writeData(0);
  writeData(127);
  writeCommand(0x5C);
#endif

  digitalWrite(BOTH_DC, HIGH);
  digitalWrite(OLED_CS, LOW);
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
        // SPI FIFO technique from Paul Stoffregen's ILI9341_t3 library.
        // This line is effectively that lib's writedata16_cont(p):
        KINETISK_SPI0.PUSHR = p | SPI_PUSHR_CTAS(1) | SPI_PUSHR_CONT;
      } else { // Out-of-bounds pixel -- show black (writedata16_cont(0)):
        KINETISK_SPI0.PUSHR = SPI_PUSHR_CTAS(1) | SPI_PUSHR_CONT;
      }
      do { // And this is from the same lib's waitFifoNotFull() function:
        sr = KINETISK_SPI0.SR;
        if(sr & 0xF0) tmp = KINETISK_SPI0.POPR;
      } while(sr & 0xC000);
    }
  }

  SPI.endTransaction();
  delayMicroseconds(3);
  digitalWrite(OLED_CS, HIGH);
}

void drawEyeTFT(void) {

  uint8_t  x, y;
  uint16_t p, a;
  uint32_t d, scale, sr;
  uint32_t tmp __attribute__((unused));

  scale = analogRead(DIAL); // Read pot on A0
  // Set up for fixed-point linear scaling calculation later on:
  scale = map(scale * scale, 0, 1023 * 1023, IMG_HEIGHT, IMG_HEIGHT * 64);

  // Set up raw write to full screen:
  SPI.beginTransaction(settings);

  tft.setAddrWindow(0, 0, 127, 127);
  digitalWrite(BOTH_DC, HIGH);
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
        // SPI FIFO technique from Paul Stoffregen's ILI9341_t3 library.
        // This line is effectively that lib's writedata16_cont(p):
        KINETISK_SPI0.PUSHR = p | SPI_PUSHR_CTAS(1) | SPI_PUSHR_CONT;
      } else { // Out-of-bounds pixel -- show black (writedata16_cont(0)):
        KINETISK_SPI0.PUSHR = SPI_PUSHR_CTAS(1) | SPI_PUSHR_CONT;
      }
      do { // And this is from the same lib's waitFifoNotFull() function:
        sr = KINETISK_SPI0.SR;
        if(sr & 0xF0) tmp = KINETISK_SPI0.POPR;
      } while(sr & 0xC000);
    }
  }

  SPI.endTransaction();
  delayMicroseconds(3);
  digitalWrite(TFT_CS, HIGH);
}

