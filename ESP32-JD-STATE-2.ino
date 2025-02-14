// Simple State Machine Based Soda Dispenser
// GD4 and GD5 with special guest GD3
#include <TimeLib.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <FS.h>
#include <SD.h>
#include <JPEGDecoder.h>

TFT_eSPI tft = TFT_eSPI();

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

// SD Card Reader pins
#define SD_CS 5 // Chip Select

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2

// Define Button 1
#define BTN1_X 30
#define BTN1_Y 150
#define BTN1_WIDTH 240
#define BTN1_HEIGHT 35

// Define State Machine States
#define STATE_ACTIVE 1
#define STATE_STANDBY 2
#define STATE_VENDING 3

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

// State Machine Current State
int currentState;

// Indicate State Change is in Progess
bool changeState = false;

// Standby Timer Variables
#define STANDBY_DURATION 1
time_t standbyStart = now();

// Function to see if Button 1 has been pressed
// Inputs are X and Y from touchscreen press
// Returns true if X and Y are within Button 1 area
// Returns false otherwise
//
bool btn1_pressed(int x,int y) {
  // Check to see if the coordinates of the touch fall inside the range of the button
  if (((x>=BTN1_X) && (x<=(BTN1_X+BTN1_WIDTH))) && ((y>=BTN1_Y)&&(y<=(BTN1_Y+BTN1_HEIGHT)))) {
    Serial.println("Button 1 Pressed");
    return(true);
  } else {
    Serial.println("Button 1 Not Pressed");
    return(false);
  }
}

// Initialize SD Card Reader
bool SD_Init(int cs) {
  if (!SD.begin(cs, tft.getSPIinstance())) {
    Serial.println("Card Mount Failed");
    return(false);
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return(false);
  }

    Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  Serial.println("SD Card initialization Complete");
  return(true);
}

// ======================================================
// BEGIN - JPEG Support Functions
//

//####################################################################################################
// Draw a JPEG on the TFT pulled from SD Card
//####################################################################################################
// xpos, ypos is top left corner of plotted image
void drawSdJpeg(const char *filename, int xpos, int ypos) {

  // Open the named file (the Jpeg decoder library will close it)
  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library
 
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");

  // Use one of the following methods to initialise the decoder:
  bool decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
  //bool decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)

  if (decoded) {
    // print information about the image to the serial port
    jpegInfo();
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

//####################################################################################################
// Draw a JPEG on the TFT, images will be cropped on the right/bottom sides if they do not fit
//####################################################################################################
// This function assumes xpos,ypos is a valid screen coordinate. For convenience images that do not
// fit totally on the screen are cropped to the nearest MCU size and may leave right/bottom borders.
void jpegRender(int xpos, int ypos) {

  //jpegInfo(); // Print information from the JPEG file (could comment this line out)

  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);
  
  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

  // save the current image block size
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // Fetch data from the file, decode and display
  while (JpegDec.read()) {    // While there is more data in the file
    pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)

    // Calculate coordinates of top left corner of current MCU
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++)
      {
        p += mcu_w;
        for (int w = 0; w < win_w; w++)
        {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    // calculate how many pixels must be drawn
    uint32_t mcu_pixels = win_w * win_h;

    // draw image MCU block only if it will fit on the screen
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= tft.height())
      JpegDec.abort(); // Image has run off bottom of screen so abort decoding
  }

  tft.setSwapBytes(swapBytes);

  showTime(millis() - drawTime); // These lines are for sketch testing only
}

//####################################################################################################
// Print image information to the serial port (optional)
//####################################################################################################
// JpegDec.decodeFile(...) or JpegDec.decodeArray(...) must be called before this info is available!
void jpegInfo() {

  // Print information extracted from the JPEG file
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}

//####################################################################################################
// Show the execution time (optional)
//####################################################################################################
// WARNING: for UNO/AVR legacy reasons printing text to the screen with the Mega might not work for
// sketch sizes greater than ~70KBytes because 16-bit address pointers are used in some libraries.

// The Due will work fine with the HX8357_Due library.

void showTime(uint32_t msTime) {
  //tft.setCursor(0, 0);
  //tft.setTextFont(1);
  //tft.setTextSize(2);
  //tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.print(F(" JPEG drawn in "));
  //tft.print(msTime);
  //tft.println(F(" ms "));
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}

// END -- JPEG Support Functions
// ======================================================

// Perform Actions for Vending State
void StateVending() {
  Serial.println("Entering State = Vending");
  
  // Set current state to Vending
  currentState = STATE_VENDING;

  // Clear TFT screen 
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Prepare to Print to screen
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  int textY = 80;
 
  // Print to screen
  // String tempText = "Vending State";
  // tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);

  drawSdJpeg("/lena20k.jpg", 0, 0);     // This draws a jpeg pulled off the SD Card
  
  // Simulate Vending with Delay
  delay(5000);
  
  // Vending is complete - Set State to Active 
  StateActive();
}

// Perform Actions for Active State
void StateActive() {
  Serial.println("Entering State = Active");
  
  // Set current state to Active
  currentState = STATE_ACTIVE;

  // Clear TFT screen 
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Prepare to Print to screen
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  int textY = 80;
 
  // Print to screen
  String tempText = "Active State";
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);
  tft.drawCentreString("Welcome to Jordes Sodaland", centerX, 30, FONT_SIZE);
  tft.drawCentreString("Press the button for a Soda!", centerX, centerY, FONT_SIZE);

  // Draw Button
  tft.drawRect( BTN1_X, BTN1_Y, BTN1_WIDTH, BTN1_HEIGHT, TFT_BLACK);

  // Start Time for Inactive Timer
  standbyStart = now();

  // Set Change State to False to indicate State Handling is complete
  changeState = false;
}

// Perform Actions for StandBy State
void StateStandBy() {
  Serial.println("Entering State = StandBy");

  // Set current state to StandBy
  currentState = STATE_STANDBY;

  // Clear TFT screen 
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // Prepare to Print to screen
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
  int textY = 80;
 
  // Print to screen
  String tempText = "StandBy State";
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);
  
  // Set Change State to False to indicate State Handling is complete
  changeState = false;
}

void setup() {
  Serial.begin(9600);

  // Start the SPI for the touchscreen and init the touchscreen
  //
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);

  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 3: touchscreen.setRotation(3);
  //
  touchscreen.setRotation(3);

  // Start the tft display
  //
  tft.init();

  // Set the TFT display rotation in landscape mode
  //
  tft.setRotation(1);

  // Clear the screen before writing to it
  //
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  // Initialize SD Card Reader
if (SD_Init(SD_CS) == false) {
  Serial.println("SD Card Initialization Failed");
}
  
  // Set initial State to Active State
  StateActive();
}

void loop() {
  // Checks if Touchscreen was touched, then checks if button pressed and changes state
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    // Touchscreen event has occurred
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;


    if (currentState == STATE_ACTIVE) {
      // Check and see if touch was pressing Button 1
      // If so, change state to vending
      if (btn1_pressed(x, y)) {
        StateVending();
      } 
    } else if (currentState == STATE_STANDBY) {
        StateActive();
    } else if (currentState == STATE_VENDING) {
      // Do Nothing for now
    }

    // Set Polling Period for button presses (in millisecs)
    delay(100);
  } else {    // No Touchscreen Event
    // Check and see if inactivity time has been exceeded and if so, go into standby state
    if ((currentState == STATE_ACTIVE) && ((minute(now()) - minute(standbyStart)) >= STANDBY_DURATION)) {
      StateStandBy();
    }
  }
}
