#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <esp8266-google-home-notifier.h>

const char* ssid     = "<REPLASE_YOUR_WIFI_SSID>";
const char* password = "<REPLASE_YOUR_WIFI_PASSWORD>";

ESP8266WebServer server(80);
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
  
  const char displayName[] = "Family Room";

  Serial.println("connecting to Google Home...");
  if (ghn.device(displayName, "en") != true) {
    Serial.println(ghn.getLastError());
    return;
  }
  Serial.print("found Google Home(");
  Serial.print(ghn.getIPAddress());
  Serial.print(":");
  Serial.print(ghn.getPort());
  Serial.println(")");
  
  server.on("/speech", handleSpeechPath);
  server.on("/", handleRootPath);
  server.begin();
 }

void handleSpeechPath() {
  String phrase = server.arg("phrase");
  if (phrase == "") {
    server.send(401, "text / plain", "query 'phrase' is not found");
    return;
  }
  if (ghn.notify(phrase.c_str()) != true) {
    Serial.println(ghn.getLastError());
    server.send(500, "text / plain", ghn.getLastError());
    return;
  }
  server.send(200, "text / plain", "OK");
}

void handleRootPath() {
  server.send(200, "text/html", "<html><head></head><body><input type=\"text\"><button>speech</button><script>var d = document;d.querySelector('button').addEventListener('click',function(){xhr = new XMLHttpRequest();xhr.open('GET','/speech?phrase='+encodeURIComponent(d.querySelector('input').value));xhr.send();});</script></body></html>");
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
}
