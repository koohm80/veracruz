

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

//#define USE_SERIAL Serial1   // Serial 1?

#define Relay_1_Engine 16     // 엔진작동 상태 보드 16번 릴레이 연결 온오프 체크
#define Relay_2_BreakPedal 4  // 브레이크 상태 보드  4번         "
#define Relay_3_Smartkey 5    // 스마트키 상태 보드  5번         "

int Relay_4_Engine_state = 0;

#define COMMAND_ENGINE_START    "#/Command_Engine_Start/#"
#define COMMAND_ENGINE_STOP     "#/Command_Engine_Stop/#"
//#define COMMAND_LOW_VOLTAGE     "#/Command_Low_voltage_Engine_Start/#"
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

bool Driver_Mode = false;               // 운전자 드라이빙 모드. 기본 false. 어떻게 상태 변화를 줄 것인가? 생각 중. CAN 통신...
                                        //주행 중 예상치 못한 엔진 스타트,스탑 릴레이 제어가들어가면 안 됨.
                                        // 자동차 자체가 시동버튼 빠르게 누르거나 2초간 누르고 있지 않는 이상은 시동 안거진다고 하는데..
                                        // LOLIN 보드는 릴레이 제어를 중지한다... 이렇게 테스트 중.

                                        
                                        // 현재 롤린으로 제어 하는 것 :  스타트 - 앱, 저전압
                                        //                             스톱 - 앱, 타이머
bool Lolin_D1_mini_Mode = false;        // 롤린 보드, 앱이 제어 하는 것


bool Engine_Control_Possible = false;   // 엔진제어 가능, 불가능, 이거는 보드 작동 중 상황에 대한 것

bool Engine_state_starting = false;     // 엔진시동 절차 중 배터리 전압떨어질때 등 저전압 시동이 또 개입할까봐 잠시 차단

String Engine_Stop_Timer_state = "on";  // 타이머 상태
unsigned int Engine_Stop_Timer = 10;    // 분단위 시동 후 엔진정지 시간 설정
unsigned int minute_timer_counter = 0;  //엔진 스톱타이머 용 

float Voltage =           12.8;         // 배터리 전압 센싱 . 부팅 시 12.6으로 자동 시작하지 않도록. 
float Bat_Volt_Low =      12.17;        // 배터리 low 기준 정하는 곳 12.2 내외면 안될려나? . 대략 12.10v 이면 50%라고 함.
float Eng_Starting_volt = 13.7;         // 시동걸려서 발전기 전압 일때 명령 및 저전압 시동 안받기 위해.

String Get_Text = "";                   // 메시지 수신시 저장



