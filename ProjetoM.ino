#include <ESP8266WiFi.h>
#include "FS.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char *ssid = "ESP8266";
const char *password = "987654321";
IPAddress local_IP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

AsyncWebServer  server(80);

void setup()
{
 	Serial.begin(115200);
  configWiFiMode();

}
void loop() {
 	
}

bool configWiFiMode(){
  Serial.print("Setting soft-AP configuration ... ");
 	Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
 	Serial.print("Setting soft-AP ... ");
 	Serial.println(WiFi.softAP(ssid) ? "Ready" : "Failed!");
 	Serial.print("Soft-AP IP address = ");
 	Serial.println(WiFi.softAPIP());

  server.begin();
  Serial.println("Servidor iniciado"); 
 
  Serial.print("IP para se conectar ao NodeMCU: "); 
  Serial.print("http://"); 
  Serial.println(WiFi.softAPIP());

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return false;
  }

  File f = SPIFFS.open("/wifimanager.html", "r");
  if(!f){
    Serial.println("File error");
    return false;
  }
  f.close();

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/wifimanager.html", "text/html");
  });
  server.begin();
  return true;
}