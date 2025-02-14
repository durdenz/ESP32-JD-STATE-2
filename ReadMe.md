# SD Card Reader - JPEG Test

## Notes:

### ESP32 to TFT LCD Interface

| TFT LCD Touchscreen | ESP32 | COLOR |
| :--- | :--- | :--- |
| T_IRQ | GPIO 36 | BROWN |
| T_OUT | GPIO 39 | GRAY |
| T_DIN | GPIO 32 | YELLOW |
| T_CS | GPIO 33 | PURPLE |
| T_CLK | GPIO 25 | BLUE |
| SDO(MISO) | GPIO 12 | WHITE |
| LED | GPIO 21 | GREEN |
| SCK | GPIO 14 | YELLOW |
| SDI(MOSI) | GPIO 13 | BROWN |
| D/C | GPIO 2 | ORANGE |
| RESET | EN/RESET | ORANGE |
| CS  | GPIO 15 | GREEN |
| GND | GND | BLACK |
| VCC | 3V3 | RED |

| SD CARD READER | ESP32 | COLOR |
| :--- | :--- | :--- |
| SD_CS | GPIO 5 | BLUE |
| SD_MOSI | GPIO 13 | PURPLE |
| SD_MISO | GPIO 12 | GRAY |
| SD_SCK | GPIO 14 | WHITE |

## PreRequisites
- Connect ESP32 with TFT Display/Touchscreen and SD Card Reader per the wiring table above
- Place image files from SDCARD folder onto SD Card
- Ensure that the following Libraries are installed in the Arduino IDE:
    1. TFT_eSPI
    2. SD
    3. FS
    4. JPEGDecoder (Bodmer)

## Build and Download
1. Copy ESP32_JD_STATE_2.ino into a new Sketch folder in Arduino IDE
2. Set Arduino IDE to ESP32 Dev Module and the correct USB port
3. Build and Upload Sketch to ESP32
4. Open Serial Monitor and set Baud Rate to 9600 baud