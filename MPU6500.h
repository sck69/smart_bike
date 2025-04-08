#ifndef MPU6500_H
#define MPU6500_H

#include <Wire.h>

// MPU6500 I2C address
#define MPU_ADDR 0x68

// Function declarations
void initMPU();
void readMPUData();
void calibrateMPU();
void setMPUOffsets();
void mpuCalc();

extern int16_t AcX, AcY, AcZ, GyX, GyY, GyZ, Tmp;
extern int32_t offsetAcX, offsetAcY, offsetAcZ;
extern float ax, ay, az;

#endif
