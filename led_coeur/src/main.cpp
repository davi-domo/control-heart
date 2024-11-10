// code original de LOURORO https://github.com/lauroro/esp8266-ledstrip/


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include "LittleFS.h"
#include <WS2812FX.h>

#define LED_COUNT     1   //For a strip with 144leds, it's possible to use less
#define LED_PIN       2     //D4 on NodeMCU Esp8266


// Wifi params
const char* ssid = "my-ssid";
const char* password = "my-pass";
String myHostname = "coeur";
IPAddress ip(192,168,1,50);     
IPAddress gateway(192,168,1,1);   
IPAddress subnet(255,255,255,0);

// State params
String _color = "c16777215";
String _speed = "s1500";
String _mode = "m12";
uint8_t my_mode[22] = {0,1,7,8,9,10,11,12,15,20,21,22,34,35,42,45,46,47,51,52,55};

// Led strip handler
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Async Web Server + websocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    if(message.indexOf("c") >= 0){
      char color[message.length()];
      message.substring(1).toCharArray(color, message.length());
      ws2812fx.setColor((uint32_t)atol(color));
      ws.textAll(message);
      _color = message;
    }
    else if(message.indexOf("m") >= 0){
      char mode[message.length()];
      message.substring(1).toCharArray(mode, message.length());
      ws2812fx.setMode(atol(mode));
      ws.textAll(message);
      _mode = message;
    }
    else if(message == "su"){
      if(ws2812fx.getSpeed()>100){
        ws2812fx.setSpeed(ws2812fx.getSpeed() - 100); 
        _speed = "s";
        _speed += ws2812fx.getSpeed();
        ws.textAll(_speed);
      }
    }
    else if(message == "sd"){
      if(ws2812fx.getSpeed()<5000){
        ws2812fx.setSpeed(ws2812fx.getSpeed() + 100);  
        _speed = "s";
        _speed += ws2812fx.getSpeed();
        ws.textAll(_speed);
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      //Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      ws.textAll(_color);
      ws.textAll(_speed);
      ws.textAll(_mode);
      break;
    case WS_EVT_DISCONNECT:
      //Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}



void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }

  // Init
  ws2812fx.init();
  ws2812fx.setMode(12);
  ws2812fx.setColor(0xFFFFFF);
  ws2812fx.setSpeed(1500);
  ws2812fx.setBrightness(100);
  ws2812fx.start();

  WiFi.mode(WIFI_STA); 
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
if (!MDNS.begin("coeur")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");

  // HTTP request for modes list
  server.on("/modes", HTTP_GET, [](AsyncWebServerRequest *request){
    String modes = "";
    modes.reserve(5000);
    // Get only first 56 effects ( [0-55] pre-built effects ) 
    for(uint8_t i=0; i < 21; i++) {
      modes += "<button type='button' id='m";
      modes += my_mode[i];
      modes += "'>";
      modes += ws2812fx.getModeName(my_mode[i]);
      modes += "</button>";
    }
    request->send(200, "text/plain", modes);
  });

  // Start server
  server.begin();
  
}

void loop() {
  ws.cleanupClients();
  ws2812fx.service();
}
