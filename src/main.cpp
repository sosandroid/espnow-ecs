
#include <Arduino.h>
#include <wifi.h>
#include <string.h>
#include <OneWire.h>
#include <DS18B20.h>

#define DEBUG

#ifdef DEBUG
    #define BAUD_RATE 115200
#endif
#define NODE_NAME "ECS_T\0"


/*************
* Data handling
***************/

struct Message_t {
    String node;
    float ecsT;
    //float ambT;
};

Message_t message {NODE_NAME, 0.0f};
boolean data_ready = false;

void debug_message(Message_t& data);

/**********
 * One Wire stuff
 * *********/

#define ONE_WIRE_BUS 2
#define DS_RESOLUTION 10

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

void sensors_setup(void);
void sensors_loop(Message_t& data);


void sensors_setup() {
    sensor.begin();
    sensor.setResolution(DS_RESOLUTION);
    sensor.requestTemperatures();
}

void sensors_loop(Message_t& data) {
    if(sensor.isConversionComplete()) {
        data.ecsT = sensor.getTempC();
        data_ready = true;
        sensor.requestTemperatures();
    }
}

/************
 * Main stuff
 *****/

void setup() {
  #ifdef DEBUG
    Serial.begin(BAUD_RATE);
  #endif
  sensors_setup();
}

void loop() {
  sensors_loop(message);
  if(data_ready) {
    debug_message(message);
    data_ready = false;
  }
}

/*********
 * Debug Stuff
 * 
*/
void debug_message(Message_t& data) {
  #ifdef DEBUG
    Serial.print("Node: ");
    Serial.println(data.node);
    Serial.print("TEMP ECS: ");
    Serial.println(data.ecsT);
  #endif
}