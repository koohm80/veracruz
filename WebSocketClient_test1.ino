
#include <ArduinoWebsockets.h>
#include <ESP8266WiFi.h>

const char* ssid = "hodol"; //Enter SSID
const char* password = "600589rr"; //Enter Password
const char* websockets_server_host = "219.255.105.186"; //Enter server adress
const uint16_t websockets_server_port = 8181; // Enter server port

using namespace websockets;

WebsocketsClient client;

void setup() {
    Serial.begin(115200);
    // Connect to wifi
    WiFi.begin(ssid, password);

    // Wait some time to connect to wifi
    for(int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
        Serial.print(".");
        delay(1000);
    }
    pinMode(LED_BUILTIN, OUTPUT); // LED 점등 테스트용
    
    // Check if connected to wifi
    if(WiFi.status() != WL_CONNECTED) {
        Serial.println("No Wifi!");
        return;
    }

    Serial.println("Connected to Wifi, Connecting to server.");
    // try to connect to Websockets server
    bool connected = client.connect(websockets_server_host, websockets_server_port, "/");
    if(connected) {
        Serial.println("Connecetd!");
        client.send("VERACRUZ : Hello Server");
    } else {
        Serial.println("Not Connected!");
    }
    
    // run callback when messages are received
    client.onMessage([&](WebsocketsMessage message) {
        Serial.print("Got Message: ");
        Serial.println(message.data());

        //LED 점등 테스트용
        if (message.data() == "Engine_start/"){
          digitalWrite(LED_BUILTIN, LOW);
        }
        if (message.data() == "Engine__stop/"){
          digitalWrite(LED_BUILTIN, HIGH);
        //LED 테스트용
        }
    });
}

unsigned long last_10sec = 0;
unsigned int counter = 0;
String minute = "";
void loop() {
    // let the websockets client check for incoming messages
    if(client.available()) {
        client.poll();
    }
    
    unsigned long t = millis();
    if((t - last_10sec) > 10 * 6000) { //지정 시간 마다 브로드캐스트 테스트 1000은 1초
        counter++;
        minute = "VERACRUZ : I am alive, " + (String)counter + " minute";
        client.send(minute);
        last_10sec = millis();
    }

//    delay(500);
}
