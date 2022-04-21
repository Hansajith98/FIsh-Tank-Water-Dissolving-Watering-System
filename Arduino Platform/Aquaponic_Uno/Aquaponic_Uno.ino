/*

  - This is Aquaponic Automation System Sorce Code.
  - This Aquaponic_Uno.ino file will handle the all the sensors and controlling stuff and also
  it sends data to the NodeMcu ESP8266 module using Serial Commiunication.
  - This file need config.h file for execution. All the configurations are stored in that file.

*/

/*Impport following Libraries*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <dht11.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

/* Import configure file for configuration data */
#include "config.h"

/* Define Serial commiunication link for commiunicate with nodemcu ESP8266 */
SoftwareSerial linkSerial(10, 11); // RX, TX

/* I2C pin declaration for LCD display */
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

/* Pin definements */
#define DHT11PIN 4  // DHT11 sensor value reading pin
#define WATER_LEVEL_POWER_PIN  6  // Water Level sensor power output pin
#define WATER_LEVEL_SIGNAL_PIN A3 // Water Level sensor data reading pin
#define WATER_TEMPERATURE_SIGNAL_PIN 2  // DS18B20 Water Proof Temperature Sensor Input pin
#define SOIL_MOISTURE_SIGNAL_PIN A2 // Soil Moisture Sensor 
#define MOTION_SENSOR_SIGNAL_PIN 3
#define WATER_PUMP_PIN 9
#define RESET_POWER_PIN 8


dht11 DHT11;  // DHT11 object for handle DHT11 sensor work process
OneWire oneWire(WATER_TEMPERATURE_SIGNAL_PIN);  // Create data bus connectino for DS18B20 senesor
DallasTemperature sensors(&oneWire);  // DallasTemperature library object for handle DS18B20 waterproof temperature work process

/*  Variable definition for sensor values  */
bool Motion_Detected = 0; // by default, no motion detected
float Air_Temperature;
int Air_Humidity;
float Tank_Temperature;
int Soil_Moisture;
int Water_Level;
bool Water_Pump_Status = 0; // Water pump is off when defaults
int Daily_Water_Need;

/*  Vaiable definition for time based method calling  */
unsigned long sendDataPrevMillis = 0;
unsigned long printDataPrevMillis = 0;
unsigned long updateDataPrevMillis = 0;
unsigned long waterPumpPrevMillis = 0;


/*  Process Innitaliczation with setup()  */
void setup()
{

  lcd.begin(16, 2); //Defining 16 columns and 2 rows of lcd display
  lcd.backlight();  //To Power ON the back light
  pinMode(WATER_LEVEL_POWER_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  digitalWrite(WATER_LEVEL_POWER_PIN, LOW); // turn the water level sensor power OFF when starts
  digitalWrite( WATER_PUMP_PIN, HIGH );  // turn off the water pump when starts
  digitalWrite( RESET_POWER_PIN, HIGH );  // handle reset method programatically
  pinMode(RESET_POWER_PIN, OUTPUT);   // pin for power to invoke the reset pin
  pinMode(MOTION_SENSOR_SIGNAL_PIN, INPUT);
  Serial.begin(9600);        // initialize serial print for debugging

  PrintWelcomeMessage();  // Print welcome messgae when starts the system

  /*  Intialize serial bus for connect with NodeMcu ESP8266  */
  linkSerial.begin(4800);   // Use the lowest possible data rate to reduce error ratio

  Daily_Water_Need = CheckDailyWaterNeedforSoil();

}

/*  Process running with loop() forever  */
void loop() {

  if (millis() - updateDataPrevMillis > 1000 || updateDataPrevMillis == 0) {  // condition will true in every second after system Booted
    updateDataPrevMillis = millis();
    UpdateSensorData();
    printData();
  }
  if (millis() - sendDataPrevMillis > 6000 || sendDataPrevMillis == 0) {   // condition will true in every 6 second after system Booted
    sendDataPrevMillis = millis();
    SendDataToNodemcu();
  }

  if (millis() > 86400000) { // System will Reboot in evry 24 hours system after started to reduce errors and clean RAM
    ResetUno();
  }

  int soil_dry_level = CheckSoilDrylevel();

  if ( (soil_dry_level == 1) || (soil_dry_level == 2) ) {   // If soil has dried, watering process will start
    if ( CheckSafeToReleaseWater() ) {
      ReleaseWater( soil_dry_level );
    }
  }
}

/*  Print string messges on LCD display  */
void Print_Message( String message ) {
  lcd.clear();//Clean the screen
  lcd.setCursor(0, 0);
  lcd.print(message);
}

/*  Method for Print Welcome message on LCD display  */
void PrintWelcomeMessage() {
  lcd.clear();//Clean the screen
  lcd.setCursor(3, 0);
  lcd.print("Aquaponic");
  scrollMessage(1, "Welcome to the Aquaponic Automation System", 250, 16);
}

/*  Method for scroll Strings on LCS display  */
void scrollMessage(int row, String message, int delayTime, int totalColumns) {
  for (int i = 0; i < totalColumns; i++) {
    message = " " + message;
  }
  message = message + " ";
  for (int position = 0; position < message.length(); position++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(position, position + totalColumns));
    delay(delayTime);
  }
}

