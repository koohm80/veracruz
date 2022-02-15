
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>

#define USE_SERIAL Serial

#define COMMAND_ENGINE_START    "#/Command_Engine_Start/#"
#define COMMAND_ENGINE_STOP     "#/Command_Engine_Stop/#"
#define PROCESS_START_BUTTON_1  "VERACRUZ : #/Start_Button_1_ACC/#"
#define PROCESS_START_BUTTON_2  "VERACRUZ : #/Start_Button_2_ON/#"
#define PROCESS_START_BUTTON_3  "VERACRUZ : #/Start_Button_3_START/#"
#define PROCESS_STOP_BUTTON_1   "VERACRUZ : #/Start_Button_1_STOP/#"
#define PROCESS_SMARTKEY_ON     "VERACRUZ : #/SmartKey_on/#"
#define PROCESS_SMARTKEY_OFF    "VERACRUZ : #/SmartKey_off/#"
#define PROCESS_BREAKPEDAL_ON   "VERACRUZ : #/Breakpedal_on/#"
#define PROCESS_BREAKPEDAL_OFF  "VERACRUZ : #/Breakpedal_off/#"
#define STATE_ENGINE_STARTED    "VERACRUZ : #/Engine_state_started/#"
#define STATE_ENGINE_STOPED     "VERACRUZ : #/Engine_state_stoped/#"

// 이거는 최대한 안쓰게
//#define STATE_ENGINE_RUNNING    "VERACRUZ : #/Engine_state_running/#"
//#define STATE_ENGINE_REST       "VERACRUZ : #/Engine_state_rest/#"

ESP8266WiFiMulti WiFiMulti;

// 웹서버의 포트는 기본 포트인 80번을 사용한다
ESP8266WebServer server(80);
// 웹서버와 웹클라이언트는 웹소켓에서 81번을 사용한다

//var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
  // 요밑에 웹소켓 주소 원래소스 잠시 보관
  
WebSocketsServer webSocket = WebSocketsServer(81);

String response = "\
<html>\
<head>\
  <meta name=\"viewport\" content=\"width=device-width\">\
  <meta charset=\"UTF-8\">\
  <script>\
    var connection = new WebSocket('ws://219.255.105.186:8181/', ['arduino']);\
    connection.onopen = function() {\
       connection.send('Connected');\
    };\
    connection.onerror = function(error) {\
       console.log('WebSocket Error ', error);\
    };\
    connection.onmessage = function(e) {\
       if(e.data=='VERACRUZ : #/Engine_state_started/#'){ \
            document.getElementById('btn_Engine_state').value='START';\
            document.getElementById('btn_Engine_state').style.backgroundColor='#088A29';\
            setTimeout(() => document.getElementById('btn_Engine').disabled = false, 7000);\
       } else if(e.data=='VERACRUZ : #/Engine_state_stoped/#'){ \
            document.getElementById('btn_Engine_state').value='STOP';\
            document.getElementById('btn_Engine_state').style.backgroundColor='#A4B0B9';\
            setTimeout(() => document.getElementById('btn_Engine').disabled = false, 7000);\
       }\ 
       let MsgMoniter = e.data;\
       if(e.data == '#/Engine_Start/#' || e.data == '#/Engine_Stop/#'){\
          document.getElementById('btn_Engine').disabled = true;\
       }\
       document.getElementById('Msg_Moniter').value += MsgMoniter + '\\n';\
       document.getElementById('Msg_Moniter').scrollTop = document.getElementById('Msg_Moniter').scrollHeight;\
    };\
    function send_msg() {\
       if(document.getElementById('btn_Engine_state').value=='STOP'){\
           document.getElementById('btn_Engine').disabled=true;\
           connection.send('#/Command_Engine_Start/#');\
       }\
       else if(document.getElementById('btn_Engine_state').value=='START'){\
           document.getElementById('btn_Engine').disabled=true;\
           connection.send('#/Command_Engine_Stop/#');\
       }\
    };\
   </script>\
