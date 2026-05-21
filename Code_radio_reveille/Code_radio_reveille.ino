#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <RTClib.h>
#include <radio.h>
#include <RDA5807M.h>
#include <Adafruit_ZeroI2S.h>

// ================================================================
// CONSTANTES RADIO / TFT
// ================================================================
#define FIX_BAND    RADIO_BAND_FM
#define FIX_STATION 8880
#define FIX_VOLUME   3
#define FAV_STATION 10370   // 103.7 MHz

#define TFT_CS  10
#define TFT_DC  12
#define TFT_RST  6

#define ENC_A    A3
#define ENC_B    A4
#define ENC_F    13
#define ENC_F2   11
#define BP_Mute  A5
#define BP_Fav   5

#define GAUCHE   A0

#define VOLUME_MAX   5
#define VOLUME_MIN   0
#define FREQ_MIN  8750
#define FREQ_MAX 10800
#define FREQ_STEP   10

// ================================================================
// OBJETS
// ================================================================
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
RTC_PCF8523      rtc;
RDA5807M         radio;
Adafruit_ZeroI2S i2s;

// ================================================================
// VARIABLES
// ================================================================
int           Volume            = FIX_VOLUME;
int           Frequence         = FIX_STATION;
unsigned long lastInterruptVol  = 0;
unsigned long lastInterruptFreq = 0;
unsigned long lastHeureUpdate   = 0;
int           volumeAvantMute   = FIX_VOLUME;
DateTime      heureDepart;
unsigned long millisDepart      = 0;

// ================================================================
// PROTOTYPES
// ================================================================
void afficherVolume();
void afficherFrequence();
void afficherHeure();
void Interruption_ENC_A();
void Interruption_ENC_F();
void Interruption_BP_Mute();
void Interruption_BP_Fav();

// ================================================================
// AFFICHAGE
// ================================================================
void afficherVolume() {
  tft.setTextSize(2);
  tft.fillRect(106, 80, 60, 16, ILI9341_BLACK);
  tft.setCursor(10, 80);
  tft.setTextColor(ILI9341_GREEN);
  tft.print("Volume: ");
  tft.print(Volume);
}

void afficherFrequence() {
  tft.setTextSize(2);
  tft.fillRect(82, 110, 238, 16, ILI9341_BLACK);
  tft.setCursor(10, 110);
  tft.setTextColor(ILI9341_MAGENTA);
  tft.print("Freq: ");
  tft.print(Frequence / 100);
  tft.print(".");
  if ((Frequence % 100) < 10) tft.print("0");
  tft.print(Frequence % 100);
  tft.print(" MHz");
}

void afficherHeure() {
  if (millis() - lastHeureUpdate < 60000) return;
  lastHeureUpdate = millis();

  unsigned long secondesEcoulees = (millis() - millisDepart) / 1000;
  DateTime now = heureDepart + TimeSpan(secondesEcoulees);

  tft.setTextSize(2);
  tft.fillRect(94, 50, 226, 16, ILI9341_BLACK);
  tft.setCursor(10, 50);
  tft.setTextColor(ILI9341_CYAN);
  tft.print("Heure: ");
  if (now.hour()   < 10) tft.print("0");
  tft.print(now.hour());   tft.print(":");
  if (now.minute() < 10) tft.print("0");
  tft.print(now.minute());
}

