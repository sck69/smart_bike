#include "MPU6500.h"
#include "SIM7600.h"
#include "calorie.h"
#include "fingerprint.h"
#include <Arduino.h>


// Raw data variables
boolean fall = false;       // Stores if a fall has occurred
boolean trigger1 = false;   // Stores if the first trigger (lower threshold) has occurred
boolean trigger2 = false;   // Stores if the second trigger (upper threshold) has occurred
unsigned long trigger1Time = 0;
#define highThreshold 18     // High amplitude threshold (e.g., 3g)
#define lowThreshold 11        // Low amplitude threshold (e.g., 0.5g)
#define impactDelay 500       // Max time (ms) between high and low amplitude
bool theftDetect = false;




//fall detection
void resetTriggers() {
    trigger1 = false;
    trigger2 = false;
    fall = false;
}

// Task running on Core 0
void TaskFD(void *pvParameters) {
    while (1) {
        // Fall detection code
        readMPUData();
        mpuCalc();

        // Calculate amplitude and angle change
        float raw_amplitude = pow(pow(ax, 2) + pow(ay, 2) + pow(az, 2), 0.5);
        int amplitude = raw_amplitude * 10;  // Multiplied by 10 because values are between 0 to 1
        Serial.print("acc amplitude:");
        Serial.println(amplitude);

        unsigned long currentTime = millis();

        if (amplitude >= highThreshold && !trigger1) {
            trigger1 = true;
            trigger1Time = currentTime;
            Serial.println("Trigger 1: High amplitude detected");
        }

        // Check for sudden stop after high amplitude
        if (trigger1 && amplitude < lowThreshold && (currentTime - trigger1Time) <= impactDelay) {
            trigger2 = true;
            Serial.println("Trigger 2: Sudden stop detected");
        }

        if (trigger1 && trigger2 && (AcZ < 5000)) {
            fall = true;
            Serial.println("Fall detected!");
            fallGPS();
            resetTriggers();  // Reset all triggers
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Short delay to prevent excessive CPU usage
    }
}




//theft detecttion
void TaskTD(void *pvParameters) {
    while (1) {
        // Fall detection code
        readMPUData();
        mpuCalc();

        // Calculate amplitude and angle change
        float raw_amplitude = pow(pow(ax, 2) + pow(ay, 2) + pow(az, 2), 0.5);
        int amplitude = raw_amplitude * 10;  // Multiplied by 10 because values are between 0 to 1
        Serial.print("acc amplitude:");
        Serial.println(amplitude);

        if(amplitude >= 11 && !theftDetect){
          theftDetect = true;
          Serial.println("Theft detected!");
        }

        if(theftDetect){
          Serial.println("Movement detected! Attempting to obtain GPS fix...");
          theftGPS();
          /*flash LEDS
          digitalWrite(34,HIGH);
          delay(1000);
          digitalWrite(34,LOW);
          delay(1000);
          digitalWrite(34,HIGH);
          delay(1000);
          digitalWrite(34,LOW);
          */
          theftDetect = false; // Reset theft detection flag after sending SMS
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Short delay to prevent excessive CPU usage
    }
}




//calorie detection
// Task running on Core 1
void TaskCalorie(void *pvParameters) {
  while (1) {
    if (pulseSensor.sawStartOfBeat()) {    
      int currentBPM = pulseSensor.getBeatsPerMinute();
    
      if (currentBPM > 40 && currentBPM < 220) { // Ensure BPM is within a reasonable range
        stableBPM = currentBPM;
        Serial.println("♥ A HeartBeat Happened!");
        Serial.print("BPM: ");
        Serial.println(stableBPM);
      }
    }


    // Call the calorie update function every 8 seconds
    if (millis() - lastUpdateTime >= updateInterval) {
      updateCalories();
    }

    // Delay to prevent overwhelming the core
    vTaskDelay(pdMS_TO_TICKS(500)); 
  }
}




//fingerprint function
void TaskFingerprint(void *pvParameters) {
    while (1) {
      uint8_t result = getFingerprintID();
      if (result == FINGERPRINT_OK) {
        finger.LEDcontrol(FINGERPRINT_LED_ON, 0, 4); // turn fingerprint green
        digitalWrite(18,HIGH);
        vTaskDelay(pdMS_TO_TICKS(2000));
        digitalWrite(18,LOW);
        finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, 0);
      } else if (result == FINGERPRINT_NOTFOUND) {
        finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED); //turn fingerprint red
        vTaskDelay(pdMS_TO_TICKS(2000));
        finger.LEDcontrol(FINGERPRINT_LED_OFF, 0, 0);
      }

      vTaskDelay(pdMS_TO_TICKS(250));
    }
}







//Wire Cut detection Task
void TaskWire(void *pvParameters){
  while (1) {
    int wireStatus = digitalRead(26);
    
    if (wireStatus == HIGH) {
        digitalWrite(23,LOW);
    } else {
        //Serial.println("wire disconnected");
        digitalWrite(23,HIGH);
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

