/* Talkie library  Copyright 2011 Peter Knight
   Talkie is a speech synthesiser that works from a fixed vocabulary.

   Demo put together by Gadget Reboot

   Original library:  https://github.com/going-digital/Talkie/tree/master/Talkie
   Updated library:   https://github.com/ArminJo/Talkie

   Voice PWM output pins for different ATmegas:
    ATmega328 (Uno and Nano): non inverted at pin 3, inverted at pin 11.
    ATmega2560: non inverted at pin 6, inverted at pin 7.
    ATmega32U4 (Leonardo): non inverted at pin 10, inverted at pin 9.
    ATmega32U4 (CircuitPlaygound): only non inverted at pin 5.

   Connect speaker between pin 3 and pin 11

   Compiled for Uno, using Arduino IDE 1.8.10, Talkie (ArminJo) v1.0.2
   Sketch uses 8202 bytes (25%) of program storage space. Maximum is 32256 bytes.
   Global variables use 145 bytes (7%) of dynamic memory, leaving 1903 bytes for local variables. Maximum is 2048 bytes.


*/
#include <Arduino.h>
#include <Servo.h>

#include "Talkie.h"
#include "Vocab_US_Large.h"
#include "Vocab_US_Clock.h"
#include "Vocab_US_Acorn.h"

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"



RF24 radio(8, 10); 
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

struct DataPacket {
  uint8_t angles[5];     // для 5 пальцев
  bool speechCommand;
};

DataPacket packet;

const int flexPins[5] = {A0, A1, A2, A3, A4};  // аналоговые пины
int minValues[5] = {0};
int maxValues[5] = {1023};

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setAutoAck(1);
  radio.setRetries(0, 15);
  radio.setPayloadSize(32);
  radio.openWritingPipe(address[0]);
  radio.setChannel(0x60);
  radio.setPALevel(RF24_PA_MAX);        // Максимальная мощность
  radio.setDataRate(RF24_250KBPS);
  radio.powerUp();
  radio.stopListening();

  calibrateSensors();   // калибровка при запуске
}

void calibrateSensors() {
  Serial.println("Калибровка датчиков... Держите пальцы выпрямленными 5 сек");
  delay(5000);
  
  for(int i = 0; i < 5; i++) {
    minValues[i] = analogRead(flexPins[i]);
  }
  
  Serial.println("Теперь максимально согните пальцы и держите 5 сек");
  delay(5000);
  
  for(int i = 0; i < 5; i++) {
    maxValues[i] = analogRead(flexPins[i]);
    if (maxValues[i] < minValues[i]) swap(maxValues[i], minValues[i]); // на всякий случай
  }
  Serial.println("Калибровка завершена!");
}

int readFlex(int pin, int minVal, int maxVal) {
  int val = analogRead(pin);
  val = constrain(val, minVal, maxVal);
  return map(val, minVal, maxVal, 0, 180);
}

void loop() {
  for(int i = 0; i < 5; i++) {   // пока используем 3, остальные можно оставить 90
    packet.angles[i] = (i < 3) ? readFlex(flexPins[i], minValues[i], maxValues[i]) : 90;
  }
  
  packet.speechCommand = false; // позже добавим кнопку

  bool ok = radio.write(&packet, sizeof(packet));
  
  if (ok) {
    Serial.print("Отправлено: ");
    for(int i=0; i<3; i++) Serial.print(packet.angles[i]), Serial.print(" ");
    Serial.println();
  } else {
    Serial.println("Ошибка отправки");
  }
  
  delay(80); // ~12 пакетов в секунду — оптимально
}