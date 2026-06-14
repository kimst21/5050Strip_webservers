#include <WiFi.h>

// ================================
// Wi-Fi 설정
// ================================
const char* ssid     = "-----";
const char* password = "-----";

// 웹 서버 포트
WiFiServer server(80);

// ================================
// RGB LED 핀 설정
// ================================
const int redPin   = 41;
const int greenPin = 42;
const int bluePin  = 39;

// PWM 설정
const int freq = 5000;
const int resolution = 8;   // 0 ~ 255

// HTTP 요청 저장 변수
String header;

// RGB 값 저장
String redString = "0";
String greenString = "0";
String blueString = "0";

int pos1 = 0;
int pos2 = 0;
int pos3 = 0;
int pos4 = 0;

// 접속 제한 시간
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

// ================================
// RGB LED 출력 함수
// ================================
void setColor(int redValue, int greenValue, int blueValue) {
  redValue   = constrain(redValue, 0, 255);
  greenValue = constrain(greenValue, 0, 255);
  blueValue  = constrain(blueValue, 0, 255);

  ledcWrite(redPin, redValue);
  ledcWrite(greenPin, greenValue);
  ledcWrite(bluePin, blueValue);

  Serial.print("RGB = ");
  Serial.print(redValue);
  Serial.print(", ");
  Serial.print(greenValue);
  Serial.print(", ");
  Serial.println(blueValue);
}

void setup() {
  Serial.begin(115200);

  // ================================
  // ESP32 Core 3.x PWM 설정
  // ================================
  ledcAttach(redPin, freq, resolution);
  ledcAttach(greenPin, freq, resolution);
  ledcAttach(bluePin, freq, resolution);

  // 초기 LED OFF
  setColor(0, 0, 0);

  // ================================
  // Wi-Fi 연결
  // ================================
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    currentTime = millis();
    previousTime = currentTime;

    Serial.println("New Client.");

    String currentLine = "";
    header = "";

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();

      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {

            // ================================
            // HTTP 응답 헤더
            // ================================
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // ================================
            // 색상 요청 처리
            // 예: /?r255g0b0&
            // ================================
            if (header.indexOf("GET /?r") >= 0) {
              pos1 = header.indexOf('r');
              pos2 = header.indexOf('g');
              pos3 = header.indexOf('b');
              pos4 = header.indexOf('&');

              redString = header.substring(pos1 + 1, pos2);
              greenString = header.substring(pos2 + 1, pos3);
              blueString = header.substring(pos3 + 1, pos4);

              int redValue = redString.toInt();
              int greenValue = greenString.toInt();
              int blueValue = blueString.toInt();

              setColor(redValue, greenValue, blueValue);
            }

            // ================================
            // HTML 웹 페이지
            // ================================
            client.println("<!DOCTYPE html><html>");
            client.println("<head>");
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\">");
            client.println("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js\"></script>");
            client.println("</head>");

            client.println("<body>");
            client.println("<div class=\"container\">");
            client.println("<h1>All In One KIT Color Picker</h1>");
            client.println("<p>Select RGB LED Color</p>");
            client.println("<a class=\"btn btn-primary btn-lg\" href=\"#\" id=\"change_color\" role=\"button\">Change Color</a>");
            client.println("<br><br>");
            client.println("<input class=\"jscolor {onFineChange:'update(this)'}\" id=\"rgb\">");
            client.println("</div>");

            client.println("<script>");
            client.println("function update(picker) {");
            client.println("var r = Math.round(picker.rgb[0]);");
            client.println("var g = Math.round(picker.rgb[1]);");
            client.println("var b = Math.round(picker.rgb[2]);");
            client.println("document.getElementById('change_color').href='?r' + r + 'g' + g + 'b' + b + '&';");
            client.println("}");
            client.println("</script>");

            client.println("</body></html>");

            client.println();
            break;
          } 
          else {
            currentLine = "";
          }
        } 
        else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
