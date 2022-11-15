#include <ESP8266WiFi.h>
#include "FS.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


bool initWiFi(String ssid, String password, int timeOut = 60000);
void reconnection(String ssid, String password, int timeOut = 300000);
void logSystem(String str, String func = "");

const char *ssid = "ESP8266";
const char *password = "987654321";
IPAddress local_IP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

bool ap_mode = true;
bool wifi_seting = false;
int recon_try_count = 0;
int recon_try_count_max = 30;

//Variables to save values from HTML form
String ssid_;
String pass_;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "scan";
const char* PARAM_INPUT_4 = "";

long long int  startTimeToRevifyConnection;
long long int  mills;
int time_add_ap_mode_startTimeToRevifyConnection = 300000; // 5 min
unsigned long intervalTimeToVerifyConnection = 60000; // 1 min

bool restart = false;

String scan = "";
String listSSID = "";

AsyncWebServer  server(80);

void setup()
{
 	Serial.begin(115200);

  if(!SPIFFS.begin()){
    logSystem("An Error has occurred while mounting SPIFFS", "setup");
    return;
  }

  // Load values saved in SPIFFS
  wifi_seting = loadWiFiSettings();

  if ( !wifi_seting || !initWiFi(ssid_, pass_)){
    startTimeToRevifyConnection = millis() + time_add_ap_mode_startTimeToRevifyConnection;
    if (!configWiFiMode()){
      ESP.restart();
    }
  }else{
    startTimeToRevifyConnection = millis();
  }
}

void loop() {
 	if(restart){
    ESP.restart();
  }

  reconnection(ssid_, pass_);
  
}

bool loadWiFiSettings(){
  ssid_ =  "";

  ssid_ = readFile(ssidPath);
  pass_ = readFile(passPath);
  
  if (ssid_ == ""){
    return false;
  }else{
    logSystem("Network name saved: " + String(ssid_), "loadWiFiSettings");
  }

  return true;
}

//log system
void logSystem(String str, String func){

  unsigned long runMillis= millis();
  unsigned long allSeconds=millis()/1000;
  int runHours= allSeconds/3600;
  int secsRemaining=allSeconds%3600;
  int runMinutes=secsRemaining/60;
  int runSeconds=secsRemaining%60;

  char buf[21];
  sprintf(buf,"%02d:%02d:%02d",runHours,runMinutes,runSeconds);
  Serial.print(buf);
  Serial.print("> ");
  Serial.print(str);
  Serial.print(" :");
  Serial.println(func);
}

// Start AP mode
bool configWiFiMode(){

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
      logSystem("Network name: " + String(WiFi.SSID(i)) + "\nSignal strength: " + String(WiFi.RSSI(i)) + "\n-----------------------", "configWiFiMode");

  }

  WiFi.mode(WIFI_AP);
  ap_mode = true;
  String str_aux = "Setting soft-AP configuration ... ";
  if (WiFi.softAPConfig(local_IP, gateway, subnet))
    str_aux += "Ready";
  else
    str_aux += "Fail!";
 	
  logSystem(str_aux, "configWiFiMode");  
  str_aux = "Setting soft-AP ... ";
  if (WiFi.softAP(ssid))
    str_aux += "Ready";
  else
    str_aux += "Fail!";
  logSystem(str_aux, "configWiFiMode");  

  IPAddress ip = WiFi.localIP();
  String Ip = String(ip[0])+'.'+ String(ip[1])+'.'+ String(ip[2])+'.'+ String(ip[3]);
 	logSystem("Soft-AP IP address = " + Ip);

  server.begin();
  logSystem("Servidor iniciado"); 
  IPAddress ip_ap = WiFi.softAPIP();
  String ip_AP = String(ip_ap[0])+'.'+ String(ip_ap[1])+'.'+ String(ip_ap[2])+'.'+ String(ip_ap[3]);
  logSystem("IP para se conectar ao NodeMCU: http://" + ip_AP); 

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
            logSystem("SSID set to: "+String(ssid_));
            writeFile(SPIFFS, ssidPath, ssid_);
            // Write file to save value

          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass_ = p->value().c_str();
            logSystem("Password set ");
            writeFile(SPIFFS, passPath, pass_);
            // Write file to save value
          }

          if (p->name() == PARAM_INPUT_3) {
            logSystem("SCAN");
            
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
    logSystem("- failed to open file for reading");
    return "";
  }
  
  String fileContent;
  while(file.available()){
    fileContent += file.readStringUntil(EOF);
  }
  return fileContent;
}

