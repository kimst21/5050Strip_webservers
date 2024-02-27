#include <ArduinoJson.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "wifisecrets.h"

// LED 핀의 개수
const int blueLedPin = 2;
const int redLedPin = 0;
const int greenLedPin = 4;

// PWM 속성 설정
const int freq = 5000;
const int blueLedChannel = 10;
const int redLedChannel = 11;
const int greenLedChannel = 12;
const int resolution = 8;

// 불필요하게 변경되지 않도록 현재 값을 추적합니다
byte currentRed = 255;
byte currentGreen = 255;
byte currentBlue = 255;

#define MAXIMUM_PATTERN_COMPONENTS 20

// 여기 와이파이 자격 증명...
const char* ssid = "**********";
const char* password = "**********";

// HTTP에서 실행 중인 웹 서버
WebServer server(80);

// 특정 시간에 색상을 지정하기 위한 클래스(이들은 보간됨)
class ColorAndTime
{
public:
  // 빨간색, 녹색 및 파란색 값
  byte red;
  byte green;
  byte blue;
  // 시간(마이크로초)
  unsigned long time;

public:
  // 생성할 생성자(배열 초기화에 사용 가능)
  ColorAndTime(byte newRed, byte newGreen, byte newBlue, unsigned long newTime)
  {
    red = newRed;
    green = newGreen;
    blue = newBlue;
    time = newTime;
  }

  //초기화자 없이 배열을 지정할 때 사용되는 기본 빈 생성자
  ColorAndTime()
  {
    red = 0;
    green = 0;
    blue = 0;
    time = 0;
  }

};

// 패턴 배열
ColorAndTime pattern[MAXIMUM_PATTERN_COMPONENTS];
size_t statesInPattern;


bool connectToWifi(const char* ssid, const char* password) {
  int waitTime = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // 연결 대기
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    waitTime++;
    if (waitTime > 20)
    {
      WiFi.disconnect();
      return false;
    }

  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  return true;
}

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void handlePattern() {
  Serial.println("In POST /pattern");
  Serial.println(server.args());
  Serial.println(server.argName(0));
  Serial.println(server.arg(0));

  String body = server.arg("plain");

  DynamicJsonDocument doc(ESP.getMaxAllocHeap());  
  auto error = deserializeJson(doc, body);

  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    server.send(400);
    return;
  }

  Serial.println("Deserialized JSON doc");

  JsonArray patternArray = doc.as<JsonArray>();
  if (!patternArray) {
    server.send(400, "text/plain", "Top level JSON object should be an array of pattern values");
    return;
  }

  if ((patternArray.size() < 2) || (patternArray.size() > MAXIMUM_PATTERN_COMPONENTS)) {
    // 작업: MAXIM_PATTN_COMPONTENS(스프린트)를 올바르게 보고하려면 이 오류 메시지를 가져옵니다
    server.send(400, "text/plain", "Number of pattern elements should be between 2 and 20");
    return;
  }

  for (size_t i = 0; i < patternArray.size(); i++)
  {
    // 작업: 개체에서 이러한 구성 요소를 확인합니다
    // 배열의 개체에서 구성 요소를 꺼냅니다
    int r = patternArray[i]["r"];
    int g = patternArray[i]["g"];
    int b = patternArray[i]["b"];
    int t = patternArray[i]["t"];

    pattern[i].red = r;
    pattern[i].green = g;
    pattern[i].blue = b;
    pattern[i].time = t;
  }

  // 어레이에 실제로 있는 구성 요소의 수를 추적하도록 글로벌 설정
  statesInPattern = patternArray.size();

  server.send(200);
}


void initializeDefaultPattern()
{
  // "검은색"에서 "무지개"를 거쳐 "검은색"으로 되돌아오는 패턴을 8초에 걸쳐 만듭니다
  pattern[0] = {0, 0, 0, 0};
  pattern[1] = {0, 0, 0, 1000000L};
  pattern[2] = {255, 0, 0, 2000000L};
  pattern[3] = {255, 255, 0, 3000000L};
  pattern[4] = {0, 255, 0, 4000000L};
  pattern[5] = {0, 255, 255, 5000000L};
  pattern[6] = {0, 0, 255, 6000000L};
  pattern[7] = {255, 0, 255, 7000000L};
  pattern[8] = {0, 0, 0, 8000000L};

  statesInPattern = 9;
}

