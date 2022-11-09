/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <WiFi.h>
#include <ArduinoJson.h>

const char* ssid     = "sala da justica";
const char* password = "26041996";

const char* host = "10.0.0.189";
const char* streamId   = "....................";
const char* privateKey = "....................";

void setup()
{
    Serial.begin(115200);
    delay(10);

    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

int value = 0;
WiFiClient client;

void loop()
{
    delay(5000);
    ++value;

    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    
    const int httpPort = 8085;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
    }

    // We now create a URI for the request
    String url = "/data.json";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
    skipResponseHeaders();
    
    disconnect();
    wait();
    Serial.println();
    Serial.println("closing connection");
}

bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(1000);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    Serial.println("No response or invalid response!");
  }else{
    Serial.println("OK");
    DynamicJsonDocument doc(16384);
    Serial.println(doc.capacity());
    DeserializationError  err = deserializeJson(doc, client,DeserializationOption::NestingLimit(20));
    if (err) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(err.c_str());
    }else{
        JsonObject obj = doc.as<JsonObject>();
        String sensor = obj["Children"][0]["Children"][0]["Text"];
        Serial.println(sensor);
    }
    
  }

  return ok;
}

// Close the connection with the HTTP server
void disconnect() {
  Serial.println("Disconnect");
  client.stop();
}

// Pause for a 1 minute
void wait() {
  Serial.println("Wait 3 seconds");
  delay(3000);
}