#ifndef SIM7600_H
#define SIM7600_H

#include <HardwareSerial.h>

extern HardwareSerial sim7600; // Declare HardwareSerial instance
extern bool firstFix; // Flag for first GPS fix
extern bool currFix;
extern float prevLat, prevLon; // Global variables
extern bool simInitialized;
extern String userNumber;
extern String emerNumber;

// Function prototypes
String sendCommand(String command, const int timeout, bool debug = false);
void initSIM();
bool checkSIMCard();
void configureAPN(String apn);
void setNetworkMode();
bool checkNetworkRegistration();
void testSignalStrength();
void sendSMS(String phoneNumber, String message);
void fallGPS();
void theftGPS();
float convertToDecimal(String coord, bool isLatitude);
float degreesToRadians(float degrees); 
float haversine(float lat1, float lon1, float lat2, float lon2, int measure);

#endif // SIM7600_H