void setup() {
  
//	USE_SERIAL.begin(115200);
  Serial.begin(115200);

	Serial.setDebugOutput(true);
//	USE_SERIAL.setDebugOutput(true);

	Serial.println();
	Serial.println();
	Serial.println();

	for(uint8_t t = 4; t > 0; t--) {
		Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
		Serial.flush();
		delay(1000);
	}
/////////////////////////////////////////////////////////////////
//	WiFiMulti.addAP("hodol", "600589rr");
//
//	//WiFi.disconnect();
//	while(WiFiMulti.run() != WL_CONNECTED) {
//		delay(100);
//	}

  WiFi.mode(WIFI_STA);
  WiFi.begin("hodol", "600589rr");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

///////////////////////////////////////////////////

	// server address, port and URL
	webSocket.begin("219.255.105.186", 8181, "/");

	// event handler
	webSocket.onEvent(webSocketEvent);

  /////////////////////////////////////////////////////////////////////////
  pinMode(LED_BUILTIN, OUTPUT); // LED 점등 테스트용
  pinMode(Relay_1_Engine, OUTPUT); // 릴레이 시동버튼 제어 출력    
  pinMode(Relay_2_BreakPedal, OUTPUT); // 릴레이 시동버튼 제어 출력
  pinMode(Relay_3_Smartkey, OUTPUT); // 릴레이 스마트키 제어 출력

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(Relay_1_Engine, HIGH);       // 릴레이 꺼짐으로 초기화
  digitalWrite(Relay_2_BreakPedal, HIGH);
  digitalWrite(Relay_3_Smartkey, HIGH);
/////////////////////////////////////////////////////////////////////////  

	// HTTP 기본 인증 사용 필요하지 않은 경우 선택적 제거
	// webSocket.setAuthorization("user", "Password");

	
	webSocket.setReconnectInterval(10000);      // try ever 5000 again if connection has failed
  
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  
  webSocket.enableHeartbeat(30000, 10000, 3);

}


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    //--------------------------------------------------------------------------------
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    //---------------------------------------------------------------------------------  
    case WStype_CONNECTED: {    // 접속 성공 시에 할 것들
      Serial.printf("[VERACRUZ] Connected to url: %s\n", payload);
      
      webSocket.sendTXT("VERACRUZ : Hello Server. I'm ESP8266 Connecetd!");
      webSocket.sendTXT(COMMAND_ENGINE_STOP);
      
      String tmp1 = "Timerset : " + (String)Engine_Stop_Timer;
      webSocket.sendTXT(tmp1); 
      
      if(!Engine_Control_Possible){
         webSocket.sendTXT("VERACRUZ : Engine Control Possible = False");
      } else if(Engine_Control_Possible) {
         webSocket.sendTXT("VERACRUZ : Engine Control Possible = True");
        }
    }
      break;
    //---------------------------------------------------------------------------------  
    case WStype_TEXT:   // ● 메시지 받는 곳
      Serial.printf("[WSc] get text: %s\n", payload);

      for(int i=0; i<length; i++){                            // payload length만큼 뽑아내서 저장, length넣으면 입력된 모든글자, 숫자 넣으면 원하는만큼의 길이
         Get_Text += (char)payload[i];                           // payload는 바이트 배열임.  사용자가 길이지정가능. 
      }

      // 엔진 온오프 제어 가능하게 할지 여부 설정
      if(Get_Text == "Engine Control : Allow engine control." || Get_Text == "Engine Control : Disallow engine control."){
         if(Get_Text == "Engine Control : Allow engine control."){                
             Engine_Control_Possible = true;
             webSocket.sendTXT("VERACRUZ : Engine Control Possible = True");         //  컨트롤 가능 여부 지정
             
         } else if(Get_Text == "Engine Control : Disallow engine control."){
             Engine_Control_Possible = false;         
             webSocket.sendTXT("VERACRUZ : Engine Control Possible = False");
         }
      }

      // 엔진 스타트 명령 수신!
      if( Driver_Mode || !Engine_Control_Possible && Get_Text == COMMAND_ENGINE_START){   
         webSocket.sendTXT("VERACRUZ : No engine control. Make it controllable");

       // 엔진 컨트롤 가능상태이고, 시동 명령 수신시
      } else if( !Driver_Mode && Engine_Control_Possible && Get_Text == COMMAND_ENGINE_START){  
        
        if(Voltage < Eng_Starting_volt){            // 현재 전압이 발전기 작동 전압(13.7v로 셋팅)보다 작으면
          Engine_state_starting = true;             // 시동 절차 동작 중 시동모터 동작하며 순간적 전압 다운 일때 다시 저전압 시동 절차 방지 테스트
          Lolin_D1_mini_Mode = true;
                                                    // 엔진 스탑 함수 끝에 false(저전압시동 가능) 있음
          Engine_Start();                           // 엔진시동 함수 호출  
          
        } else if(Voltage > Eng_Starting_volt){     // 센싱 전압이 발전기 작동 전압(13.7v로 셋팅)보다 크면(시동걸리면 14v 내외) 엔진 동작 중이니 안걸어 줌
                                                    // IBS센서(배터리)는  배터리 만충시 발전기 멈추고 배터리 전압 꺼내써서 12.6v 내외가 됨.
                                                    // 이때 어떻게 할지 아직... 
          webSocket.sendTXT("VERACRUZ : # The user starts the engine. No app control");
        }
      }
      
      // 엔진 스톱 명령 수신!!
      if(Driver_Mode || !Engine_Control_Possible && Get_Text == COMMAND_ENGINE_STOP){
         webSocket.sendTXT("VERACRUZ : No engine control. Make it controllable");
         
      } else if(!Driver_Mode && Engine_Control_Possible && Get_Text == COMMAND_ENGINE_STOP){           // 엔진 컨트롤 가능상태이고, 스탑 명령 수신시
         if(Lolin_D1_mini_Mode){    // 롤린 제어로 엔진 스타트했을 경우 true, true이면 스톱 가능.
            Engine_Stop();                                                                // 엔진스탑 함수 호출 
            Lolin_D1_mini_Mode = false;
         }
      }
                                                                        
      if (Get_Text == "## hello client ##"){                                           // 스마트폰 앱 접속하면 넘겨줄 정보
         String state_msg1 = "Timer count : " + (String)Engine_Stop_Timer;             // 저장 된 타이머 시간
         webSocket.sendTXT(state_msg1);
         String state_msg2 = "VERACRUZ : Low_volt_Set Reset = " + (String)Bat_Volt_Low; // 저장 된 low volt
         webSocket.sendTXT(state_msg2);
         if(digitalRead(LED_BUILTIN) == LOW){
            webSocket.sendTXT(STATE_ENGINE_STARTED);
         }else if(digitalRead(LED_BUILTIN) == HIGH){
            webSocket.sendTXT(STATE_ENGINE_STOPED);
         }
         if(!Engine_Control_Possible){
            webSocket.sendTXT("VERACRUZ : Engine Control Possible = False");
         } else if(Engine_Control_Possible) {
            webSocket.sendTXT("VERACRUZ : Engine Control Possible = True");
         }
         if(Driver_Mode){
            webSocket.sendTXT("VERACRUZ : ■ Driver_Mode ### Can not remote control ■  ");
         }
      }

      // 사용자(스마트폰)가 지정하는 저전압시동 작동 전압 설정
      if (Get_Text.substring(0,14) == "Low_volt_Set :"){
        Bat_Volt_Low = Get_Text.substring(15).toFloat();
        String low_volt_set_msg = "VERACRUZ : Low_volt_Set Reset = " + (String)Bat_Volt_Low;
        webSocket.sendTXT(low_volt_set_msg);
      }
      
      // 사용자가 지정하는 타이머 변수 가져와서 저장하는 곳
      if(Get_Text.substring(0,10) == "TimerSet :"){               // 11번째 문자부터 끝까지 읽어옴 substring() 참조 
        minute_timer_counter = 0;
        Engine_Stop_Timer = Get_Text.substring(11).toInt();         // 기본 10분 후 엔진정지
        String timer_reset_msg = "VERACRUZ : Timer Reset. " + (String)Engine_Stop_Timer + " minute";
        webSocket.sendTXT(timer_reset_msg);
      }
      
      Get_Text = "";  // 이거 필수. 비워줘야 잘 받아들인다.
      break;
    //---------------------------------------------------------------------------------  
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);

      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
        case WStype_PING:                                // 핑! 살아있니? 핑 보내고
            // pong will be send automatically
            Serial.printf("[WSc] get ping\n");
            break;
        case WStype_PONG:
            // answer to a ping we send                  // 퐁~ 살아있다~ 퐁을 받는다
            Serial.printf("[WSc] get pong\n");
            break;
    }

}


