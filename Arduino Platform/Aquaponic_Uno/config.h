/*

- This config.h file is for configuration of Aquaponic_Uno.ino file.

*/

/* Fish tank water Temperature thresholds */
#define TANK_SAFE_MAX_WATER_TEMPERATURE 32
#define TANK_SAFE_MIN_WATER_TEMPERATURE 20

/* Fish tank water Level threshold */
#define TANK_SAFE_MIN_WATER_LEVEL 30

/* Soil Humidity which decide */
#define SOIL_MIN_NORMAL_DRY_MOISTURE 800
#define SOIL_MIN_EXTREMLY_DRY_MOISTURE 900

#define AVG_DAILY_DAYTIME_HOURS 10
#define CROP_KC_VALUE 0.75

/*  Speed of water pump */
#define WATER_PUMP_SPEED 28.5 // ml/sec at 3.7V-4.2V
