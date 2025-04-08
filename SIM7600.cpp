#include "sim7600.h"
#include <math.h>

HardwareSerial sim7600(1); // Use UART1 (TX2, RX2)
#define DEBUG true
#define MOVEMENT_THRESHOLD 8.0 
bool simInitialized = false;
bool firstFix = false;
bool currFix = false;
const float Rm = 6371000.0; //radius of earth in meters
const float Rk = 6371.0;
float prevLat, prevLon;
float lat, lon;
String userNumber;
String emerNumber;




//SEND FUNCTION
String sendCommand(String command, const int timeout, bool debug) {
  String response = "";
  sim7600.println(command);
  long int time = millis();

  while ((millis() - time) < timeout) {
    while (sim7600.available()) {
      response += char(sim7600.read());
    }
  }

  if (debug) {
    Serial.println("Command: " + command);
    Serial.println("Response: " + response);
  }

  response.trim();
  return response;
}
//end send function





//initialization
void initSIM(){

  //enable network connection
  if (checkSIMCard()) {
    Serial.println("SIM card is ready.");
  } else {
    Serial.println("SIM card not ready.");
    return;
  }

  configureAPN("RESELLER");

  setNetworkMode();

  if (checkNetworkRegistration()) {
    Serial.println("Network registration successful.");
  } else {
    Serial.println("Failed to register on the network.");
    return;
  }


  //enable GPS
  int attempts = 0;
  while (attempts < 2 && !simInitialized) {
    // Check if GPS is already enabled
    String gpsStatus = sendCommand("AT+CGPS?", 5000, true);
    if (gpsStatus.indexOf("+CGPS: 1") > -1) {
        simInitialized = true;
        Serial.println("SIM7600 GPS is already enabled.");
        break;  // GPS is already enabled, no need to enable again
    }
    // If not, enable it
    String gpsEnable = sendCommand("AT+CGPS=1,1", 5000, true);
    if (gpsEnable.indexOf("OK") > -1) {
        simInitialized = true;
        Serial.println("SIM7600 GPS Initialized Successfully");
        break;  // GPS enabled successfully
    }
    attempts++;
    Serial.println("Retrying SIM7600 GPS Initialization...");
    delay(2000);  // Retry delay
  }

  if (!simInitialized) {
    Serial.println("Failed to initialize SIM7600 GPS!");
  }

  String message1 = "SIM7600 is set up correctly";
  sendSMS("+14124809046", message1);

}





//SMS functions
// Check if SIM card is ready
bool checkSIMCard() {
  String response = sendCommand("AT+CPIN?", 5000, DEBUG);
  return response.indexOf("+CPIN: READY") > -1;
}

// Configure APN for the SIM card
void configureAPN(String apn) {
  String command = "AT+CGDCONT=1,\"IP\",\"" + apn + "\"";
  String response = sendCommand(command, 5000, DEBUG);
  if (response.indexOf("OK") > -1) {
    Serial.println("APN configured successfully.");
  } else {
    Serial.println("Failed to configure APN.");
  }
}

// Set the network mode to LTE
void setNetworkMode() {
  String response = sendCommand("AT+CNMP=38", 5000, DEBUG);  // LTE mode
  if (response.indexOf("OK") > -1) {
    Serial.println("Network mode set to LTE.");
  } else {
    Serial.println("Failed to set network mode.");
  }
}

// Check if registered on the network
bool checkNetworkRegistration() {
  String response = sendCommand("AT+CREG?", 5000, DEBUG);
  return response.indexOf("+CREG: 0,1") > -1 || response.indexOf("+CREG: 0,5") > -1;
}

// Test signal strength
void testSignalStrength() {
  String response = sendCommand("AT+CSQ", 5000, DEBUG);
  Serial.println("Signal Strength: " + response);
}

// Send SMS
void sendSMS(String phoneNumber, String message) {
  Serial.println("Sending SMS...");
  sendCommand("AT+CMGF=1", 5000, DEBUG);  // Set SMS mode to text
  sendCommand("AT+CMGS=\"" + phoneNumber + "\"", 5000, DEBUG);
  sim7600.print(message);  // Send message
  sim7600.write(26);       // Send CTRL+Z to indicate the end of the message
  Serial.println("SMS sent!");
}
//end SMS functions



