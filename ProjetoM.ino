#include <ESP8266WiFi.h>
#include "FS.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char *ssid = "ESP8266";
const char *password = "987654321";
IPAddress local_IP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);


//Variables to save values from HTML form
String ssid_;
String pass_;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "scan";
const char* PARAM_INPUT_4 = "";

bool restart = false;

String scan = "";
String listSSID = "";

AsyncWebServer  server(80);

void setup()
{
 	Serial.begin(115200);

// Set WiFi to station mode
  // Set WiFi to station mode
  WiFi.mode(WIFI_STA);

  // Disconnect from an AP if it was previously connected
  WiFi.disconnect();
  delay(100);

  int numberOfNetworks = WiFi.scanNetworks();

  for(int i =0; i<numberOfNetworks; i++){

      scan += "<tr><th>"+String(WiFi.SSID(i))+"</th><th>"+String(WiFi.RSSI(i))+"</th></tr>";
      listSSID += "<option>"+ String(WiFi.SSID(i))+ "</option>";
      Serial.print("Network name: ");
      Serial.println(WiFi.SSID(i));
      Serial.print("Signal strength: ");
      Serial.println(WiFi.RSSI(i));
      Serial.println("-----------------------");
 
  }


  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Load values saved in SPIFFS
  //ssid_ = readFile(SPIFFS, ssidPath);
  //pass_ = readFile(SPIFFS, passPath);

  if (!configWiFiMode()){
    ESP.restart();
  }

}
void loop() {
 	if(restart){
    ESP.restart();
  }
}

bool configWiFiMode(){
  WiFi.mode(WIFI_AP_STA);
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

  String index = readFile("/wifimanager.html");
  if(index == ""){
    return false;
  }

  int indexOf = index.indexOf("</table>");
  String pt1 = index.substring(0, indexOf);
  String pt2 = index.substring(indexOf);
 
  String HTML = pt1 + scan + pt2;
  
  indexOf = HTML.indexOf("</datalist>");
  pt1 = HTML.substring(0, indexOf);
  pt2 = HTML.substring(indexOf);

  HTML = pt1 + listSSID + pt2;

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  
  server.on("/", HTTP_GET, [HTML](AsyncWebServerRequest *request){
    request->send(200, "text/html", HTML);
  });


  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid_ = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid_);
            // Write file to save value

          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass_ = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass_);
            // Write file to save value
          }

          if (p->name() == PARAM_INPUT_3) {
            Serial.println("SCAN");
            
            // Write file to save value
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->redirect("/");
      restart = true;
    });

  server.begin();
  return true;
  
}

// Read File from SPIFFS
String readFile(String path){

  File file = SPIFFS.open(path, "r");
  if(!file){
    Serial.println("- failed to open file for reading");
    return "";
  }
  
  String fileContent;
  while(file.available()){
    fileContent += file.readStringUntil(EOF);
  }
  return fileContent;
}