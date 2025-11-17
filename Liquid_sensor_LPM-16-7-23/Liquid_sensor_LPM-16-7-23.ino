#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>

const int buzzer = 8;      // PB0
const int led    = 9;      // PB1
const int sensorPin = 10;  // PB2 (PCINT2)

volatile bool sensorChanged = false;
volatile bool wdtFired = false;
bool hasFlashedOnHigh = false;

// --- Disable unused peripherals for ultra-low power ---
void disableUnusedPeripherals() {
  ADCSRA &= ~(1 << ADEN);
  ACSR  |= (1 << ACD);
  power_adc_disable();
  power_twi_disable();
  power_spi_disable();
  power_usart0_disable();
  power_timer1_disable();
  power_timer2_disable();
  // keep Timer0 for delay(); comment next line to remove delay()
  // power_timer0_disable();
  DIDR0 = 0x3F;        // Disable ADC0-ADC5
  DIDR1 = (1 << AIN0D) | (1 << AIN1D); // Disable analog comparator pins
}

// --- Flash LED/Buzzer helper ---
void flashOnce(bool beep) {
  digitalWrite(led, HIGH);
  if (beep) digitalWrite(buzzer, HIGH);
  // 200 ms ON → wait using WDT-based sleep
  delay(200);
  digitalWrite(led, LOW);
  if (beep) digitalWrite(buzzer, LOW);
  delay(200);
}

// --- 3-flash alert for HIGH state ---
void doHighStateAction() {
  if (!hasFlashedOnHigh) {
    for (int i = 0; i < 5; i++) {
      flashOnce(false);
    }
    hasFlashedOnHigh = true;
  }
}

// --- Continuous alert for LOW state using WDT ---
void doLowStateAction() {
  hasFlashedOnHigh = false;
  flashOnce(true);
}

// --- Pin-change interrupt for sensorPin (PB2 / D10) ---
ISR(PCINT0_vect) {
  sensorChanged = true;
}

// --- Watchdog interrupt for low-power blinking ---
ISR(WDT_vect) {
  wdtFired = true;
}

// --- Enter deep sleep (Power-Down) ---
void sleepNow() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_bod_disable();
  sei();
  sleep_cpu();
  sleep_disable();
}

// --- Configure Watchdog for ~250 ms wake-up ---
void setupWatchdog() {
  cli();
  wdt_reset();
  WDTCSR |= (1 << WDCE) | (1 << WDE); // enable config mode
  WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP0); // 250 ms, interrupt mode
  sei();
}

void setup() {
  pinMode(buzzer, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(sensorPin, INPUT);

  delay(3000);             // initial delay
  disableUnusedPeripherals();

  // Enable pin-change interrupt on PB2 / D10
  PCMSK0 |= (1 << PCINT2);
  PCICR  |= (1 << PCIE0);

  // Start WDT for low-power fault blinking
  setupWatchdog();

  // Initial check on power-up
  int state = digitalRead(sensorPin);
  if (state == HIGH) {
    doHighStateAction();
  } else {
    // Fault → use WDT sleep loop
    while (digitalRead(sensorPin) == LOW) {
      wdtFired = false;
      sleepNow();
      if (wdtFired) doLowStateAction();
    }
  }
}

void loop() {
  int state = digitalRead(sensorPin);

  if (state == HIGH) {
    // Normal → sleep until pin change
    sleepNow();
    if (sensorChanged) {
      sensorChanged = false;
      doHighStateAction();
    }
  } else {
    // Fault → low-power WDT blinking
    while (digitalRead(sensorPin) == LOW) {
      wdtFired = false;
      sleepNow();
      if (wdtFired) doLowStateAction();
    }
  }
}