/*  Re-read all sensor data and store fresh data on respective variables  */
void UpdateSensorData() {
  Air_Temperature = CheckDHT11Temperature();
  Air_Humidity = CheckDHT11Humidity();
  //  Tank_Temperature = CheckWaterTemperature();   // Please Un-Comment this when new DS18B20 temperature sensor has replaced
  Tank_Temperature = CheckDHT11Temperature() - 5;   // Removethis line when new DS18B20 temperature sensor has replaced
  Soil_Moisture = CheckSoilMoisture();
  Water_Level = CheckWaterLevel();
  DetectMotion();
}

/*  Check and return air humidity usign DSH11 sensor  */
int CheckDHT11Humidity() {
  int chk = DHT11.read(DHT11PIN);
  return (int)DHT11.humidity;
}

/*  Check and return air temperature usign DSH11 sensor  */
float CheckDHT11Temperature() {
  int chk = DHT11.read(DHT11PIN);
  return (float)DHT11.temperature;
}

/*  Check and return waterlevel of tank  */
int CheckWaterLevel() {
  analogWrite(WATER_LEVEL_POWER_PIN, 255);  // turn the sensor ON
  delay(10);                      // wait 10 milliseconds
  int value = analogRead(WATER_LEVEL_SIGNAL_PIN); // read the analog value from sensor
  digitalWrite(WATER_LEVEL_POWER_PIN, LOW);   // turn the sensor OFF
  float water_level = value / 5;
  return (int)water_level;
}

/*  Check and return water temperature of tank unig DS18B20 sensor  */
float CheckWaterTemperature() {
  sensors.requestTemperatures();
  float Celcius = sensors.getTempCByIndex(0);
  return Celcius;
}

/*  Check and return soil moisture of soil  */
int CheckSoilMoisture() {
  int Soil_Moisture = analogRead(SOIL_MOISTURE_SIGNAL_PIN);
  return Soil_Moisture;
}

/*  Check and return is water tank has fish. Method will return 1 if fish have and return 0 if not.  */
int DetectMotion() {
  int val = digitalRead(MOTION_SENSOR_SIGNAL_PIN);   // read sensor value
  if (val == HIGH) {           // check if the sensor is HIGH
    Motion_Detected = 1;
  }
  else {
    Motion_Detected = 0;
  }
}

/*  Calculate water need for soil with config data using Blaney-Criddle Equation  */
float CheckDailyWaterNeedforSoil() {
  float daily_water_need = AVG_DAILY_DAYTIME_HOURS * ((0.46 * Air_Temperature) + 8) * CROP_KC_VALUE;
  return daily_water_need;
}

/*  Check is tank in safe status to release water and return true if yes, false if no  */
bool CheckSafeToReleaseWater() {
  if (!Motion_Detected) {
    if ( Tank_Temperature < TANK_SAFE_MAX_WATER_TEMPERATURE && Tank_Temperature > TANK_SAFE_MIN_WATER_TEMPERATURE) {
      if ( Water_Level > TANK_SAFE_MIN_WATER_LEVEL ) {
        return true;
      }
    }
  }
  return false;
}

/*  Check and return soil moisture. 1 for high dry level and 2 for extremly dry level. 0 for normal level  */
int CheckSoilDrylevel() {
  int SoilMoisture = CheckSoilMoisture();
  int DryLevel;
  if (SoilMoisture > SOIL_MIN_EXTREMLY_DRY_MOISTURE) {
    DryLevel = 2;
  } else if(SoilMoisture > SOIL_MIN_NORMAL_DRY_MOISTURE){
    DryLevel = 1;
  } else {
    DryLevel = 0;
  }
  return DryLevel;
}

