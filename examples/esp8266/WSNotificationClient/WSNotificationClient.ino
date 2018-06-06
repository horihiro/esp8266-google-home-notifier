#ifdef ARDUINO_ARCH_8266
#include <ESP8266WiFi.h>
#endif
#ifdef ARDUINO_ARCH_32
#include <WiFi.h>
#endif
#include <WebSocketsClient.h>
#include <esp8266-google-home-notifier.h>

const char* ssid     = "<REPLASE_YOUR_WIFI_SSID>";
const char* password = "<REPLASE_YOUR_WIFI_PASSWORD>";
const char* wsserver = "<REPLACE_YOUR_WEBSOCKET_SERVER>";

WebSocketsClient webSocket;
GoogleHomeNotifier ghn;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("");
  Serial.print("connecting to Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //Print the local IP
  
  Serial.println("connecting to Google Home...");
  if (ghn.device("Family Room", "ja") != true) {
    Serial.println(ghn.getLastError());
    return;
  }
  Serial.print("found Google Home(");
  Serial.print(ghn.getIPAddress());
  Serial.print(":");
  Serial.print(ghn.getPort());
  Serial.println(")");
  webSocketConnect();
}

void webSocketConnect() {
  webSocket.beginSSL(wsserver, 443, "/ws/phrase");
  webSocket.onEvent(webSocketEvent);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t lenght) {
  switch(type) {
  case WStype_DISCONNECTED:
    Serial.printf("[WSc] Disconnected!\n");
    break;
  case WStype_CONNECTED:
    Serial.printf("[WSc] Connected to url: %s\n",  payload);
    break;
  case WStype_TEXT:
    Serial.printf("[WSc] get text: %s\n", payload);
    if (strcmp((char*)payload, "") != 0){
      webSocket.disconnect();
      if (ghn.notify((char*)payload) != true) {
        Serial.println(ghn.getLastError());
        return;
      }
      webSocketConnect();
    }
    break;
  case WStype_BIN:
    Serial.printf("[WSc] get binary lenght: %u\n", lenght);
    hexdump(payload, lenght);
    break;
  }
}


void loop() {
  webSocket.loop();
}