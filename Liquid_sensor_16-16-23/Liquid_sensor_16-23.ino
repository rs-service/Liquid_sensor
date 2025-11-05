// Buzzer control based on sensor input on ATmega328P (Arduino Uno/Nano)

const int buzzer = 8;      // buzzer connected to pin 8
const int led = 9;      // led connected to pin 9
const int sensorPin = 10;  // Sensor connected to digital pin 10

bool hasflashedOnHigh = false;  // To track if 3-flash alert was done

void setup() {
  pinMode(buzzer, OUTPUT);     // Set buzzer pin as output
  pinMode(led, OUTPUT);     // Set led pin as output
  pinMode(sensorPin, INPUT);   // Set sensor pin as input

  delay(2000);  // Wait 2 seconds before starting main code
}

void loop() {
  int sensorState = digitalRead(sensorPin);  // Read sensor value

  if (sensorState == HIGH) {
    // Do 3 flashes only once when sensor goes HIGH
    if (!hasflashedOnHigh) {
      for (int i = 0; i < 3; i++) {
        digitalWrite(led, HIGH);
        delay(500);
        digitalWrite(led, LOW);
        delay(500);
      }
      
      hasflashedOnHigh = true;
    }
  } else {
    hasflashedOnHigh = false; // Reset when sensor goes LOW again

    // Continuous flash pattern while sensor is LOW
    digitalWrite(led, HIGH);
    digitalWrite(buzzer, HIGH);
    delay(2000);
    digitalWrite(led, LOW);
    digitalWrite(buzzer, LOW);    
    delay(2000);
  }
}