</head>\
<body bgcolor='#1D5D7C'>\
Engine Start & Stop Test<br><br>\
<input id='btn_Engine_state' type='button' value='Hello' style=\"background-color:#FFFFFF;font-size : 20px; width: 80px; height: 70px;\"></button>\
<input id='btn_Engine' type='button' value='START & STOP' onClick='send_msg()' style=\"background-color:#A9D0F5;font-size : 22px; width: 200px; height: 70px;\"><br>\
Message Monitor<br>\
<textarea id='Msg_Moniter' readonly cols='70' rows='12'></textarea><br>\
</body>\
</html>";

void setup() {
    USE_SERIAL.begin(115200);

    USE_SERIAL.setDebugOutput(true);

    pinMode(LED_BUILTIN, OUTPUT); // LED 점등 테스트용
    digitalWrite(LED_BUILTIN, HIGH); // 초기 부팅시 끄기(엔진 시동아닌상태로 인식하게)

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }
    WiFiMulti.addAP("tp_guest", "12345678");  // 공유기 아이디 비번 넣는곳

    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }

    //IP공유기로부터 할당받은IP주소를 여기서 출력한다.
    USE_SERIAL.println("WiFi connected");  
    USE_SERIAL.println("IP address: ");
    USE_SERIAL.println(WiFi.localIP());

    // 웹소켓 서버를 연다
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    // 윈도우10, 안드로이드는 안된다고 함. 의미없지만 안지움
    if(MDNS.begin("esp8266")) {
        USE_SERIAL.println("MDNS responder started");
    }

    // 웹서버의 index.html
    // 웹서버가 클라이언트에게 리스폰스 해주는 부분
    server.on("/", []() {
        // send index.html
        server.send(200, "text/html", response); // 리스폰스 스트링 변수 위에 길게 선언 됨
    });

    server.begin();

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);

//////////핑퐁 test/////////////////////////////////////////////////////////////////////
    webSocket.enableHeartbeat(15000, 7000, 4);  // 핑퐁 하트비트  퐁 안오면 안오는 클라이언트 쳐내버림
    
}

String state = "";
String Engine_state = "";
String veracruz_name = "";

// 웹소켓 이벤트 관련
// 클라이언트에서 서버쪽으로 값이 전송되었을때 뭘할거냐?            ↓바이트 배열이라 함
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", 
                                  num, ip[0], ip[1], ip[2], ip[3], payload);
                                
            webSocket.broadcastTXT("## hello client ##");
            // 현재 상태를 방금 접속한 클라이언트로 전송하고 반영하기
            // 엔진 시동걸려 있으면 ESP8266 서버 & 클라이언트 둘다 빌트인 LED를 켜놨기 때문에 시동 유무 알 수 있음
            if(digitalRead(LED_BUILTIN) == LOW){
              webSocket.sendTXT(num, STATE_ENGINE_STARTED);
            } else if(digitalRead(LED_BUILTIN) == HIGH){
              webSocket.sendTXT(num, STATE_ENGINE_STOPED);
            }
        }
            break;

        case WStype_TEXT:  //● 메시지 수신부 ● 여기서 수신하고 조건에 따라 브로드캐스트 함

            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

            for(int i=0; i<length; i++){
               state += (char)payload[i];
              // payload length만큼 뽑아내서 저장, length넣으면 입력된 모든글자, 숫자 넣으면 원하는만큼의 길이
              // payload는 바이트 배열임.  사용자가 길이지정가능. 
            }

            if(state == "Engine Control : Allow engine control." || state == "Engine Control : Disallow engine control."){
              webSocket.broadcastTXT(state);              
            }
            
            // 받은 메시지에 "VERACRUZ :" 문자가 0~10에 포함되어 있으면 브로드캐스트
            //             "TimerSet :"            
            if(state.substring(0,10)=="VERACRUZ :" || state.substring(0,10) == "TimerSet :"){ // 서브스트링 0번째 ~ x번째 앞자리까지 읽어서 반영
              webSocket.broadcastTXT(state);
            }
            if(state.substring(0,13) == "Timer count :"){
              webSocket.broadcastTXT(state);
            }
            if(state.substring(0,14) == "Low_volt_Set :"){
              webSocket.broadcastTXT(state);
            }
          
