
// Imports related to Wifi Client
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// Global variables related to Wifi Client
ESP8266WiFiMulti WiFiMulti;


// Imports related to Receiver radio module
#include <SPI.h>
#include "printf.h"
#include "RF24.h"


// Global variables related to the recieving temperature
int temperature;
// Instantiate an object for the nRF24L01 transceiver
// TODO: Need to change these pins!
RF24 radio(9, 10);  // using pin 7 for the CE pin, and pin 8 for the CSN pin
// Let these addresses be used for the pair
uint8_t address[][6] = { "1Node", "2Node" };
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit
bool radioNumber = 1;  // 0 uses address[0] to transmit, 1 uses address[1] to transmit

// Used to control whether this node is sending or receiving
bool role = false;  // true = TX role, false = RX role

// For this example, we'll be using a payload containing
// a single float number that will be incremented
// on every successful transmission
float payload = 0.0;



// Global variable for relay status
int relayStatus = 0;


void setupWifi(){
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("shellyplug-s-4022D88965C1", "relayrelay");
}

void setupRadio(){
  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    while (1) {}  // hold in infinite loop
  }
  radio.setDataRate( RF24_250KBPS );
  // print example's introductory prompt
  // To set the radioNumber via the Serial monitor on startup
  //Serial.println(F("Which radio is this? Enter '0' or '1'. Defaults to '0'"));
  //while (!Serial.available()) {
    // wait for user input
  //}
  char input = 1;
  radioNumber = input == 1;
  
  // role variable is hardcoded to RX behavior, inform the user of this
  
  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity to
  // each other.
  radio.setPALevel(RF24_PA_HIGH);  // RF24_PA_MAX is default.

  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a float
  radio.setPayloadSize(sizeof(payload));  // float datatype occupies 4 bytes

  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);  // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]);  // using pipe 1

  // additional setup specific to the node's role
  if (role) {
    radio.stopListening();  // put radio in TX mode
  } else {
    radio.startListening();  // put radio in RX mode
  }
}



int turnOnRelay(){
  int status = 0;
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, "http://192.168.33.1/relay/0?turn=on")) {  // HTTP

      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          status = 1;
          String payload = http.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
  return status;
}

int turnOffRelay(){
  int status = 0;
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, "http://192.168.33.1/relay/0?turn=off")) {  // HTTP

      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          status = 1;
          String payload = http.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
  return status;
}

int waitForTemperatureMsg(){
    int result = 0;
    // This device is a RX node

    uint8_t pipe;
    if (radio.available(&pipe)) {              // is there a payload? get the pipe number that recieved it
      uint8_t bytes = radio.getPayloadSize();  // get the size of the payload
      radio.read(&payload, bytes);             // fetch payload from FIFO
      result = 1;
      /*
      Serial.print(F("Received "));
      Serial.print(bytes);  // print the size of the payload
      Serial.print(F(" bytes on pipe "));
      Serial.print(pipe);  // print the pipe number
      Serial.print(F(": "));
      Serial.println(payload);  // print the payload's value
      */
    }
    return result;
}

void setup() {

  // Setup radio
  setupRadio();


  // Setup Wifi client
  setupWifi();  
}



void loop() {
   // Stand in infinite wait for a temp message from the radio module (i.e. polling mode)
   // temperature = recieveTemp();
   // Temperature in main building is now known in variable houseTemp
   //temperature = waitForTemperatureMsg();
   temperature = 40; //dummy temp

   // The relayStatus != 1 prevents the controller from repeatedly sending "ON" commands
   if(temperature < 45 && relayStatus != 1){
    relayStatus = turnOnRelay();
    
   } else if(temperature > 60 && relayStatus != 0){
    relayStatus = turnOffRelay();
   }

  // Sleep for 30 seconds
  delay(30000);
  
}