// timeout de 5 minuto
bool initWiFi(String ssid, String password, int timeOut) {
  unsigned long timeStart = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  logSystem("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED && (millis() - timeStart) < timeOut) {
    logSystem(".");
    delay(1000);
  }

  if((millis() - timeStart) >= timeOut){
    logSystem("Connecting to WiFi FAIL");
    return false;
  }
  ap_mode = false;
  logSystem("Connecting to WiFi success");
  IPAddress ip_ap = WiFi.localIP();
  String ip = String(ip_ap[0])+'.'+ String(ip_ap[1])+'.'+ String(ip_ap[2])+'.'+ String(ip_ap[3]);
  logSystem("Device IP: " + ip);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  // Configura as funções das páginas
  String message = "<html>\
 <meta http-equiv=\'content-type\' content=\'text/html; charset=utf-8\'>\
 <h1>Central de controle dos LEDs</h1>\
 <p> Apague e acione os LEDs facilmente.</p>\
 <p> Basta apertar os botões.</p>\
 <img src='https://mundoprojetado.com.br/wp-content/uploads/2018/06/Template2-e1528172108632.png'>\
</html>";

  server.on("/", HTTP_GET, [message](AsyncWebServerRequest *request){
    request->send(200, "text/html", message);
  });

  // Inicializa o servidor
  server.begin();
  Serial.println("Web Server iniciado");

  return true;
}

// verify connection status
String connectionStatus() {
  
  switch (WiFi.status()){
    case WL_NO_SSID_AVAIL:
      return "WL_NO_SSID_AVAIL";
      break;
    case WL_CONNECTED:
      return "WL_CONNECTED";
      break;
    case WL_CONNECT_FAILED:
      return "WL_CONNECT_FAILED";
      break;
  }
  return "NULL";

}

// verify the wifi status and handle the response
//  fail to connect to wifi: try more (recon_try_count_max) * times
//  no ssid available: Restart and enter in AP mode.
void reconnection(String ssid, String password, int timeOut){
  mills = millis();
  if( wifi_seting && mills - startTimeToRevifyConnection > intervalTimeToVerifyConnection){
    if(ap_mode){
      logSystem("try to connect", "loop"); 
      if (!initWiFi(ssid_, pass_)){
        if (!configWiFiMode()){
          ESP.restart();
        }
      }
      startTimeToRevifyConnection = millis() + time_add_ap_mode_startTimeToRevifyConnection;
    }else{
      String receiv = connectionStatus();
      if(receiv == "WL_CONNECT_FAILED" ){
        logSystem("try to reconnect", "loop"); 
        //try to reconnect while we receive a fail response or pass the limits
        while (!initWiFi(ssid, password) && recon_try_count < recon_try_count_max){
          recon_try_count ++;
        }
        if(recon_try_count >= recon_try_count_max){
          ESP.restart();
        }else{
          recon_try_count = 0;
        }
      }else if (receiv == "WL_NO_SSID_AVAIL"){
        ESP.restart();
      }
      startTimeToRevifyConnection = millis(); // reset timer
    }
  }
  
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char *  path, String  message){
  logSystem("Writing file: "+ String(path));

  File file = fs.open(path, "w");
  if(!file){
    logSystem("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
   logSystem("- file written");
  } else {
   logSystem("- frite failed");
  }
}