// loop 안에서 사용 될 변수선언. 복잡다 정리필요.

unsigned long volt_last_sec = 0;
unsigned int  volt_count = 0;

unsigned long low_volt_last_sec = 0;
unsigned int  low_volt_count = 0;

// 엔진 시동 후 경과시간 
unsigned long last_sec = 0;
unsigned int vera_counter = 0;
unsigned int  vera_minute = 0;

// 부팅 후 경과시간
unsigned long Boot_last_sec = 0;
unsigned int  Boot_Sec = 0;
unsigned int  Boot_10Sec = 0;
// 분, 시, 일
unsigned int  Boot_minute = 0;
unsigned int  Boot_hour = 0;
unsigned int  Boot_day = 0;

String        Start_Engine_seconds = "";
String        Alive_time = "";

float Verazruz_Voltage = 0;
bool low_Volt_start_possible = false;


void loop() {   // 부팅 후 setup 1회 실행 후  loop 무한반복
  

//    if(!Driver_Mode && Voltage > Eng_Starting_volt){            // 드라이버 모드(운전자 직접조작)가 거짓 인데 현재 전압이 13.7보다 높으면 운전자가 직접 시동 한것으로 판단
//  
//          Driver_Mode = true;
//      
//    } else if(Driver_Mode && Voltage < Eng_Starting_volt){      // 드라이버 모드가 참 인데 현재 전압이 13.7보다 낮으면 운전자가 직접 스톱 한것으로 판단
//          
//          Driver_Mode = false;     
//    }
    
	
    webSocket.loop();
    

    // 엔진 시동 후 경과시간
    unsigned long t = millis();
    if((t - last_sec) > 10 * 1000 && digitalRead(LED_BUILTIN) == LOW) { 
      
        Start_Engine_seconds = "";                                   // 스트링 비우기
        vera_counter++;
        if(vera_counter == 6){
            vera_minute++;
            minute_timer_counter++;                                    //엔진스톱 타이머 +1분
            vera_counter = 0;
        }
        if(vera_minute == 0 && vera_counter >= 0){ // 초만 출력
            Start_Engine_seconds = "VERACRUZ : Engine running for " + (String)vera_counter + "0s";
        }
        if(vera_minute > 0 && vera_counter >= 0){
            Start_Engine_seconds = "VERACRUZ : Engine running for " + (String)vera_minute + "m" + (String)vera_counter + "0s";
        }
        webSocket.sendTXT(Start_Engine_seconds);
        
        last_sec = millis();
        
    } else if(digitalRead(LED_BUILTIN) == HIGH){
        minute_timer_counter = 0;
        vera_counter = 0;
        vera_minute = 0;
        
    }
    

    // 클라이언트 부팅 후 경과시간
    unsigned long Boot_time = millis();
    if((Boot_time - Boot_last_sec) > 1000 ){ // 1000 = 1초 시간 지정
       Boot_Sec++;
       if(Boot_Sec == 10){
        Boot_10Sec++;
        Boot_Sec = 0;
        if(Boot_10Sec == 6){
           Boot_minute++;
           Boot_10Sec = 0;
           if(Boot_minute == 60){
             Boot_hour++;
             Boot_minute = 0;
             if(Boot_hour == 24){
               Boot_day++;  
               Boot_hour = 0;
             }
           }
        }
       
       if(Boot_day == 0 && Boot_hour == 0 && Boot_minute == 0 && Boot_10Sec >= 0){ // 초만 출력
          Alive_time = "VERACRUZ : I am alive, " + (String)Boot_10Sec + "0s";
       }
       if(Boot_day == 0 && Boot_hour == 0 && Boot_minute > 0 && Boot_10Sec >= 0){  // 분 초 출력
          Alive_time = "VERACRUZ : I am alive, " + (String)Boot_minute + "m " + (String)Boot_10Sec + "0s";
       }
       if(Boot_day == 0 && Boot_hour > 0 && Boot_minute >= 0 && Boot_10Sec >= 0){  // 시 분 초 출력
          Alive_time = "VERACRUZ : I am alive, " + (String)Boot_hour+ "h " + (String)Boot_minute+ "m " + (String)Boot_10Sec + "0s";
       }
       if(Boot_day > 0 && Boot_hour >= 0 && Boot_minute >= 0 && Boot_10Sec >= 0){  // 일 시 분 초 출력
          Alive_time = "VERACRUZ : I am alive, " + (String)Boot_day+ "d " + (String)Boot_hour+ "h " + (String)Boot_minute+ "m " + (String)Boot_10Sec + "0s";
       } // 달 년은 없음 귀찮...
       
       webSocket.sendTXT(Alive_time);  //서버로 전달하면 알아서 뿌려줌
       Alive_time = "";                //보낸 후 스트링 비우기
      }
        ///////////////////////////////////////////////////////////////
        // 엔진 스톱 타이머, 위의 타이머 속에 속해있음
        // 드라이버 모드가 아니고 && 조건2
        if(!Driver_Mode && Engine_Stop_Timer_state == "on"){
           if(minute_timer_counter >= Engine_Stop_Timer){
               String timer_msg = "VERACRUZ : Timer start after " + (String)Engine_Stop_Timer + "minutes ,  Engine Stop!! ";
               webSocket.sendTXT(timer_msg);

               Engine_Stop();
       // ★★ 엔진 스톱 함수 포함 ★★
               
           }
        }
        ///////////////// 타이머 //////////////////////////////////////

       Boot_last_sec = millis();

    }


    // 전압 1초에 100번 측정하고 평균 내기
    unsigned long volt_t = millis();
    if((volt_t - volt_last_sec) >= 10){
      Verazruz_Voltage += analogRead(A0);
      volt_count++;
      
      if(volt_count >= 100){
          Voltage = (Verazruz_Voltage / 100) * 14.5 / 1024.0;    // 보드 측정 전압의 변화가 커서 1초에 100번 측정하고 평균값 산출
          if(Voltage > Bat_Volt_Low){
              String volt = "VERACRUZ : Volt = " + (String)Voltage;
              webSocket.sendTXT(volt);
          } else if(Voltage < Bat_Volt_Low){
              String volt = "VERACRUZ : Volt = " + (String)Voltage + ", Low Voltage!!";
              webSocket.sendTXT(volt);
          }          
          
          volt_count = 0;
          Verazruz_Voltage = 0;
      }
      volt_last_sec = millis();
    }

    // 저전압 확인 후 시간 지연. 운전자가 차량 직접시동 중 전압 강하 대비 1분 지연 후 다시 저전압 조건 확인.

    //★★  22.02.14.13:09 현재 확인 중.  시간지연 되는지 확인 중
    
    if(Voltage < Bat_Volt_Low){
       unsigned long low_volt_t = millis();
       if((low_volt_t - low_volt_last_sec) >= 10 * 1000){
    
          low_volt_count++;
          
          if(low_volt_count >= 6){
            
             Verazruz_Voltage_low_start();        // 저전압 시동 함수 호출
             
             low_volt_count = 0;
          }
          low_volt_last_sec = millis();
       }
     }
        
    
}  //loop 끝 부분 임.