// ================================================================
// SETUP
// ================================================================
void setup() {
  pinMode(A2,OUTPUT);
  delay(3000);
  Serial.begin(115200);

  // RTC — lecture unique au démarrage
  if (!rtc.begin()) {
    Serial.println("RTC non trouvee");
    while (1);
  }
  if (!rtc.initialized() || rtc.lostPower())
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  rtc.start();
  heureDepart  = rtc.now();
  millisDepart = millis();

  // TFT
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 10);
  tft.println("Radio");
  afficherVolume();
  afficherFrequence();

  // Affichage immédiat de l'heure
  lastHeureUpdate = millis() - 60000;
  afficherHeure();

  // Encodeurs + boutons
  pinMode(ENC_A,   INPUT_PULLUP);
  pinMode(ENC_B,   INPUT_PULLUP);
  pinMode(ENC_F,   INPUT_PULLUP);
  pinMode(ENC_F2,  INPUT_PULLUP);
  pinMode(BP_Mute, INPUT_PULLUP);
  pinMode(BP_Fav,  INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A),   Interruption_ENC_A,   FALLING);
  attachInterrupt(digitalPinToInterrupt(ENC_F),   Interruption_ENC_F,   FALLING);
  attachInterrupt(digitalPinToInterrupt(BP_Mute), Interruption_BP_Mute, FALLING);
  attachInterrupt(digitalPinToInterrupt(BP_Fav),  Interruption_BP_Fav,  FALLING);

  // Radio
  radio.setup(RADIO_FMSPACING,  RADIO_FMSPACING_100);
  radio.setup(RADIO_DEEMPHASIS, RADIO_DEEMPHASIS_50);
  if (!radio.initWire(Wire)) {
    Serial.println("no radio chip found.");
    delay(4000);
  }
  radio.setBandFrequency(FIX_BAND, FIX_STATION);
  radio.setVolume(FIX_VOLUME * 3);
  radio.setMono(false);
  radio.setMute(false);

  // I2S — MAX98357A
  i2s.begin(I2S_32_BIT, 22050);
  i2s.enableTx();

  Serial.println("Init OK");
}

// ================================================================
// LOOP
// ================================================================
void loop() {

  afficherHeure();
digitalWrite(A2,1);
  int ADC_Gauche = analogRead(GAUCHE);
digitalWrite(A2,0);
  int32_t audioGauche = ((int32_t)ADC_Gauche - 2048) * 1048576;
  i2s.write(audioGauche, audioGauche);

}

// ================================================================
// INTERRUPTIONS
// ================================================================
void Interruption_ENC_A() {
  if ((millis() - lastInterruptVol) > 500) {
    lastInterruptVol = millis();
    if (digitalRead(ENC_B)) Volume++;
    else                     Volume--;
    if (Volume > VOLUME_MAX) Volume = VOLUME_MAX;
    if (Volume < VOLUME_MIN) Volume = VOLUME_MIN;

    if (Volume == VOLUME_MIN) {
      radio.setMute(true);
    } else {
      radio.setMute(false);
      radio.setVolume(Volume * 3);
    }
    afficherVolume();
  }
}

void Interruption_ENC_F() {
  if ((millis() - lastInterruptFreq) > 150) {
    lastInterruptFreq = millis();
    if (digitalRead(ENC_F2)) Frequence += FREQ_STEP;
    else                      Frequence -= FREQ_STEP;
    if (Frequence > FREQ_MAX) Frequence = FREQ_MAX;
    if (Frequence < FREQ_MIN) Frequence = FREQ_MIN;
    radio.setFrequency(Frequence);
    afficherFrequence();
  }
}

void Interruption_BP_Mute() {
  static unsigned long lastChange = 0;
  if ((millis() - lastChange) < 50) return;
  lastChange = millis();

  if (digitalRead(BP_Mute) == LOW) {
    if (Volume == 0) {
      Volume = volumeAvantMute;
      radio.setMute(false);
      radio.setVolume(Volume * 3);
    } else {
      volumeAvantMute = Volume;
      Volume = 0;
      radio.setMute(true);
    }
    afficherVolume();
  }
}

void Interruption_BP_Fav() {
  static unsigned long lastChange = 0;
  if ((millis() - lastChange) < 50) return;
  lastChange = millis();

  if (digitalRead(BP_Fav) == LOW) {
    Frequence = FAV_STATION;
    radio.setFrequency(Frequence);
    afficherFrequence();
  }
}
