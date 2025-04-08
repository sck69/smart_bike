#include "calorie.h"

// Global variable definitions
const int PulseWire = 36;
int Threshold = 550;

bool gender = false; //male
int weight = 0;
int age = 0;

int stableBPM = 0;
float totalCaloriesBurned = 0.0;
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 5000; // Update every 5 seconds

const float myConstant = .8;


// Calculate Calories Burned
float calculateCalories(int bpm, int weight, int age, bool gender, float timeMinutes) {
  if (bpm < 40 || bpm > 220) return 0; // Ignore unrealistic BPM values

  float caloriesPerMin;
  if (gender) {
    caloriesPerMin = (-55.0969 + (0.6309 * bpm) + (0.1988 * weight) + (0.2017 * age)) / 4.184; //female
  } else {
    caloriesPerMin = (-20.4022 + (0.4472 * bpm) - (0.1263 * weight) + (0.074 * age)) / 4.184; //male
  }

  if (caloriesPerMin < 0) caloriesPerMin = 0; // Ensure non-negative calorie count

  //Serial.print("Calories per minute: ");
  //Serial.println(caloriesPerMin);

  return caloriesPerMin * timeMinutes; // Multiply by time in minutes
}

void updateCalories(){
  float elapsedTime = (millis() - lastUpdateTime) / 60000.0; // Convert to minutes
  lastUpdateTime = millis(); // Reset update time

  float caloriesBurned = calculateCalories(stableBPM, weight, age, gender, elapsedTime);
  totalCaloriesBurned += myConstant * caloriesBurned;

  Serial.print("Calories Burned (last 5 sec): ");
  Serial.println(caloriesBurned);
  Serial.print("Total Calories Burned: ");
  Serial.println(totalCaloriesBurned);
}