/*  This method will release water turning on the pump and continuesly check if water release is safe  */
void ReleaseWater( int soil_dry_level ) {

  if(soil_dry_level == 2 && ((Daily_Water_Need / WATER_PUMP_SPEED) / 2) < 1){
    Daily_Water_Need = round(CheckDailyWaterNeedforSoil()/4);
  }

  int trip_rounds = round( (Daily_Water_Need / WATER_PUMP_SPEED) / 2);  // Water volume need to watering will calculate based on dry level and daily water need
  for (int i = 0; i < trip_rounds; i++) {
    Print_Message("Water Releasing");
    digitalWrite( WATER_PUMP_PIN, LOW );
    Water_Pump_Status = 1;
    delay(2000);
    if( (Daily_Water_Need - (WATER_PUMP_SPEED * 2)) > 1){
      Daily_Water_Need -= WATER_PUMP_SPEED * 2;
    } else {
      Daily_Water_Need = 0;
      digitalWrite( WATER_PUMP_PIN, HIGH );
      Water_Pump_Status = 0;
      printData();
      return;
    }
    UpdateSensorData();
    SendDataToNodemcu();
    if (!CheckSafeToReleaseWater()) {
      digitalWrite( WATER_PUMP_PIN, HIGH );
      Water_Pump_Status = 0;
//      SendDataToNodemcu();
      printData();
      return;
    }
  }
  digitalWrite( WATER_PUMP_PIN, HIGH );
  Water_Pump_Status = 0;
  SendDataToNodemcu();
  return;
}

/*  Sending data trough linkSerail Bus will done by this method. Data will send as Json data  */
void SendDataToNodemcu() {

  StaticJsonDocument<200> doc;
  doc["Air_Temperature"] = Air_Temperature;
  doc["Air_Humidity"] = Air_Humidity;
  doc["Soil_Moisture"] = Soil_Moisture;
  doc["Water_Level"] = Water_Level;
  doc["Tank_Temperature"] = Tank_Temperature;
  doc["Motion_Detected"] = Motion_Detected;
  doc["Water_Pump_Status"] = Water_Pump_Status;

  serializeJson(doc, linkSerial);
  return;
}

/*  All sensor data wil print on LCD diplay in one by one basis in every second  */
void printData() {
  if (millis() - printDataPrevMillis > 5000 ) {
    printAirHumidity();
    printDataPrevMillis = millis();
    return;
  } else if (millis() - printDataPrevMillis > 4000 ) {
    printAirTemperature();
    return;
  } else if (millis() - printDataPrevMillis > 3000 ) {
    printTankTemperature();
    return;
  } else if (millis() - printDataPrevMillis > 2000 ) {
    printTankWaterLevel();
    return;
  } else if (millis() - printDataPrevMillis > 1000 ) {
    printSoilMoisture();
    return;
  } else {
    Print_Message("  System : On  ");
  }
}

/* This will print air humidity data */
void printAirHumidity() {
  lcd.clear();//Clean the screen
  lcd.setCursor(0, 0);
  lcd.print("Air Humidity:");
  lcd.setCursor(5, 1);
  lcd.print( String(Air_Humidity) + " %" );
}

/* This will print air temerature data */
void printAirTemperature() {
  lcd.clear();//Clean the screen
  lcd.setCursor(0, 0);
  lcd.print("Air Temperature:");
  lcd.setCursor(5, 1);
  lcd.print( String(Air_Temperature) + " C" );

}

/* This will print tank temperatire data */
void printTankTemperature() {
  lcd.clear();//Clean the screen
  lcd.setCursor(0, 0);
  lcd.print("Tank Temperature:");
  lcd.setCursor(5, 1);
  lcd.print( String(Tank_Temperature) + " C" );
}

/* This will print tank water level data */
void printTankWaterLevel() {
  lcd.clear();//Clean the screen
  lcd.setCursor(0, 0);
  lcd.print("Tank Water Level:");
  lcd.setCursor(5, 1);
  lcd.print( String(Water_Level) + " %" );
}

/* This will print soil moisture value */
void printSoilMoisture() {
  lcd.clear();//Clean the screen
  lcd.setCursor(0, 0);
  lcd.print("Soil Moisture:");
  lcd.setCursor(5, 1);
  lcd.print( String(Soil_Moisture) );
}

/* This method will reset the arduino programmatically */
void ResetUno() {
  digitalWrite(RESET_POWER_PIN, LOW);
}
