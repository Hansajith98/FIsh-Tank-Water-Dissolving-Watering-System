/*

- This is Aquaponic Automation System Sorce Code.
- This Aquaponic_Nodemcu.ino file will handle the firebase stuff with 
based on receiving sensor data as json file from Uno module using Serial Commiunication.
- This file need config.h file for execution. All the configurations are stored in that file.

*/

/*  libraries import  */
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <time.h>
#include <ArduinoJson.h>

/*  Provide the token generation process info.  */
#include <addons/TokenHelper.h>

/*  Provide the RTDB payload printing info and other helper functions.  */
#include <addons/RTDBHelper.h>

/*  confirguarion file import  */
#include "config.h"

/*  define inbuilt LED pin  */
#define LED_PIN 2


/*  Define Firebase Data object */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/*  Initialize local time with Sri Lanka Time Zone with NTP Servers  */
static void initializeTime()
{
  configTime(5.5 * 3600, 0, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < 1510592825)
  {
    delay(500);
    now = time(NULL);
  }
}

/*  This will return local time now  */
static char* getCurrentLocalTimeString()
{
  time_t now = time(NULL);
  return ctime(&now);
}

/*  this will initialize the system work before it starts with setup()  */
void setup()
{
  pinMode(LED_PIN, OUTPUT);

  /*  This will initialize bult-in Serial bus for commiunicate with Arduino Uno  */
  Serial.begin(4800);   // Use the lowest possible data rate to reduce error ratio
  while (!Serial) continue;

  /*  connect to the WiFi network  */
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
  }

  initializeTime();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);

}

/*  work process of system will perform by here with loop() forever  */
void loop()
{

  // Check if the other Arduino is transmitting
  if (Serial.available())
  {
    /*  Allocate the JSON document  */
    StaticJsonDocument<300> doc;    // This one must be bigger than for the sender because it must store the strings

    /*  Read the JSON document from the buiilt-in serial port */
    DeserializationError err = deserializeJson(doc, Serial);

    if (err == DeserializationError::Ok)
    {
      SendDataToFirebase(doc);
    }
    else
    {
      /*  Flush all bytes in the "link" serial port buffer  */
      while (Serial.available() > 0)
        Serial.read();
    }
  }
}

/*  This will senddata to the Firebase database  */
void SendDataToFirebase(StaticJsonDocument<300> doc) {
  if (Firebase.ready())
  {

    time_t now = time(NULL);
    char CurrentTime [80] = "\0";
    strftime(CurrentTime, sizeof(CurrentTime), "%a %b %d %T %Y", localtime(&now));
    String DataRowId = "/SensorTimeSeries/" + String(CurrentTime) + "/";

    String Air_TemperatureId = "Air_Temperature";
    String Air_HumidityId = "Air_Humidity";
    String Soil_MoistureId = "Soil_Moisture";
    String Water_LevelId = "Water_Level";
    String Tank_TemperatureId = "Tank_Temperature";
    String Motion_DetectedId = "Motion_Detected";
    String Water_Pump_StatusId = "Water_Pump_Status";

    float Air_Temperature = doc["Air_Temperature"].as<float>();
    float Air_Humidity = doc["Air_Humidity"].as<float>();
    int Soil_Moisture = doc["Soil_Moisture"].as<int>();
    int Water_Level = doc["Water_Level"].as<int>();
    float Tank_Temperature = doc["Tank_Temperature"].as<float>();
    int Motion_Detected = doc["Motion_Detected"].as<int>();
    int Water_Pump_Status = doc["Water_Pump_Status"].as<int>();
    
    digitalWrite(LED_PIN, HIGH);

    FirebaseJson json;
    json.add(Air_TemperatureId, Air_Temperature);
    json.add(Air_HumidityId, Air_Humidity);
    json.add(Soil_MoistureId, Soil_Moisture);
    json.add(Water_LevelId, Water_Level);
    json.add(Tank_TemperatureId, Tank_Temperature);
    json.add(Motion_DetectedId, Motion_Detected);
    json.add(Water_Pump_StatusId, Water_Pump_Status);
    bool error = Firebase.updateNode(fbdo, DataRowId, json);
    
    delay(10);
    
    FirebaseJson Sensor_json;
    Sensor_json.add(Air_TemperatureId, Air_Temperature);
    Sensor_json.add(Air_HumidityId, Air_Humidity);
    Sensor_json.add(Soil_MoistureId, Soil_Moisture);
    Sensor_json.add(Water_LevelId, Water_Level);
    Sensor_json.add(Tank_TemperatureId, Tank_Temperature);
    Sensor_json.add(Motion_DetectedId, Motion_Detected);
    Sensor_json.add(Water_Pump_StatusId, Water_Pump_Status);
    error = Firebase.updateNode(fbdo, "/SensorRealtime/", Sensor_json);

    digitalWrite(LED_PIN, LOW);
  }
}
