#include "MPU6500.h"

// Raw data variables
float ax,ay,az;
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ, Tmp;
int32_t offsetAcX = 0, offsetAcY = 0, offsetAcZ = 0;
#define NUM_SAMPLES 500

void initMPU() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // Set to zero (wakes up the MPU-6500)
  Wire.endTransmission(true);
  calibrateMPU();    // Calibrate the sensor
}

void readMPUData() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // Starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);  // Request a total of 14 registers
  AcX = Wire.read() << 8 | Wire.read();  // ACCEL_XOUT_H & ACCEL_XOUT_L
  AcY = Wire.read() << 8 | Wire.read();  // ACCEL_YOUT_H & ACCEL_YOUT_L
  AcZ = Wire.read() << 8 | Wire.read();  // ACCEL_ZOUT_H & ACCEL_ZOUT_L
  Tmp = Wire.read() << 8 | Wire.read();  // TEMP_OUT_H & TEMP_OUT_L
  GyX = Wire.read() << 8 | Wire.read();  // GYRO_XOUT_H & GYRO_XOUT_L
  GyY = Wire.read() << 8 | Wire.read();  // GYRO_YOUT_H & GYRO_YOUT_L
  GyZ = Wire.read() << 8 | Wire.read();  // GYRO_ZOUT_H & GYRO_ZOUT_L
}

void calibrateMPU() {
  setMPUOffsets();
  int32_t sumAcX = 0, sumAcY = 0, sumAcZ = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    readMPUData();

    sumAcX += AcX;
    sumAcY += AcY;
    sumAcZ += AcZ;

    delay(10);  // Short delay between samples
  }

  offsetAcX = sumAcX / NUM_SAMPLES;
  offsetAcY = sumAcY / NUM_SAMPLES;
  offsetAcZ = (sumAcZ / NUM_SAMPLES) - 16384;  // Account for gravity on Z-axis
}

void setMPUOffsets() {
  // Function to set the offsets manually if needed
  offsetAcX = 0;
  offsetAcY = 0;
  offsetAcZ = 0;
}

void mpuCalc(){
  ax = ((AcX - offsetAcX) / 16384.00); //meters per second squared
  ay = ((AcY - offsetAcY) / 16384.00); 
  az = ((AcZ - offsetAcZ) / 16384.00); 
}
