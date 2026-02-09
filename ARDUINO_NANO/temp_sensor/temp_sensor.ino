/*
 * Arduino Nano - DS18B20 Temperature Sensor Reader
 * Sends temperature data to Raspberry Pi via USB Serial
 * 
 * Hardware:
 * - Arduino Nano (ATmega328P)
 * - DS18B20 Temperature Sensor
 * - 4.7kΩ Resistor (pull-up)
 * - USB cable to Raspberry Pi
 * 
 * Connections:
 * DS18B20 Pin 1 (GND)   → Arduino GND
 * DS18B20 Pin 2 (DATA)  → Arduino D2 (with 4.7kΩ pull-up to 5V)
 * DS18B20 Pin 3 (VCC)   → Arduino 5V
 * 4.7kΩ Resistor        → Between DATA and VCC
 * 
 * Required Libraries:
 * - OneWire by Paul Stoffregen
 * - DallasTemperature by Miles Burton
 * 
 * Install via: Tools → Manage Libraries → Search for library names
 */

#include <OneWire.h>
#include <DallasTemperature.h>

// ==================== CONFIGURATION ====================
#define DS18B20_PIN 2           // GPIO pin connected to DS18B20 data line
#define CABIN_ID 0              // Which cabin this sensor monitors (0-9)
#define READ_INTERVAL 5000      // Read temperature every 5 seconds (milliseconds)
#define SEND_INTERVAL 10000     // Send update to Pi every 10 seconds (milliseconds)
#define HIGH_TEMP_THRESHOLD 24  // TESTING: Lowered to 24°C for easy testing (normal: 45)
#define HIGH_TEMP_ALERT_INTERVAL 3000  // Alert every 3 seconds when over threshold (milliseconds)

#define LED_PIN 13              // Built-in LED on Arduino Nano
#define SERIAL_BAUD 115200      // Serial communication baud rate

// ==================== GLOBAL VARIABLES ====================
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

unsigned long lastReadTime = 0;
unsigned long lastSendTime = 0;
unsigned long lastHighTempAlertTime = 0;
int readCount = 0;
float lastTemp = -999.0;  // Invalid initial value
bool inHighTempState = false;
DeviceAddress sensorAddress;

// ==================== FUNCTION DECLARATIONS ====================
void blinkLED(int times, int delayMs = 100);
float readTemperature();
void sendToRaspberryPi(int cabinId, float temperature);
void sendHighTempEmergency(int cabinId, float temperature);

// ==================== SETUP ====================
void setup() {
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD);
  
  // Small delay to allow serial to initialize
  delay(1000);
  
  // Startup message
  Serial.println();
  Serial.println("==================================================");
  Serial.println("DS18B20 Temperature Monitor - Coach RTOS");
  Serial.println("Arduino Nano Edition");
  Serial.println("==================================================");
  Serial.print("Monitoring Cabin: ");
  Serial.println(CABIN_ID);
  Serial.print("High Temp Threshold: ");
  Serial.print(HIGH_TEMP_THRESHOLD);
  Serial.println("°C");
  Serial.println("==================================================");
  
  // Initialize DS18B20 sensor
  sensors.begin();
  
  // Check if sensor is connected
  int deviceCount = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" DS18B20 sensor(s)");
  
  if (deviceCount == 0) {
    Serial.println("ERROR: No DS18B20 sensors found!");
    Serial.println("Check wiring and 4.7kΩ pull-up resistor");
    // Keep LED on to indicate error
    digitalWrite(LED_PIN, HIGH);
    while (1) {
      delay(1000);  // Halt execution
    }
  }
  
  // Get address of first sensor
  if (sensors.getAddress(sensorAddress, 0)) {
    Serial.print("Sensor Address: ");
    for (uint8_t i = 0; i < 8; i++) {
      if (sensorAddress[i] < 16) Serial.print("0");
      Serial.print(sensorAddress[i], HEX);
      if (i < 7) Serial.print(":");
    }
    Serial.println();
  }
  
  // Set resolution (9-12 bits, higher = more accurate but slower)
  sensors.setResolution(sensorAddress, 12);
  
  Serial.println("Starting temperature monitoring...");
  Serial.println();
  
  // Blink 3 times to indicate successful startup
  blinkLED(3, 200);
}