void setup()
{
  // 디버깅 중에 출력을 확인해야 할 경우를 대비하여 직렬을 시작합니다.
  Serial.begin(115200);

  //이것이 약간 신뢰할 수 없기 때문에 여러 번 연결해 보십시오
  bool connected = false;
  while (!connected)
  {
    connected = connectToWifi(ssid, password);
  }

  // 채널 설정
  ledcSetup(redLedChannel, freq, resolution);
  ledcSetup(greenLedChannel, freq, resolution);
  ledcSetup(blueLedChannel, freq, resolution);

  // 채널에 핀을 부착합니다
  ledcAttachPin(redLedPin, redLedChannel);
  ledcAttachPin(greenLedPin, greenLedChannel);
  ledcAttachPin(blueLedPin, blueLedChannel);

  //초기 상태 작성
  ledcWrite(redLedChannel, currentRed);
  ledcWrite(greenLedChannel, currentGreen);
  ledcWrite(blueLedChannel, currentBlue);

  // 기본 패턴으로 패턴 채우기
  initializeDefaultPattern();

  // 결국 테스트를 위해 REST API를 호출하려면 Javacapt가 포함된 페이지여야 합니다
  server.on("/", handleRoot);
  // 이 페이지는 패턴을 업데이트하기 위해 RESTful JSON POST를 처리합니다
  server.on("/pattern", HTTP_POST, handlePattern);

  // 서버를 시작합니다
  server.begin();
}

void updateColorChannel(byte& currentValue, byte newValue, int channel)
{
  //새 값이 현재 값과 다를 경우에만 업데이트합시다
  if (currentValue != newValue)
  {
    // 현재 값 업데이트
    currentValue = newValue;
    ledcWrite(channel, newValue);
  }
}

void updateColorsWithPattern(unsigned long currentTime, bool invertValues)
{
  //우리가 어느 브래킷에 있는지 파악합니다(어느 두 패턴 섹션 사이에 있는지)
  size_t colorBracketStart = 0;
  for (size_t index = 0; index < (statesInPattern - 1); index++)
  {
    if (currentTime >= (pattern[index].time) && (currentTime < pattern[index + 1].time))
    {
      colorBracketStart = index;
      break;
    }
  }

  // 브래킷의 길이와 브래킷의 거리를 파악합니다
  float bracketLength = float(pattern[colorBracketStart + 1].time - pattern[colorBracketStart].time);
  float timeIntoBracket = float(currentTime - pattern[colorBracketStart].time);

  // 브래킷의 각 끝에 있는 색상의 선형 보간 가중치를 구합니다
  float percentageColorA = 1.0 - (timeIntoBracket / bracketLength);
  float percentageColorB = 1.0 - percentageColorA;

  byte redValue = byte(percentageColorA * float(pattern[colorBracketStart].red) + percentageColorB * float(pattern[colorBracketStart + 1].red));
  byte greenValue = byte(percentageColorA * float(pattern[colorBracketStart].green) + percentageColorB * float(pattern[colorBracketStart + 1].green));
  byte blueValue = byte(percentageColorA * float(pattern[colorBracketStart].blue) + percentageColorB * float(pattern[colorBracketStart + 1].blue));

  // 값을 반전시킬 필요가 있는지 확인합니다(공통 애노드 대 캐소드에 적합)
  if (invertValues) {
    redValue = 255 - redValue;
    greenValue = 255 - greenValue;
    blueValue = 255 - blueValue;
  }

  //잠재적으로 채널 업데이트
  updateColorChannel(currentRed, redValue, redLedChannel);
  updateColorChannel(currentGreen, greenValue, greenLedChannel);
  updateColorChannel(currentBlue, blueValue, blueLedChannel);
}

void loop()
{
  // 색상 업데이트
  unsigned long currentTime = micros() % pattern[statesInPattern - 1].time;
  updateColorsWithPattern(currentTime, false);

  // REST 활동 점검
  server.handleClient();
}
