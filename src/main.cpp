
#include <Arduino.h>
#include <wifi.h>
#include <string.h>
#include <OneWire.h>
#include <DS18B20.h>
#include <ezLED.h>
#include <ZHNetwork.h>

#define DEBUG

#ifdef DEBUG
    #define BAUD_RATE 115200
#endif

#define NODE_NAME "ECS_T"

#define ONE_WIRE_BUS 4
#define DS_RESOLUTION 12
#define INTERVAL_TEMP 15000

#define NETWORK_NAME = "SuzeNet"

/*************
* Data handling init
***************/

struct Message_t {
    String node;
    float ecsT;
    //float ambT;
    #ifdef DEBUG
      uint16_t nb; //Simple counter usefull for debug
    #endif
};

Message_t message {NODE_NAME, 0.0f, 0};
boolean data_ready = false;

void debug_message(Message_t& data);

/***********
 * Led init Stuff
******/
ezLED led(LED_BUILTIN);
unsigned long onTime = 250;
unsigned long offTime = 250;
unsigned long blinkTimes = 1;
unsigned long blinkTimesTransmit = 3;

/**********
 * One Wire init tuff
 * *********/

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

unsigned long lastUpdateTemp = 0;
const unsigned long intervalTemp = INTERVAL_TEMP;
boolean conversionOnGoing = false;

void sensorsSetup(void);
void sensorsLoop(Message_t& data);

/***********
 * ESP Now init stuff
 * Try to broadcast to get receiver MAC, then start sending data
*/

ZHNetwork SuzeNet;
const char* networkName = "SuzeNet";

const uint8_t broadcast[6]{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t target[] = {0xD8, 0xBC, 0x38, 0xE2, 0x2A, 0x68};
boolean gatewaySet = false;

void onConfirmRcv(const uint8_t *target, const uint16_t id, const bool status);
void onUnicastRcv(const char *data, const uint8_t *sender);
void getGatewayAddress(void);
void espnowSetup(void);
void espnowSend(Message_t& data);

/************
 * Main functions
 *****/

void setup() {
  digitalWrite(LED_BUILTIN, LOW);
  #ifdef DEBUG
    Serial.begin(BAUD_RATE);
    Serial.println("Starting...");
  #endif
  espnowSetup();
  sensorsSetup();
  //1st measurement
  sensorsLoop(message);
}

void loop() {
  
  if(data_ready) {
    espnowSend(message);
    debug_message(message);
    data_ready = false;
  }
  sensorsLoop(message);
  SuzeNet.maintenance();
  led.loop();
}

/**********
 * Temperature functions
*/

void sensorsSetup() {
    sensor.begin();
    sensor.setResolution(DS_RESOLUTION);
}

void sensorsLoop(Message_t& data) {
    if(!conversionOnGoing && (millis() - lastUpdateTemp) > intervalTemp) {
      sensor.requestTemperatures();
      conversionOnGoing = true;
      led.blinkNumberOfTimes(onTime,offTime,blinkTimes);
    }

    if(conversionOnGoing && sensor.isConversionComplete()) {
        data.ecsT = sensor.getTempC();
        #ifdef DEBUG
          data.nb++;
        #endif
        data_ready = true;
        conversionOnGoing = false;
        lastUpdateTemp = millis();
    }
}

/*********
 * ESP-Now functions
*/

void espnowSetup() {
  SuzeNet.begin(networkName);
  SuzeNet.setOnConfirmReceivingCallback(onConfirmRcv);
  SuzeNet.setOnUnicastReceivingCallback(onUnicastRcv);
  getGatewayAddress();
}
void onConfirmRcv(const uint8_t *target, const uint16_t id, const bool status) {
  //do nothing yet
  #ifdef DEBUG
    Serial.print("MAC: ");
    Serial.println(SuzeNet.macToString(target));
  #endif
}

void espnowSend(Message_t& data){
  SuzeNet.sendUnicastMessage((char *) &data, target, sizeof(data));
}

void onUnicastRcv(const char *data, const uint8_t *sender) {
  /*Get the gateway address*/
  memcpy(target, sender, sizeof(sender));
  gatewaySet = true;
  led.blinkNumberOfTimes(50,50,3);
}

void getGatewayAddress(){
    /*Only gateway should reply to broadcast messages*/
    led.blinkNumberOfTimes(100,100,1000000);
    unsigned long start = 0;
    unsigned long waiting = 500;
    while(!gatewaySet) {
      if(millis() - start > waiting) {
        SuzeNet.sendBroadcastMessage("Someone there?");
        start = millis();
      }
      SuzeNet.maintenance();
      led.loop();
    }
    led.cancel();
    #ifdef DEBUG
      Serial.println("gateway set");
      Serial.print("gateway: ");
      for (int i=0; i < sizeof(target);i++) Serial.print(target[i]);
      Serial.println("");
    #endif
}

/*********
 * Debug functions
 * 
*/
void debug_message(Message_t& data) {
  #ifdef DEBUG
    Serial.print("Node: ");
    Serial.println(data.node);
    Serial.print("TEMP ECS: ");
    Serial.println(data.ecsT);
    Serial.print("nb: ");
    Serial.println(data.nb);
  #endif
}
