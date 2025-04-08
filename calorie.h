#ifndef CALORIE_H
#define CALORIE_H

#include <PulseSensorPlayground.h>

extern PulseSensorPlayground pulseSensor;  

// External variable declarations
extern const int PulseWire;
extern int Threshold;

extern int weight;
extern int age;
extern bool gender;

extern int stableBPM;
extern float totalCaloriesBurned;
extern unsigned long lastUpdateTime;
extern const unsigned long updateInterval;

// Function prototypes
float calculateCalories(int bpm, int weight, int age, bool gender, float timeMinutes);
void updateCalories();

#endif
