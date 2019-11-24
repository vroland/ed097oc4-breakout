/* Simple firmware for a ESP32 displaying a static image on an EPaper Screen.
 *
 * Write an image into a header file using a 3...2...1...0 format per pixel,
 * for 4 bits color (16 colors - well, greys.) MSB first.  At 80 MHz, screen
 * clears execute in 1.075 seconds and images are drawn in 1.531 seconds.
 */
#include "Arduino.h"
//#include "image.hpp"
#include "EPD.hpp"
#include "font.h"
#include "firasans.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <time.h>
#include <WiFi.h>

EPD* epd;
SPIClass* spi;

void draw_caption(File* file) {
    int fn_len = strlen(file->name());
    char* caption = (char*)malloc(fn_len);
    char* filename = (char*)file->name();
    char* fn_ptr = strrchr(filename, '/');
    fn_ptr++;
    char* ptr = caption;
    while (fn_ptr < strrchr(filename, '.')) {
        *(ptr++) = *(fn_ptr++);
    }
    *(ptr++) = 0;
    Serial.print("Displaying: ");
    Serial.println(caption);

    int x, y, w, h;
    getTextBounds((GFXfont*)&FiraSans, (unsigned char*)caption, 0, 100, &x, &y, &w, &h);

    x = 1100 - w;
    Rect_t font_area = Rect_t {
        .x = x - 5,
        .y = y - 5,
        .width = w + 10,
        .height = h + 10,
    };
    epd->draw_byte(&font_area, 80, 0B10101010);
    y = 100;
    writeln((GFXfont*)&FiraSans, (unsigned char*)caption, &x, &y, epd);
    free(caption);
}

void setup() {
    Serial.begin(115200);
    epd = new EPD(1200, 825);

    spi = new SPIClass();
    spi->begin(22, 35, 21, 12);

    if(!SD.begin(12, *spi, 80000000)){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void loop() {
    File root = SD.open("/");
    File file = root.openNextFile();
    while(file) {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("  SIZE: ");
        Serial.print(file.size());
        time_t t= file.getLastWrite();
        struct tm * tmstruct = localtime(&t);
        Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);

        epd->poweron();
        epd->clear_screen();
        Serial.println(epd->draw_sd_image(&file));
        draw_caption(&file);
        epd->poweroff();
        file.close();
        file = root.openNextFile();
        delay(10000);
    }
    root.close();
}
