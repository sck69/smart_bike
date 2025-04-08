# smart_bike
I want the world to have this. This is my current senior project code, maybe ill add this code later when i am officially done.
Parts needed: ESP32 wroom 30 pin, MPU6500 (21 22), SIM7600 (16 17), adafruit fingerprint sensor (4 5), arduino heart rate sensor (36), and a wire (25 26)(if bike lock cable is cut or removed)
you will also need relays and leds
This code contains two modes: fall detection, theft detection and a way to switch between the two.
By using the freeRTOS to manage tasks the mode switching algorithm was created.
This code utilizes Blynk to create a UI for the user and switch between modes.
This code also uses Blynk timers to update various data on the UI.

