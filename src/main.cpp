#include <Arduino.h>
#include <AnimatedGIF.h>
#include <Arduino_GFX_Library.h>
#include <FS.h>
#include <SD_MMC.h>
#define GFX_BL 6  // backlight pin

// Create a dummy (software) SPI bus object (no real SPI used for RGB)
Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED /* DC */, GFX_NOT_DEFINED /* CS */,
    19 /* SCK (unused) */, 4 /* MOSI (unused) */, GFX_NOT_DEFINED /* MISO */);

// Create the ESP32RGBPanel with the pin mapping (DE, VSYNC, HSYNC, PCLK, R0..R4, G0..G5, B0..B4, HSYNC/VSYNC porches)
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40 /* DE */, 39 /* VSYNC */, 38 /* HSYNC */, 41 /* PCLK */,
    46 /* R0=phys R1 */,  3 /* R1=R2 */,  8 /* R2=R3 */, 18 /* R3=R4 */, 17 /* R4=R5 */,
    14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */,  9 /* G5 */,
     5 /* B0=phys B1 */, 45 /* B1=B2 */, 48 /* B2=B3 */, 47 /* B3=B4 */, 21 /* B4=B5 */,
     1 /* hsync polarity */, 10 /* hsync front porch */, 8 /* hsync pulse width */, 50 /* hsync back porch */,
     1 /* vsync polarity */, 10 /* vsync front porch */, 8 /* vsync pulse width */, 20 /* vsync back porch */
);

// Create the display object, specifying width=480, height=480, and pass ST7701 init sequence
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    480, 480, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* no RST pin used */,
    st7701_type5_init_operations, sizeof(st7701_type5_init_operations)
);
// === GIF DECODER ===
AnimatedGIF gif;
File gifFile;

void GIFDraw(GIFDRAW *pDraw) {
  if (pDraw->y >= 0 && pDraw->iWidth > 0) {
    gfx->draw16bitRGBBitmap(
      pDraw->iX, pDraw->iY,
      (uint16_t *)pDraw->pPixels,
      pDraw->iWidth, pDraw->iHeight);
  }
}

// File I/O callbacks for decoder
// Open the file via SD_MMC instead of SD.open()
void* GIFOpenFile(const char* fname, int32_t* pSize) {
  File* f = new File(SD_MMC.open(fname));
  if (!f || !f->available()) {
    delete f;
    return nullptr;
  }
  *pSize = f->size();
  return (void*)f;
}

void GIFCloseFile(void* handle) {
  File* f = (File*)handle;
  f->close();
  delete f;
}

int32_t GIFReadFile(GIFFILE* gf, uint8_t* buf, int32_t len) {
  File* f = (File*)gf->fHandle;
  return f->read(buf, len);
}

int32_t GIFSeekFile(GIFFILE* gf, int32_t pos) {
  File* f = (File*)gf->fHandle;
  f->seek(pos);
  return pos;
}

bool playing = false;
// Helper: open & start a file
bool openGIF(const char *filename) {
  gif.reset();                      // rewind any previous
  if (!gif.open(filename,
                GIFOpenFile, GIFCloseFile,
                GIFReadFile,  GIFSeekFile,
                GIFDraw)) {
    Serial.printf("Failed to open %s\n", filename);
    return false;
  }
  playing = true;
  gfx->fillScreen(RGB565_BLACK);
  Serial.printf("▶ Now playing %s\n", filename);
  return true;
}




void setup() {
  Serial.begin(115200);
  // Mount SD card using default SDMMC pins
  // 1) Init SD_MMC
  if (!SD_MMC.begin("/sdcard", false)) {  // false = use 4-bit mode
    Serial.println("Card Mount Failed");
    while (1);
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    while (1);
  }

  Serial.println("SD card initialized successfully!");
  // 2) Init your display



  // 2) Init your display
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
  gfx->begin();
  gfx->fillScreen(RGB565_BLACK);

  // 3) Init GIF decoder
  gif.begin(LITTLE_ENDIAN_PIXELS);


  // 2) init display
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
  gfx->begin();
  gfx->fillScreen(RGB565_BLACK);
  // 3) init GIF decoder
  gif.begin(LITTLE_ENDIAN_PIXELS);
  // 4) open GIF on SDMMC

  Serial.println("Ready. Send playgif1 or playgif2.");

  
}

void loop() {
  



// 1) Check for commands
  if (Serial.available() && !playing) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "playgif1") {
      openGIF("/test1.gif");
    }
    else if (cmd == "playgif2") {
      openGIF("/test2.gif");
    }
  }

  // 2) If playing, render next frame
  if (playing) {
    if (!gif.playFrame(true, NULL)) {
      // animation finished
      playing = false;
      Serial.println("✔ Playback finished");
    }
  }


  
}

/* 
void setup() {
  // Enable backlight
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  gfx->begin();               // Initialize the display
  gfx->fillScreen(RGB565_BLACK);
  gfx->setCursor(20, 20);
  gfx->setTextColor(RGB565_CYAN);
  gfx->println("Hello, ESP32-S3!");  
}

void loop() {
  // Your code – e.g. update graphics
}
*/