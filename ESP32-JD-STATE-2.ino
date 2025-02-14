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
  String tempText = "Vending State";
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);

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