// ==================== MAIN LOOP ====================
void loop() {
  unsigned long currentTime = millis();
  
  // Read temperature at specified interval
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;
    
    float temp = readTemperature();
    
    if (temp != -127.0) {  // -127 indicates sensor error
      readCount++;
      
      // Check for high temperature emergency
      bool isHighTemp = (temp > HIGH_TEMP_THRESHOLD);
      
      // Show reading on console (for debugging)
      Serial.print("[Reading ");
      Serial.print(readCount);
      Serial.print("] ");
      if (isHighTemp) {
        Serial.print("⚠️ HIGH TEMP! ");
      }
      Serial.print("Cabin ");
      Serial.print(CABIN_ID);
      Serial.print(": ");
      Serial.print(temp, 2);
      Serial.println("°C");
      
      // Blink once on successful read
      blinkLED(1, 50);
      
      // Handle high temperature emergency
      if (isHighTemp) {
        // Send emergency alert at faster rate when temperature is high
        if ((currentTime - lastHighTempAlertTime >= HIGH_TEMP_ALERT_INTERVAL) || !inHighTempState) {
          sendHighTempEmergency(CABIN_ID, temp);
          inHighTempState = true;
          lastHighTempAlertTime = currentTime;
          // Rapid blink for emergency
          blinkLED(5, 50);
        }
      } else {
        // Clear emergency state when temperature drops
        if (inHighTempState) {
          Serial.print("Temperature returned to normal: ");
          Serial.print(temp, 2);
          Serial.println("°C");
          inHighTempState = false;
        }
        lastHighTempAlertTime = 0;
      }
      
      // Send normal temperature update
      bool tempChanged = (lastTemp == -999.0) || (abs(temp - lastTemp) >= 1.0);
      bool sendIntervalReached = (currentTime - lastSendTime >= SEND_INTERVAL);
      
      if (sendIntervalReached || tempChanged) {
        sendToRaspberryPi(CABIN_ID, temp);
        lastTemp = temp;
        lastSendTime = currentTime;
        // Double blink when sending (if not in emergency)
        if (!isHighTemp) {
          blinkLED(2, 50);
        }
      }
    } else {
      Serial.println("Failed to read temperature - sensor may be disconnected");
      // Keep LED on to indicate error
      digitalWrite(LED_PIN, HIGH);
    }
  }
  
  // Small delay to prevent overwhelming the serial buffer
  delay(10);
}

// ==================== FUNCTION IMPLEMENTATIONS ====================

/**
 * Read temperature from DS18B20 sensor
 * Returns temperature in Celsius or -127.0 on error
 */
float readTemperature() {
  sensors.requestTemperatures();
  float temp = sensors.getTempC(sensorAddress);
  return temp;
}

/**
 * Send temperature command to Raspberry Pi via USB serial
 * Format: TEMP <cabin_id> <temperature>
 */
void sendToRaspberryPi(int cabinId, float temperature) {
  int tempInt = (int)round(temperature);
  
  // Send actual temperature (clamped to display range for TEMP command)
  int tempDisplay = tempInt;
  if (tempDisplay < 10) {
    tempDisplay = 10;
  } else if (tempDisplay > 35) {
    tempDisplay = 35;
  }
  
  Serial.print("TEMP ");
  Serial.print(cabinId);
  Serial.print(" ");
  Serial.println(tempDisplay);
}

/**
 * Send emergency alert for high temperature
 * Format: EMERGENCY <cabin_id>
 */
void sendHighTempEmergency(int cabinId, float temperature) {
  int tempInt = (int)round(temperature);
  
  Serial.print("HIGH TEMP ALERT! Cabin ");
  Serial.print(cabinId);
  Serial.print(": ");
  Serial.print(tempInt);
  Serial.print("°C (Threshold: ");
  Serial.print(HIGH_TEMP_THRESHOLD);
  Serial.println("°C)");
  
  Serial.print("EMERGENCY ");
  Serial.println(cabinId);
}

/**
 * Blink LED to indicate activity
 * @param times Number of blinks
 * @param delayMs Delay between blinks in milliseconds
 */
void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}