/////////////////// 이하 동작 함수 ///////////////////////

//   저전압 스타트 함수. 
void Verazruz_Voltage_low_start(){                             
     
      // 운전자가 스타트버튼 눌러서 시동 절차 중 전압 다운될때를 확인하고 저전압 차단 구현해야 함. ★ 22.02.13 22:54 부터 생각 중
      // 앱으로 시동 중 다운될때와는 다름(제어불가 셋팅 함)
   if(!Driver_Mode && Voltage < Bat_Volt_Low && !Engine_state_starting && digitalRead(LED_BUILTIN) == HIGH && Boot_minute >= 2){  // 전압이 낮고 엔진이 꺼져 있으면. 실행
    
//       if(low_Volt_start_possible){    // 저전압 스타트 가능상태인지 확인 true면 저전압 시동 시작
        
          String Volt_msg = "VERACRUZ : WARNING. Battery LOW( " + (String)Voltage + "v )= Start engine";
          webSocket.sendTXT(Volt_msg); 
          
          Engine_Start();                  //  저전압 시동 실시 
          // ★★ 엔진 스타트 함수 포함 ★★
//       }
       
       Engine_state_starting = true;          // 앱 엔진시동 절차 동작 중 시동모터 동작하며 순간적 전압 다운 일때 저전압으로 인한 시동 절차 중복 방지 테스트
                                              // 엔진 스탑 함수 끝에 false(저전압시동 가능) 있음
   }
   
//   low_Volt_start_possible = false;   // 저전압 시동 절차 불가설정
}




