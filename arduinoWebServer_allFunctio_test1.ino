
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
    var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);\
    connection.onopen = function() {\
       connection.send('Connected');\
    };\
    connection.onerror = function(error) {\
       console.log('WebSocket Error ', error);\
    };\
    connection.onmessage = function(e) {\
       let index_data = e.data.substring(0,10);\
       if(index_data=='VERACRUZ :'){\
            if(e.data=='VERACRUZ : ### Start Engine!! ###' || e.data=='VERACRUZ : Engine_running..=3=3=3'){ \
                 document.getElementById('btn_Engine_state').value='START';\
                 document.getElementById('btn_Engine_state').style.backgroundColor='#088A29';\
                 setTimeout(() => document.getElementById('btn_Engine').disabled = false, 7000);\
            } else if(e.data=='VERACRUZ : ### Stop Engine!! ###' || e.data=='VERACRUZ : engine... at.. rest...'){ \
                 document.getElementById('btn_Engine_state').value='STOP';\
                 document.getElementById('btn_Engine_state').style.backgroundColor='#A4B0B9';\
                 setTimeout(() => document.getElementById('btn_Engine').disabled = false, 7000);\
            }\ 
        }\
       let MsgMoniter = e.data;\
       if(e.data == 'Engine_start/' || e.data == 'Engine__stop/'){\
          document.getElementById('btn_Engine').disabled = true;\
       }\
       document.getElementById('Msg_Moniter').value += MsgMoniter + '\\n';\
       document.getElementById('Msg_Moniter').scrollTop = document.getElementById('Msg_Moniter').scrollHeight;\
    };\
    function send_msg() {\
       if(document.getElementById('btn_Engine_state').value=='STOP'){\
           document.getElementById('btn_Engine').disabled=true;\
           connection.send('Engine_start/');\
       }\
       else if(document.getElementById('btn_Engine_state').value=='START'){\
           document.getElementById('btn_Engine').disabled=true;\
           connection.send('Engine__stop/');\
       }\
    };\
   </script>\
</head>\
<body bgcolor='#356cb1ff'>\
Engine Start & Stop Test<br><br>\
<input id='btn_Engine_state' type='button' value='Hello' style=\"background-color:#FFFFFF;font-size : 20px; width: 80px; height: 70px;\"></button>\
<input id='btn_Engine' type='button' value='START & STOP' onClick='send_msg()' style=\"background-color:#A9D0F5;font-size : 22px; width: 200px; height: 70px;\"><br>\
Message Monitor<br>\
<textarea id='Msg_Moniter' readonly cols='70' rows='12'></textarea><br>\
</body>\
</html>";

void setup() {
    USE_SERIAL.begin(115200);

    //USE_SERIAL.setDebugOutput(true);

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
                                  
            // 현재 상태를 방금 접속한 클라이언트로 전송하고 반영하기 테스트
            // 엔진 시동걸려 있으면 ESP8266 서버 & 클라이언트 둘다 빌트인 LED를 켜놨기 때문에 시동 유무 알 수 있음
            if(digitalRead(LED_BUILTIN) == LOW){
//              webSocket.sendTXT(num, "Engine_start/");
              webSocket.sendTXT(num, "VERACRUZ : Engine_running..=3=3=3");
            } else if(digitalRead(LED_BUILTIN) == HIGH){
//              webSocket.sendTXT(num, "Engine__stop/");
              webSocket.sendTXT(num, "VERACRUZ : engine... at.. rest...");
            }
        }
            break;
            
        case WStype_TEXT:  
        // 메시지 수신부
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

            for(int i=0; i<length; i++){
               state += (char)payload[i];
              // payload length만큼 뽑아내서 저장, length넣으면 입력된 모든글자, 숫자 넣으면 원하는만큼의 길이
              // payload는 바이트 배열임.  사용자가 길이지정가능. 
            }

            // 받은 메시지에 "VERACRUZ :" 문자가 0~10에 포함되어 있으면 실행
            if(state.substring(0,10)=="VERACRUZ :"){ // 서브스트링 0번째~10번째 앞자리까지 읽어서 반영
              webSocket.broadcastTXT(state);
            }
              
//------------↓↓↓↓↓  엔진 온오프 관련 테스트 중  ------------------------------------------------
//-------------------------------------------------------------------------------------
            // led 명령어 들어오면 Led_state에 on off 상태 저장
            if(state == "VERACRUZ : ### Start Engine!! ###" || state == "VERACRUZ : ### Stop Engine!! ###"){
               if (state == "VERACRUZ : ### Start Engine!! ###"){
                 digitalWrite(LED_BUILTIN, LOW);
               }
               else if (state == "VERACRUZ : ### Stop Engine!! ###"){
                 digitalWrite(LED_BUILTIN, HIGH);
               }
            }
            if(state == "Engine_start/" || state == "Engine__stop/"){
               Engine_state = state;
               webSocket.broadcastTXT(Engine_state);
               USE_SERIAL.printf("broadcast Text : ");
               USE_SERIAL.println(Engine_state);
            }
//-----------↑↑↑↑↑=-엔진관련---------------------------------------------------------------------
            //쓰고 비워야 뒤에 계속 붙어나오지 않더라고
            Engine_state = "";
            state = "";
            
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
        // 웹소켓 클라이언트가 작동없는 시간 길어지면 다운되는것 같아서 서버에게 주기적 메시지 전송
        
        USE_SERIAL.printf("server start :  %d Minute\n", counter);
        webSocket.broadcastTXT(server_start_time);
        last_10sec = millis();
    }
    // 여기 루프안에는 delay() 절대 쓰면 안된다고 함.

}
