//need to replace
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""


#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
BlynkTimer timer;
int calUpdateTimer_id;


//my libs
#include "task.h"
#include "tpms.h"
#include "SIM7600.h"
#include "fingerprint.h"
#include "MPU6500.h"
#include "calorie.h"


//tasks
TaskHandle_t Task_FD;
TaskHandle_t Task_Calorie;
TaskHandle_t Task_TD;
TaskHandle_t Task_Fingerprint;
TaskHandle_t Task_Wire;
TaskHandle_t Task_TPMS;


//mode switching
enum Mode {
  FD,
  TD,
  // Add more modes here as you expand
  MODE_COUNT // Keeps track of the number of modes
};
Mode currentMode = TD;
bool previousButtonState = false;  // Flag to track the previous state of the button
//end mode switching



//heart rate sensor
PulseSensorPlayground pulseSensor;  //this needs to stay



//TPMS
//BLE VARAIBLES
BLEScan* pBLEScan;
BLEClient*  pClient;
static BLEAddress *pServerAddress;
String knownAddresses[] = { "80:ea:ca:13:09:d6" , "81:ea:ca:23:0a:e7"};  
String tirePositions[] = { "Front Tire", "Rear Tire" };
int desiredPressure = 20; // -> V12
// Front Tire -> V2, Rear Tire -> V7

//BLE class for TPMS
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  public:
    void onResult(BLEAdvertisedDevice Device) override {
        pServerAddress = new BLEAddress(Device.getAddress());
        String ManufData = Device.toString().c_str();
        
        for (int i = 0; i < 2; i++) {
            if (strcmp(pServerAddress->toString().c_str(), knownAddresses[i].c_str()) == 0){
                Serial.println("Device found: " + tirePositions[i]);

                String inString = retmanData(ManufData, 0);

                // Pressure conversion
                long int kPa = returnData(inString, 8) / 1000.0;
                long int psi = (kPa * 1000) / 6895;
                Serial.print("Pressure: ");
                Serial.print(psi);
                Serial.println(" PSI");

                // Send pressure data to Blynk
                int whichPin = (i == 0) ? V2 : V7;
                Blynk.virtualWrite(whichPin, psi);

                if(psi < desiredPressure){
                  Blynk.logEvent("tire_pressure");
                  Serial.print("ALARM, your ");
                  if(i==0){
                    Serial.println("FRONT tire is low");
                  }
                  else{
                    Serial.println("BACK tire is low");
                  }
                }

                // Battery percentage
                Serial.print("Battery: ");
                Serial.print(returnBatt(inString));
                Serial.println("%");

                Serial.println("");
            }
        }
    }
};

// Task running on Core 1
void TaskTPMS(void *pvParameters) {
  while (1) {
    // Start the scan and retrieve results
    pBLEScan->start(3);  // Perform a scan for 5 seconds
    pBLEScan->clearResults();  // Optionally clear results to free memory
    //Serial.println(uxTaskGetStackHighWaterMark(NULL));

    // Delay to prevent overwhelming the core
    vTaskDelay(pdMS_TO_TICKS(17000));
  }
}





//BLYNK FUNCTIONS

// Function to handle mode changes from Blynk Button 
BLYNK_WRITE(V0) {
  int value = param.asInt();  // Get the button value (1 for ON, 0 for OFF)
  
  // Toggle the mode based on button state
  if (value == 1 && !previousButtonState) {
    currentMode = static_cast<Mode>((currentMode + 1) % MODE_COUNT); // Switch mode
    Serial.print("Mode changed to: ");
    
    // Display mode change in the serial monitor
    switch (currentMode) {
      case FD:
        Serial.println("Fall Detection");
        Blynk.virtualWrite(V1, "Fall Detection");  // Update the mode in the app
        vTaskSuspend(Task_TD);  // Stop Theft Detection
        vTaskSuspend(Task_Fingerprint); //Stop fingerprint detection
        vTaskSuspend(Task_Wire);  // Stop Wire Cut Detection
        vTaskResume(Task_FD);    // Start Fall Detection
        vTaskResume(Task_Calorie);    // Start calorie Detection
        timer.enable(calUpdateTimer_id); //start calorie update timer
        break;
      case TD:
        Serial.println("Theft Detection");
        Blynk.virtualWrite(V1, "Theft Detection");  // Update the mode in the app
        vTaskSuspend(Task_FD);  // Stop Fall Detection
        vTaskSuspend(Task_Calorie);  // Stop Calorie Detection
        timer.disable(calUpdateTimer_id); // Stop calorie update timer
        vTaskResume(Task_TD);    // Start Theft Detection
        vTaskResume(Task_Fingerprint);    // Start fingerprint Detection
        vTaskResume(Task_Wire); //Start Wire Cut detection
        currFix = false; //force the theftGPS function to update prevLon and prevLat
        break;
    }
  }
  previousButtonState = value;
}


BLYNK_WRITE(V3) {
  int value = param.asInt();
  // Toggle gender whenever the button is pressed

  if(value == 1){
    gender = !gender;  // This will toggle between true (Male) and false (Female)

    if (gender) { // Print the current gender state to Blynk
      Blynk.virtualWrite(V8,"Male");
    } else {
      Blynk.virtualWrite(V8,"Female");
    }
  }
}