void SmartKey_Start(){
      webSocket.sendTXT(PROCESS_SMARTKEY_ON);
      digitalWrite(Relay_3_Smartkey, LOW);     // 스마트키 배터리 릴레이 연결
}
void SmartKey_Stop(){
//      delay(300);       
      webSocket.sendTXT(PROCESS_SMARTKEY_OFF);
      digitalWrite(Relay_3_Smartkey, HIGH);     // 스마트키 배터리 릴레이 해제
}




// ★★ 엔진 스타트 함수 ★★
void Engine_Start(){

 // 추가할 것
 // 딜레이함수 말고 다른방법 있으면 좋으려나? for 문 이용 생각해봐
 // for문 만들긴 했는데....
    
    if(!digitalRead(LED_BUILTIN) == LOW){

      
      
//////////////////////////////////// for문 이용 시간 지연 테스트 //////////////////////////////////////////////////////////////
      int i=1;
      unsigned long before_time = millis();
 
      SmartKey_Start();
            
      for(   ;   ;  ){    // 무한루프 for문으로 시간지연 및 시동절차 테스트, // while(1){ <- for문 대신 이거 써도 됨
                          // 딜레이 주는 것보다 안정적인지 테스트 해봐야...
        unsigned long now_time = millis();
        if(i == 1 && (now_time - before_time) > 1500){
          webSocket.sendTXT(PROCESS_START_BUTTON_1);
          digitalWrite(Relay_1_Engine, LOW);                      // 시동버튼 1회 누르고
          i++;
          delay(10);
          before_time = millis();
        } else if(i == 2 && (now_time - before_time) > 300){
          digitalWrite(Relay_1_Engine, HIGH);                     // 시동버튼 1회 떼고
          i++;
          delay(10);
          before_time = millis();
        } else if(i == 3 && (now_time - before_time) > 1500){     //  3000 지연하면 재부팅 됨.  i = 3 ~ 4  나눠서 지연시키기
          i++;
          delay(10);
          before_time = millis();
        } else if(i == 4 && (now_time - before_time) > 1500){
          webSocket.sendTXT(PROCESS_START_BUTTON_2);       
          digitalWrite(Relay_1_Engine, LOW);                      // 시동버튼 2회 누르고
          i++;
          delay(10);
          before_time = millis();
        } else if(i == 5 && (now_time - before_time) > 300){
          digitalWrite(Relay_1_Engine, HIGH);                     // 시동버튼 2회 떼고
          i++;
          delay(10);
          before_time = millis();
        } else if(i == 6 && (now_time - before_time) > 1500){     // 4000 지연하면 재부팅 됨.  i = 6 ~ 8 3개로 나눠서 지연
          i++;
          delay(10);
          before_time = millis();
        } else if(i == 7 && (now_time - before_time) > 1500){
          i++;
          delay(10);
          before_time = millis();
        } else if(i == 8 && (now_time - before_time) > 1000){
          webSocket.sendTXT(PROCESS_BREAKPEDAL_ON);               // 브레이크 밟고. 릴레이로 밟는 시늉
          digitalWrite(Relay_2_BreakPedal, LOW); 
          i++;
          delay(10);
          before_time = millis();
        } else if(i == 9 && (now_time - before_time) > 1000){
          webSocket.sendTXT(PROCESS_START_BUTTON_3);              // 시동버튼  3회 누르고 = 이때 엔진시동 걸리겠지?
          digitalWrite(Relay_1_Engine, LOW);
          i++;
          delay(10);
          before_time = millis();
        } else if(i == 10 && (now_time - before_time) > 300){ 
          digitalWrite(Relay_1_Engine, HIGH);                     // 시동버튼  3회 떼고. 시동 걸리는 중이거나 걸렸거나 
          i++;
          delay(10);
          before_time = millis();
        } else if(i == 11 && (now_time - before_time) > 1000){ 
          webSocket.sendTXT(PROCESS_BREAKPEDAL_OFF);              // 시동 걸렸으니 브레이크 떼고 
          digitalWrite(Relay_2_BreakPedal, HIGH);   
          i=1;   // 절차 초기화
          break;
        } 
      } // 무한반복 끝

      digitalWrite(LED_BUILTIN, LOW);
      if(digitalRead(LED_BUILTIN) == LOW){
         webSocket.sendTXT(STATE_ENGINE_STARTED);      // 서버야 나 엔진시동 절차 완료 했어
      } else if(digitalRead(LED_BUILTIN) == LOW){
          webSocket.sendTXT(STATE_ENGINE_STARTED);// 이거보내면 서버가 started 브로드캐스트.  스마트폰 앱은 시동완료 상태로 만듬
      }
    }
//////////////////////////////////// 위는 for문 이용 시간 지연 테스트 //////////////////////////////////////////////////////////////
   // 엔진 스타트 함수 끝.
}


// ★★ 엔진 스톱 함수 ★★
void Engine_Stop(){
  if(digitalRead(LED_BUILTIN) == LOW){
    webSocket.sendTXT(PROCESS_STOP_BUTTON_1);           // 시동버튼 눌러 엔진정지
    digitalWrite(Relay_1_Engine, LOW);

    delay(300);
    digitalWrite(Relay_1_Engine, HIGH);
    webSocket.sendTXT(STATE_ENGINE_STOPED);
    digitalWrite(LED_BUILTIN, HIGH);
    
    SmartKey_Stop();
    
  } else if(!digitalRead(LED_BUILTIN) == LOW){
        webSocket.sendTXT(STATE_ENGINE_STOPED);
  }
  minute_timer_counter = 0; // 스톱 타이머 리셋
  vera_counter= 0;

  Engine_state_starting = false; // 이제 저전압 시동 가능

  low_volt_last_sec = 0;
}



// 끝.