//GPS functions
//fall detection GPS function
void fallGPS() {

  while (sim7600.available()) sim7600.read();

  String gpsInfo = sendCommand("AT+CGPSINFO", 5000, true);

  // Check if we got a response with CMGS (SMS sent confirmation)
  if (gpsInfo.indexOf("+CMGS:") != -1) {
    // If we see +CMGS, clear the buffer and send the command again
    Serial.println("Ignoring CMGS response...");
    gpsInfo = sendCommand("AT+CGPSINFO", 5000, true);  // Try again to get the GPS info
  }

  if (gpsInfo.indexOf(",,,,,,,") > -1) {
    Serial.println("GPS fix not yet available. Waiting for satellite lock...");
  } else {
    int startIndex = gpsInfo.indexOf(":") + 2;
    int comma1 = gpsInfo.indexOf(",", startIndex);
    int comma2 = gpsInfo.indexOf(",", comma1 + 1);
    int comma3 = gpsInfo.indexOf(",", comma2 + 1);
    int comma4 = gpsInfo.indexOf(",", comma3 + 1);

    if (comma1 > 0 && comma2 > 0 && comma3 > 0 && comma4 > 0) {
      String latStr = gpsInfo.substring(startIndex, comma1);
      String latDir = gpsInfo.substring(comma1 + 1, comma2);
      String lonStr = gpsInfo.substring(comma2 + 1, comma3);
      String lonDir = gpsInfo.substring(comma3 + 1, comma4);

      lat = convertToDecimal(latStr, true);
      lon = convertToDecimal(lonStr, false);

      if (latDir == "S") lat = -lat;
      if (lonDir == "W") lon = -lon;

      Serial.print("Latitude: ");
      Serial.println(lat, 6);
      Serial.print("Longitude: ");
      Serial.println(lon, 6);

      if (lat == 0.0 && lon == 0.0) {
       Serial.println("Invalid GPS coordinates. No fix obtained.");
        return;
      }

      
      if (!firstFix) {
        firstFix = true;
        Serial.println("First GPS fix acquired.");
        //sendSMS(userNumber,"first fix obtained, starting Blynk now");
      } 
      else {
        String message = "Fall detected! Location:\n";
        message += "Lat: " + String(lat, 6) + "\n";
        message += "Lon: " + String(lon, 6);
        //Serial.println(message);

        sendSMS(emerNumber, message);
      }
    }
    else{
      Serial.println("error getting coordinates. I am sorry");
    }
  }
}


//theft detection GPS function
void theftGPS() {

  while (sim7600.available()) sim7600.read();

  String gpsInfo = sendCommand("AT+CGPSINFO", 5000, DEBUG);

  // Check if we got a response with CMGS (SMS sent confirmation)
  if (gpsInfo.indexOf("+CMGS:") != -1) {
    // If we see +CMGS, clear the buffer and send the command again
    Serial.println("Ignoring CMGS response...");
    gpsInfo = sendCommand("AT+CGPSINFO", 5000, true);  // Try again to get the GPS info
  }

  if (gpsInfo.indexOf(",,,,,,,") > -1) {
    Serial.println("GPS fix not yet available. Waiting for satellite lock...");
  }

  int startIndex = gpsInfo.indexOf(":") + 2;
  int comma1 = gpsInfo.indexOf(",", startIndex);
  int comma2 = gpsInfo.indexOf(",", comma1 + 1);
  int comma3 = gpsInfo.indexOf(",", comma2 + 1);
  int comma4 = gpsInfo.indexOf(",", comma3 + 1);

  if (comma1 > 0 && comma2 > 0 && comma3 > 0 && comma4 > 0) {
    String latStr = gpsInfo.substring(startIndex, comma1);
    String latDir = gpsInfo.substring(comma1 + 1, comma2);
    String lonStr = gpsInfo.substring(comma2 + 1, comma3);
    String lonDir = gpsInfo.substring(comma3 + 1, comma4);

    // Convert correctly using isLatitude flag
    float lat = convertToDecimal(latStr, true);
    float lon = convertToDecimal(lonStr, false);

    if (latDir == "S") lat = -lat;
    if (lonDir == "W") lon = -lon;

    Serial.print("Latitude: ");
    Serial.println(lat, 6);
    Serial.print("Longitude: ");
    Serial.println(lon, 6);

    if (lat == 0.0 && lon == 0.0) {
      Serial.println("Invalid GPS coordinates. No fix obtained.");
      return;
    }


    if (!currFix) {
      currFix = true;
      prevLat = lat;
      prevLon = lon;
      Serial.println("tracking distance from current coordinate now");
    } 
    else {
      float distanceMoved = haversine(prevLat, prevLon, lat, lon, 0);
      Serial.print("Distance moved: ");
      Serial.print(distanceMoved);
      Serial.println(" meters");

      if (distanceMoved > MOVEMENT_THRESHOLD) {  
        Serial.println("Movement detected! Sending SMS...");

        String message = "Bike Moved! Location:\n";
        message += "Lat: " + String(lat, 6) + "\n";
        message += "Lon: " + String(lon, 6);
        //Serial.println(message);

        sendSMS(userNumber, message);
      }
      else{
        Serial.println("bike has not moved");
      }
    }
  }
  else{
    Serial.println("error getting coordinates. I am sorry");
  }
}




float convertToDecimal(String coord, bool isLatitude) {
  int degLength = isLatitude ? 2 : 3;
  float degrees = coord.substring(0, degLength).toFloat();
  float minutes = coord.substring(degLength).toFloat();

  return degrees + (minutes / 60.0);
}

float degreesToRadians(float degrees) {
  return degrees * (M_PI / 180.0);
}


// 1 for kilometers or 0 for meters in measure variable
float haversine(float lat1, float lon1, float lat2, float lon2, int measure) {
  lat1 = degreesToRadians(lat1);
  lon1 = degreesToRadians(lon1);
  lat2 = degreesToRadians(lat2);
  lon2 = degreesToRadians(lon2);

  float dlat = lat2 - lat1;
  float dlon = lon2 - lon1;
  float a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));

  if (measure) {
	  return Rk * c;
  }
  else {
	  return Rm * c;
  }
}


//end GPS functions