BLYNK_WRITE(V4) {
  // Age input, use a Numeric Input widget
  age = param.asInt();  // Read age value
  Serial.print("Age: ");  // Print the label "Age: "
  Serial.println(age); 
}

BLYNK_WRITE(V5) {
  // Weight input, use a Numeric Input widget
  weight = param.asInt();  // Read weight value
  Serial.print("Weight: ");  // Print the label "Age: "
  Serial.println(weight); 
}

//Calorie Reset button
BLYNK_WRITE(V9) {
  int buttonState = param.asInt(); // Read button state (1 = pressed, 0 = released)
  if (buttonState == 1) { 
    totalCaloriesBurned = 0.0;  // Reset the distance to zero
    Blynk.virtualWrite(V6, totalCaloriesBurned); // Update UI
    Serial.println("Total distance reset to zero.");
  }
}

// Handle phone number input
BLYNK_WRITE(V10) {
  userNumber = param.asStr();

  // Ensure the input is a valid phone number
  if (userNumber.length() > 9) {
    Serial.print("Received User Phone Number: ");
    Serial.println(userNumber);
  } else {
    Serial.println("Invalid phone number received, ignoring.");
  }
}


// Handle phone number input
BLYNK_WRITE(V11) {
  emerNumber = param.asStr();

  // Ensure the input is a valid phone number
  if (emerNumber.length() > 9) {
    Serial.print("Received Emergency Phone Number: ");
    Serial.println(emerNumber);
  } else {
    Serial.println("Invalid phone number received, ignoring.");
  }
}

BLYNK_WRITE(V12){
  int p1 = param.asInt(); //read desired Tire pressure
  desiredPressure = p1;
  Serial.print("pressure set to: ");
  Serial.println(desiredPressure);
}


// This function is called every time the device is connected to the Blynk.Cloud
//BLYNK.syncVitual() will cause the selected pin's BLYNK_WRITE() function to execute
BLYNK_CONNECTED()
{
  Serial.println("Connected to Blynk. Syncing...");
  Blynk.syncVirtual(V3); //gender
  Blynk.syncVirtual(V4); //age
  Blynk.syncVirtual(V5); //weight
  Blynk.syncVirtual(V10); //user number
  Blynk.syncVirtual(V11); //emergency number
  Blynk.syncVirtual(V12); //desired tire pressure
}

//function called when Blynk timer runs
void calUpdate() {
  //write totalCalories to blynk every x amount of seconds
  Blynk.virtualWrite(V6, totalCaloriesBurned); // Save value to Blynk
}

//END BLYNK FUNCTIONS




void setup() {
  Serial.begin(115200);
  delay(100); //can be adjusted for connection time of network on SIM7600

  sim7600.begin(115200, SERIAL_8N1, 16, 17); // SIM7600 TX=17, RX=16
  delay(1000); // adjust delay to make sure it doesnt start before green light is flashing (10 secondsish)
  initSIM(); //initialize the SIM7600 (lengthy function);

  //obtain first fix
  while (!firstFix) {
    fallGPS();
    delay(3000);
  }
  

  //TPMS sensor
  BLEDevice::init("");
  pClient  = BLEDevice::createClient();
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);


  //initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  //Blynk timer calorie updates
  calUpdateTimer_id = timer.setInterval(20000L, calUpdate); // Store timer ID


  //heartrate sensor
  pulseSensor.analogInput(PulseWire);   
  pulseSensor.setThreshold(Threshold);
  if (pulseSensor.begin()) {
    Serial.println("Pulse Sensor Initialized!");
  }


  //MPU6500
  //initialize and calibrate
  initMPU();

  //fingerprint
  setupFingerprint();

  //relay pins
  //pinMode(32,OUTPUT); //LED relay
  pinMode(18,OUTPUT); //fingerprint relay
  pinMode(23,OUTPUT); //wire cut relay pin


  //Cable Cut detection pins
  pinMode(25, OUTPUT); //wire write pin
  pinMode(26, INPUT_PULLDOWN); //wire read pin
  digitalWrite(25, HIGH);


  // third to last = priority, last = core number 0 or 1
  xTaskCreatePinnedToCore(TaskTPMS, "TPMS read and update", 2048, NULL, 2, &Task_TPMS, 1); //high priority (these are high priority so they are not missed)
  xTaskCreatePinnedToCore(TaskCalorie, "update calories", 2048, NULL, 2, &Task_Calorie, 1); //high priority
  //xTaskCreatePinnedToCore(TaskMaintain, "update maintenance", 2048, NULL, 2, &Task_Maintain, 1); //high priority

  xTaskCreatePinnedToCore(TaskFingerprint, "scan finger", 2048, NULL, 1, &Task_Fingerprint, 1); //lower priority
  xTaskCreatePinnedToCore(TaskWire, "Wire Cut Detection", 2048, NULL, 1, &Task_Wire, 1); // lower priority
  xTaskCreatePinnedToCore(TaskFD, "Fall Detection", 2048, NULL, 1, &Task_FD, 1); // lower priority
  xTaskCreatePinnedToCore(TaskTD, "Theft Detection", 2048, NULL, 1, &Task_TD, 1); // lower priority

}

void loop() {
  Blynk.run();
  timer.run();
}
