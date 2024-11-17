#include <WiFi.h>

// 여기 와이파이 자격 증명...
const char* ssid     = "--------";
const char* password = "--------";

// 웹 서버 포트 번호를 80으로 설정
WiFiServer server(80);

// HTTP GET 값 디코딩
String redString = "0";
String greenString = "0";
String blueString = "0";
int pos1 = 0;
int pos2 = 0;
int pos3 = 0;
int pos4 = 0;

// HTTP 요청을 저장할 변수
String header;

// PWM제어를 위한 빨강, 초록, 파랑 핀
const int redPin =   41;     
const int greenPin = 42;     
const int bluePin =  17;     

// PWM 주파수, 채널 및 비트 분해능 설정
const int freq = 5000;
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;
// 비트 해상도 256
const int resolution = 8;

// 현재시간
unsigned long currentTime = millis();
// 이전 시간
unsigned long previousTime = 0; 
// 제한 시간(밀리초)을 정의합니다(example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  // LED PWM 기능 라이트 구성
  ledcSetup(redChannel, freq, resolution);
  ledcSetup(greenChannel, freq, resolution);
  ledcSetup(blueChannel, freq, resolution);
  
  // 채널을 제어할 GPIO에 연결합니다
  ledcAttachPin(redPin, redChannel);
  ledcAttachPin(greenPin, greenChannel);
  ledcAttachPin(bluePin, blueChannel);
  
  // SSID와 비밀번호로 Wi-Fi 네트워크에 연결
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // 로컬 IP 주소를 인쇄하고 웹 서버를 시작합니다
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // 들어오는 클라이언트 듣기

  if (client) {                             // 새로운 클라이언트가 연결되면,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // 메시지를 시리얼 포트에 인쇄합니다
    String currentLine = "";                // 클라이언트에서 들어오는 데이터를 유지하기 위한 문자열 만들기
    while (client.connected() && currentTime - previousTime <= timeoutTime) {            // 클라이언트가 연결되어 있는 동안 루프
      currentTime = millis();
      if (client.available()) {             // 고객으로부터 읽을 바이트가 있다면,
        char c = client.read();             // 바이트를 읽습니다
        Serial.write(c);                    // 시리얼 모니터를 출력합니다
        header += c;
        if (c == '\n') { // 바이트가 새 줄 문자인 경우,현재 줄이 비어 있으면 두 줄의 새 줄 문자가 연속으로 표시됩니다
                         // 클라이언트 HTTP 요청은 여기까지이므로 응답을 보냅니다:
          if (currentLine.length() == 0) {
            // HTTP 헤더는 항상 응답 코드(예: HTTP/1.1200 OK)로 시작합니다
            // 그리고 콘텐츠 유형을 지정하여 고객이 무엇이 올지 알 수 있도록 하고 빈 줄로 표시합니다:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
                   
            // HTML 웹 페이지 표시
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\">");
            client.println("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js\"></script>");
            client.println("</head><body><div class=\"container\"><div class=\"row\"><h1>All In One KIT Color Picker</h1></div>");
            client.println("<a class=\"btn btn-primary btn-lg\" href=\"#\" id=\"change_color\" role=\"button\">Change Color</a> ");
            client.println("<input class=\"jscolor {onFineChange:'update(this)'}\" id=\"rgb\"></div>");
            client.println("<script>function update(picker) {document.getElementById('rgb').innerHTML = Math.round(picker.rgb[0]) + ', ' +  Math.round(picker.rgb[1]) + ', ' + Math.round(picker.rgb[2]);");
            client.println("document.getElementById(\"change_color\").href=\"?r\" + Math.round(picker.rgb[0]) + \"g\" +  Math.round(picker.rgb[1]) + \"b\" + Math.round(picker.rgb[2]) + \"&\";}</script></body></html>");
            // HTTP 응답은 다른 빈 줄로 끝납니다
            client.println();

            // 요청 샘플 : /?r201g32b255&
            // 레드 = 201 | 그린 = 32 | 블루 = 255
            if(header.indexOf("GET /?r") >= 0) {
              pos1 = header.indexOf('r');
              pos2 = header.indexOf('g');
              pos3 = header.indexOf('b');
              pos4 = header.indexOf('&');
              redString = header.substring(pos1+1, pos2);
              greenString = header.substring(pos2+1, pos3);
              blueString = header.substring(pos3+1, pos4);
              
              ledcWrite(redChannel, redString.toInt());
              ledcWrite(greenChannel, greenString.toInt());
              ledcWrite(blueChannel, blueString.toInt());
            }
            // 도중에 루프를 해제합니다
            break;
          } else { // 새 줄이 있으면 현재 줄을 지웁니다
            currentLine = "";
          }
        } else if (c != '\r') {  // 만약 당신이 마차 복귀 캐릭터 외에 다른 것이 있다면,
          currentLine += c;      // 현재 라인의 끝에 추가합니다
        }
      }
    }
    // 헤더 변수 지우기
    header = "";
    // 연결 닫기
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
