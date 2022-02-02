/*
  Esp8266 Websockets Client

  This sketch:
        1. Connects to a WiFi network
        2. Connects to a Websockets server
        3. Sends the websockets server a message ("Hello Server")
        4. Prints all incoming messages while the connection is open

  Hardware:
        For this sketch you only need an ESP8266 board.

  Created 15/02/2019
  By Gil Maimon
  https://github.com/gilmaimon/ArduinoWebsockets

*/

#include <ArduinoWebsockets.h>
#include <ESP8266WiFi.h>

const char* ssid = "hodol"; //Enter SSID
const char* password = "600589rr"; //Enter Password
const char* websockets_server_host = "219.255.105.186"; //Enter server adress
const uint16_t websockets_server_port = 8181; // Enter server port

using namespace websockets;

WebsocketsClient client;

#define Relay_1_Engine 16
#define Relay_2_BreakPedal 4
#define Relay_3_Smartkey 5

int Relay_4_Engine_state = 0;

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
    pinMode(Relay_1_Engine, OUTPUT); // 릴레이 시동버튼 제어 출력    
    pinMode(Relay_2_BreakPedal, OUTPUT); // 릴레이 시동버튼 제어 출력
    pinMode(Relay_3_Smartkey, OUTPUT); // 릴레이 스마트키 제어 출력

    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(Relay_1_Engine, HIGH);       // 릴레이 꺼짐으로 초기화
    digitalWrite(Relay_2_BreakPedal, HIGH);
    digitalWrite(Relay_3_Smartkey, HIGH);

    
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
        client.send("VERACRUZ : Hello Server. I'm Connecetd!");
        client.send("Engine__stop/");
    } else {
        Serial.println("Not Connected!");
    }
    
    // 서버에서 메시지 수신시(단독 수신 및 브로드캐스트)
    client.onMessage([&](WebsocketsMessage message) {
        Serial.print("Got Message: ");
        Serial.println(message.data());

        if (message.data() == "Engine_start/"){  // 시동 명령 수신시
          Engine_Start();                        // 엔진시동 함수 호출
        }
        if (message.data() == "Engine__stop/"){  // 스탑 명령 수신시
          Engine_Stop();                         // 엔진스탑 함수 호출
        }
    });
}

unsigned long last_10sec = 0;
unsigned long last_10sec2 = 0;
unsigned int counter = 0;
unsigned int counter2 = 0;
String minute = "";
String Start_Engine_seconds = "";

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

    // 이건 테스트용 완성되면 큰 필요없음
    unsigned long t2 = millis();
    if((t2 - last_10sec2) > 10 * 1000 && digitalRead(LED_BUILTIN) == LOW){ // 시간 지정
       Start_Engine_seconds = "";
       counter2++;
       Start_Engine_seconds = "VERACRUZ : Start Engine, " + (String)counter2 + " *10 seconds";
       client.send(Start_Engine_seconds);
       last_10sec2 = millis();
    } else if(!digitalRead(LED_BUILTIN) == LOW){
        counter2= 0;
    }

//    delay(500);
}


/////////////////// 이하 동작 함수 ///////////////////////

void SmartKey_Start(){
      client.send("VERACRUZ : Start SmartKey");
      digitalWrite(Relay_3_Smartkey, LOW);     // 스마트키 배터리 릴레이 연결
}
void SmartKey_Stop(){
      delay(300);       
      client.send("VERACRUZ : Stop SmartKey");
      digitalWrite(Relay_3_Smartkey, HIGH);     // 스마트키 배터리 릴레이 해제
}

void Engine_Start(){

 // 추가할 것
 // 현재 시동온오프 상태 체크 후 오프이면 시동 절차 시작하기. 예정, => 완료.
 // 딜레이함수 말고 다른방법 있으면 좋으려나?
 
    if(!digitalRead(LED_BUILTIN) == LOW){

      SmartKey_Start();
      delay(1500);
      client.send("VERACRUZ : Start btn 1");
      digitalWrite(Relay_1_Engine, LOW);     //-------------------
      delay(300);                             // 시동버튼 1회
      digitalWrite(Relay_1_Engine, HIGH);      //-------------------
      delay(3000);

      client.send("VERACRUZ : Start btn 2");
      digitalWrite(Relay_1_Engine, LOW);     //-------------------
      delay(300);                             // 시동버튼 2회
      digitalWrite(Relay_1_Engine, HIGH);      //-------------------
      delay(4000);                             

      client.send("VERACRUZ : Break Pedal On");
      digitalWrite(Relay_2_BreakPedal, LOW); // 브레이크 밟고
      delay(500);

      client.send("VERACRUZ : Start btn 3");
      digitalWrite(Relay_1_Engine, LOW);     //-------------------
      delay(300);                             // 시동버튼 누름 = 엔진시동 ㄱㄱ
      digitalWrite(Relay_1_Engine, HIGH);      //-------------------
      delay(1000);
    
      client.send("VERACRUZ : Break Pedal Off");
      digitalWrite(Relay_2_BreakPedal, HIGH);  // 브레이크 떼고

      client.send("VERACRUZ : ### Start Engine!! ###");
      digitalWrite(LED_BUILTIN, LOW);
      
    } else if(digitalRead(LED_BUILTIN) == LOW){
        client.send("VERACRUZ : engine running");
        // 켜져 있으면 클라이언트에게 엔진시동 명령어 브로드캐스트 하게...
    }
}
void Engine_Stop(){
  if(digitalRead(LED_BUILTIN) == LOW){
  
    delay(500);
    client.send("VERACRUZ : Stop btn 1");
    digitalWrite(Relay_1_Engine, LOW);     //-------------------
    delay(300);                             // 시동버튼 눌러 엔진정지
    digitalWrite(Relay_1_Engine, HIGH);      //------------------- 

    client.send("VERACRUZ : ### Stop Engine!! ###");
    digitalWrite(LED_BUILTIN, HIGH);
    SmartKey_Stop();
    
  } else if(!digitalRead(LED_BUILTIN) == LOW){
        client.send("VERACRUZ : engine not running");
  }
}
