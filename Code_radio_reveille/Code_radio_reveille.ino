#include <Wire.h>
#include "RTClib.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

RTC_PCF8523 rtc;

/*
char daysOfTheWeek[7][12] = {
  "Dimanche", "Lundi", "Mardi", "Mercredi",
  "Jeudi", "Vendredi", "Samedi"
}
*/

#define TFT_CS  10
#define TFT_DC  12
#define TFT_RST 6

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

#define ENC_A A3
#define ENC_B A4

int encoderValue = 0;
int ApresA = 0;

unsigned long lastDisplayUpdate = 0;

//////////////////////////////////////////////////////////

void setup() {

  Serial.begin(57600);
  

  // ===== RTC =====
  if (!rtc.begin()) {
    Serial.println("RTC non trouvee");
    while (1);
  }

  if (!rtc.initialized() || rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  rtc.start();

  ///////////////TFT ////////////
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Radio");

  //// Encodeur  //////
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);

  ApresA = digitalRead(ENC_A);
}

//////////////////////////////////////////////////////////

void loop() {
  lireEncodeur();
  afficherEcran();
}

//////////////////////////////////////////////////////////
// LECTURE ENCODEUR
//////////////////////////////////////////////////////////

void lireEncodeur() {

  int AvantA = digitalRead(ENC_A);

  if (AvantA != ApresA) {

    if (digitalRead(ENC_B) != AvantA)
      encoderValue++;
    else
      encoderValue--;
  }

  ApresA = AvantA;
}


///// AFFICHAGE//////


void afficherEcran() {

  if (millis() - lastDisplayUpdate < 500) return;
  lastDisplayUpdate = millis();

  DateTime now = rtc.now();

  tft.fillRect(0, 40, 320, 120, ILI9341_BLACK);

  // Heure
  tft.setCursor(10, 50);
  tft.setTextColor(ILI9341_CYAN);
  tft.print("Heure: ");

  if (now.hour() < 10) tft.print("0");
  tft.print(now.hour());
  tft.print(":");

  if (now.minute() < 10) tft.print("0");
  tft.print(now.minute());
  tft.print(":");

  if (now.second() < 10) tft.print("0");
  tft.println(now.second());

  // Encodeur//
  tft.setCursor(10, 80);
  tft.setTextColor(ILI9341_YELLOW);
  tft.print("Volume: ");
  tft.println(encoderValue);
}   dans ce code quand je coupe l alime la RTC garde L hure d arret a la place d affiche l heure de l alumage 