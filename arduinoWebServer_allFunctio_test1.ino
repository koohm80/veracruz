
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>

#define USE_SERIAL Serial

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
       let first_char = e.data.charAt(0);\
       if(first_char==1)document.getElementById('cb').checked = true;\       
       if(first_char==0)document.getElementById('cb').checked = false;\
       let index_data = e.data.substring(0,13);\
       if(index_data=='Engine_start/')document.getElementById('cb_Engine').checked = true;\
       if(index_data=='Engine__stop/')document.getElementById('cb_Engine').checked = false;\
       let MsgMoniter = e.data;\
       document.getElementById('Msg_Moniter').value += MsgMoniter + '\\n';\
       document.getElementById('Msg_Moniter').scrollTop = document.getElementById('Msg_Moniter').scrollHeight;\
    };\
    function send_msg() {\
       if(document.getElementById('cb').checked){\
           connection.send('1');\
       }\
       else {\
           connection.send('0');\
       }\
    };\
    function send_msg2() {\
       if(document.getElementById('cb_Engine').checked){\
           connection.send('Engine_start/');\
       }\
       else {\
           connection.send('Engine__stop/');\
       }\
    };\
   </script>\
</head>\
<body>\
체크 Engine_start/<br>\
해제 Engine__stop/<br>\
<input id=\"cb_Engine\" size=25 type=\"checkbox\" onChange=\"send_msg2()\"><br>\
시리얼 모니터<br>\
<textarea id=\"Msg_Moniter\" readonly cols='70' rows='12'></textarea><br>\
</body>\
</html>";


void setup() {
    USE_SERIAL.begin(115200);

    //USE_SERIAL.setDebugOutput(true);

    pinMode(LED_BUILTIN, OUTPUT); // LED 점등 테스트용

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 5; t > 0; t--) {
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

}

String state = "";
String Engine_state = "";
String veracruz_name = "";

// 웹소켓 이벤트 관련
// 클라이언트에서 서버쪽으로 값이 전송되었을때 뭘할거냐?            ↓바이트 배열이라 함
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    String State_test = "";
  
    //웹소켓이벤트의 타입 
    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", 
                                  num, ip[0], ip[1], ip[2], ip[3], payload);
                                  
            // 현재 상태를 방금 접속한 클라이언트로 전송하고 반영하기 테스트
//            USE_SERIAL.printf("[%u]\n client_data : %s\n payload : %s\n", 
//                                num, Engine_state, payload); //내용알아보기 테스트
                                
            //webSocket.sendTXT(num, Engine_state); //현재 안 됨.
            //## 지금 접속하는 클라이언트에게만 현재 상태를 반영하기 위한 메시지 보내기.
            //## 테스트, 스트링은 되는데 받은 메시지가 저장되는 payload 외의 스트링변수는 안되는 듯?
            //## 방법을 찾아라. 
            // https://github.com/Links2004/arduinoWebSockets/blob/master/src/WebSocketsServer.cpp 참조
          
            //##  Serial.println(ip.toString());  //## .toString() 이거 한번 해봐
            //## [출처] NodeMCU(혹 아두이노)를 이용하여 웹소켓 통신하기(1)|작성자 코스모스

          
            
            // send message to client
            // num = 클라이언트 번호
            //webSocket.sendTXT(num, "Connected");
            //uint8_t * payload("내가 전송할 말"); // 모든 클라이언트에게 메시지 전송
        }
            break;
            
        case WStype_TEXT:  
        // 메시지 수신부
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

            
            
//            for(int i=0; i<8; i++){
//               Led_state += (char)payload[i];            
//              // LED 온오프를 위한 스트링내 0~7위치 문자 검출하여 저장
//            }

            for(int i=0; i<length; i++){
               state += (char)payload[i];            
              // payload 길이만큼 뽑아내서 저장 
            }
            
            for(int i=0; i<15; i++){ 
              veracruz_name += (char)state[i];
//              // 스트링내 0~i번째 문자 검출

//----------------------------------------------------------------------------------------
            }
            if(veracruz_name == "VERACRUZ : I am"){ 
              webSocket.broadcastTXT(state);
            }
//-------------------------------------------------------------------------------------
//------------↓↓↓↓↓  엔진 온오프 관련 테스트 중  ------------------------------------------------
//-------------------------------------------------------------------------------------
            // led 명령어 들어오면 Led_state에 on off 상태 저장
            if(state == "Engine_start/" || state == "Engine__stop/"){
               Engine_state = state;
               if (Engine_state == "Engine_start/"){
                 digitalWrite(LED_BUILTIN, LOW);
               }
               else if (Engine_state == "Engine__stop/"){
                 digitalWrite(LED_BUILTIN, HIGH);
               }
               webSocket.broadcastTXT(Engine_state);
//               Engine_state = "Led_state : " + Engine_state + "입니다.";
//               webSocket.broadcastTXT(Engine_state);
               USE_SERIAL.printf("broadcast Text : ");
               USE_SERIAL.println(Engine_state);

            }
//-----------↑↑↑↑↑=-엔진관련---------------------------------------------------------------------
            
//            webSocket.broadcastTXT(state);
                        
//            webSocket.broadcastTXT(payload, length); 
//            USE_SERIAL.printf("payload : %s\n",payload);
            
            // payload는 바이트 배열임.        길이지정가능. 
            // length넣으면 입력된 모든글자, 숫자 넣으면 원하는만큼의 길이


            //쓰고 비워야 뒤에 붙어나오지 않더라고
            Engine_state = "";
            state = "";
            veracruz_name = "";
            
            break;
    }

}



unsigned long last_10sec = 0;
unsigned int counter = 0;
String server_start_time="";
void loop() {

    webSocket.loop();
    server.handleClient();
    
    unsigned long t = millis();
    // 
    if((t - last_10sec) > 10 * 6000) { //지정 시간 마다 브로드캐스트 테스트 1000은 1초
        counter++;
        server_start_time= "Server Start " + (String)counter + " Minute"; 
        // esp8266 클라이언트가 메시지 주고받는 작동이 없는 시간이 길어지면다운되는것 같아서 
        //경과시간 및 지속적 메시지 보내서 경과 지켜보기
        
        USE_SERIAL.printf("server start :  %d Minute\n", counter);
        webSocket.broadcastTXT(server_start_time);
        last_10sec = millis();
        
    }


    // 여기 루프안에는 delay() 절대 쓰면 안된다고 함.

}
