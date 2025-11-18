#include <Arduino.h>
#include <avr/wdt.h>

const byte WATCHDOG_INPUT = 2;        // PB2 - Signal de l'ESP32
const byte RESET_PIN = 1;             // PB1 - Contrôle le RESET de l'ESP32
const unsigned long TIMEOUT = 4000;   // 4 secondes sans signal = reset
const unsigned long RESET_DURATION = 1000; // Durée du pulse de reset (500ms)

unsigned long lastKick = 0;
unsigned long resetStartTime = 0;
bool resetting = false;


static inline void wdt_init(uint8_t t) {
  uint8_t s = SREG; cli();
  MCUSR &= ~(1<<WDRF);
  WDTCR |= (1<<WDCE)|(1<<WDE);
  WDTCR = (1<<WDE) | t;
  SREG = s;
}

void setup() {
  pinMode(WATCHDOG_INPUT, INPUT_PULLUP);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);  // HIGH = fonctionnement normal
  
  lastKick = millis();  // Initialise le timer au démarrage
  wdt_init(8);          // 1 s, mode RESET pour le wdg interne
}

void loop() {
  wdt_reset();// reveil le wd interne du tiny


  unsigned long currentTime = millis();

  if (resetting && (currentTime - resetStartTime > 10000)) {
  digitalWrite(RESET_PIN, HIGH);
  resetting = false;
}

  // Détection du signal "kick" de l'ESP32
  if (digitalRead(WATCHDOG_INPUT) == LOW) {
    lastKick = currentTime;
    
    // Si on était en train de reset, on annule
    if (resetting) {
      resetting = false;
      digitalWrite(RESET_PIN, HIGH);
    }
  }
  
  // Gestion du reset
  if (resetting) {
    // Fin du pulse de reset
    if (currentTime - resetStartTime >= RESET_DURATION) {
      digitalWrite(RESET_PIN, HIGH);  // Relâche le reset
      resetting = false;
      lastKick = currentTime;  // Réinitialise le timer
    }
  } else {
    // Vérification du timeout
    if (currentTime - lastKick > TIMEOUT) {
      // Déclenche le reset de l'ESP32
      digitalWrite(RESET_PIN, LOW);  // Active le reset (LOW)
      resetStartTime = currentTime;
      resetting = true;
    }
  }
}