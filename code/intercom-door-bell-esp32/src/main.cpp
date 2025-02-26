#include <Arduino.h>
#include <Bounce2.h>
#include "amplifier.h"
#include "waveGenerator.h"
#include "bellDetector.h"

#define PIN_bell /*             */ 34

#define PIN_btn_testSound /*    */ 4
#define PIN_btn_volumedown /*   */ 16
#define PIN_btn_volumedown /*   */ 16
#define PIN_btn_volumeUp /*     */ 17
#define PIN_btn_selectDown /*   */ 5
#define PIN_btn_selectUp /*     */ 18
#define PIN_btn_openDoor /*     */ 19

#define PIN_I2S_BCK /*          */ 33
#define PIN_I2S_LRCK /*         */ 25
#define PIN_I2S_DATA /*         */ 32

Bounce2::Button btn_testSound = Bounce2::Button();
Bounce2::Button btn_volumedown = Bounce2::Button();
Bounce2::Button btn_volumeUp = Bounce2::Button();
Bounce2::Button btn_selectDown = Bounce2::Button();
Bounce2::Button btn_selectUp = Bounce2::Button();
Bounce2::Button btn_openDoor = Bounce2::Button();

WaveGenerator waveGenerator = WaveGenerator(44100);

void prepareButton(Bounce2::Button &btn, int pin)
{
  btn.attach(pin, INPUT);
  btn.interval(5);
  btn.setPressedState(LOW);
}

void setup()
{
  Serial.begin(115200);

  prepareButton(btn_testSound, PIN_btn_testSound);
  prepareButton(btn_volumedown, PIN_btn_volumedown);
  prepareButton(btn_volumeUp, PIN_btn_volumeUp);
  prepareButton(btn_selectDown, PIN_btn_selectDown);
  prepareButton(btn_selectUp, PIN_btn_selectUp);
  prepareButton(btn_openDoor, PIN_btn_openDoor);

  if (!Amplifier.begin(
          0,
          PIN_I2S_BCK,
          PIN_I2S_LRCK,
          PIN_I2S_DATA,
          44100,
          [](I2S_Frame *data, int32_t len) -> void
          {
            waveGenerator.generate((WaveGenerator_Frame *)data, len);
          }))
  {
    Serial.println("MAX i2s driver initialization Failed");
    // return;
  }

  waveGenerator.playing(false);
  waveGenerator.amplitude(100);
  waveGenerator.freq(220);

  BellDetector.begin(
      PIN_bell,
      []() -> void
      {
        Serial.println("bell pressed");
        waveGenerator.playing(true);
      },
      []() -> void
      {
        Serial.println("bell released");
        waveGenerator.playing(false);
      });

  Serial.println("started");
}

void loop()
{
  // BellDetector.run();

  // Buttons:
  btn_testSound.update();
  btn_volumedown.update();
  btn_volumeUp.update();
  btn_selectDown.update();
  btn_selectUp.update();
  btn_openDoor.update();

  if (btn_testSound.pressed())
  {
    // btn_testSound
    Serial.println("btn_testSound");
    waveGenerator.playing(!waveGenerator.playing());
  }
  if (btn_volumedown.pressed())
  {
    // btn_volumedown

    Serial.println("btn_volumedown");

    waveGenerator.amplitude(max(waveGenerator.amplitude() - 500.0, 0.0));
  }
  if (btn_volumeUp.pressed())
  {
    // btn_volumeUp
    Serial.println("btn_volumeUp");
    waveGenerator.amplitude(min(waveGenerator.amplitude() + 500.0, +32768.0));
  }
  if (btn_selectDown.pressed())
  {
    // btn_selectDown
    Serial.println("btn_selectDown");
    waveGenerator.freq(max((unsigned int)waveGenerator.freq() - 10, (unsigned int)10));
  }
  if (btn_selectUp.pressed())
  {
    // btn_selectUp
    Serial.println("btn_selectUp");
    waveGenerator.freq(min((unsigned int)waveGenerator.freq() + 10, (unsigned int)20000));
  }
  if (btn_openDoor.pressed())
  {
    // btn_openDoor
    Serial.println("btn_openDoor");
  }
  taskYIELD();
}