//            // 엔진 컨트롤 가능 불가능 상태 확인
//            if(){
//              
//            }

//------------↓↓↓↓↓  엔진 온오프 관련 테스트 중  ------------------------------------------------
//-------------------------------------------------------------------------------------

            // led 명령어 들어오면 Led_state에 on off 상태 저장
            if (state == STATE_ENGINE_STARTED){
               digitalWrite(LED_BUILTIN, LOW);
            }
            else if (state == STATE_ENGINE_STOPED){
              digitalWrite(LED_BUILTIN, HIGH);
            }
            
            if(state == COMMAND_ENGINE_START || state == COMMAND_ENGINE_STOP){
               webSocket.broadcastTXT(state);
               USE_SERIAL.printf("broadcast Text : ");
               USE_SERIAL.println(state);
            }
            
            state = ""; // 비워야 다음 메시지를 제대로 저장함. 안비우면 state += state  됨
             
//-----------↑↑↑↑↑=-엔진관련---------------------------------------------------------------------
            break;
            // 메시지 수신부 끝부분

            case WStype_PING:
            // pong will be send automatically
            Serial.printf("Server : get ping\n");
            break;
            case WStype_PONG:
            // answer to a ping we send
            Serial.printf("Server : get pong\n");
            break;
    }
}

unsigned long last_10sec = 0;
unsigned int counter = 0;
unsigned int minute_counter = 0;
unsigned int hour_counter = 0;
unsigned int day_counter = 0;
String server_start_time="";

void loop() {

    webSocket.loop();
    server.handleClient();
   
    unsigned long t = millis();
    // 
    if((t - last_10sec) > 10 * 1000) { //지정 시간 마다 브로드캐스트 테스트 1000은 1초
        counter++;
         if(counter == 6){
          minute_counter++;
          counter = 0;
          if(minute_counter == 60){
            hour_counter++;
            minute_counter = 0;
            if(day_counter == 60){
              day_counter++;  
              hour_counter = 0;
            }
          }
       }
       // 초만 출력
       if(day_counter == 0 && hour_counter == 0 && minute_counter == 0 && counter >= 0){
          server_start_time = "Server Start : " + (String)counter + "0s";
       }
       // 분 초 출력
       if(day_counter == 0 && hour_counter == 0 && minute_counter > 0 && counter >= 0){
          server_start_time = "Server Start : " + (String)minute_counter + "m " + (String)counter + "0s";
       }
       // 시 분 초 출력
       if(day_counter == 0 && hour_counter > 0 && minute_counter >= 0 && counter >= 0){
          server_start_time = "Server Start : " + (String)hour_counter + "h " + (String)minute_counter + "m " + (String)counter + "0s";
       }
       // 일 시 분 초 출력
       if(day_counter > 0 && hour_counter >= 0 && minute_counter >= 0 && counter >= 0){
          server_start_time = "Server Start : " + (String)day_counter + "d " + (String)hour_counter + "h " + (String)minute_counter+ "m " + (String)counter + "0s";
       }
       // 달 년은 없음 귀찮...
     
       // 웹소켓 클라이언트가 작동없는 시간 길어지면 연결해제 되는 것 같아서 클라이언트에게 주기적 메시지 전송
       USE_SERIAL.println(server_start_time);
       webSocket.broadcastTXT(server_start_time);
       last_10sec = millis();
    }

    // 여기 루프안에는 delay() 절대 쓰면 안된다고 함.

}
