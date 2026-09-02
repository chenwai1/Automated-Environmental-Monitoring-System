#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL63Wrm7ORO"
#define BLYNK_TEMPLATE_NAME "Plant Monitoring System"
#define BLYNK_AUTH_TOKEN "_RcTHolNHhLMh39IcdkuuyoBsQ3O9CTO"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

// --- 配置区 ---
const char* ssid = ;//your WIFI name
const char* password = ;//your WIFI password

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo shadeServo;
int shadeServoPin = 13;
int pumpPin = 26;
int ldrPin = 34;
int soilPin = 35;

// 土壤校准
int soilDry = 4095;
int soilWet = 1100;

BlynkTimer timer;

// NTP 时间
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 28800;
const int daylightOffset_sec = 0;

// 状态变量
bool Spray = false;
bool Cool = false;
int systemMode = 1; 
int soilThreshold = 30;
int scheduleHour = 8;
int scheduleMinute = 0;
int sprayDuration = 5;
bool autoShade = true;
int manualServoAngle = 0;

// --- 核心优化：刷新 LCD 函数 ---
// 封装此函数，避免在读传感器时频繁执行 lcd.clear() 导致的闪烁和乱码
void updateLCD(const char* line1, const char* line2) {
  if (Spray) return; // 浇水时由 triggerPump 控制屏幕，不在此刷新
  lcd.setCursor(0, 0);
  lcd.print("                "); // 用空格清除行，比 clear() 稳定
  lcd.setCursor(0, 0);
  lcd.print(line1);
  
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// --- Blynk 控制回调 ---
BLYNK_WRITE(V3) { autoShade = param.asInt(); }
BLYNK_WRITE(V4) { manualServoAngle = param.asInt(); }
BLYNK_WRITE(V11) { if(systemMode == 1) soilThreshold = param.asInt(); }
BLYNK_WRITE(V13) { if(systemMode == 2) scheduleHour = param.asInt(); }
BLYNK_WRITE(V15) { if(systemMode == 2) scheduleMinute = param.asInt(); }
BLYNK_WRITE(V16) { sprayDuration = param.asInt(); }
BLYNK_WRITE(V14) {
  systemMode = param.asInt();
  // 隐藏/显示逻辑
  Blynk.setProperty(V10, "isHidden", !(systemMode == 0));
  Blynk.setProperty(V11, "isHidden", !(systemMode == 1));
  Blynk.setProperty(V13, "isHidden", !(systemMode == 2));
  Blynk.setProperty(V15, "isHidden", !(systemMode == 2));
  lcd.clear();
}

// 手动浇水
BLYNK_WRITE(V10) {
  if (param.asInt() == 1 && !Spray && !Cool) {
    triggerPump("MANUAL WATER");
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V11, V13, V14, V15, V16);
}

// --- 浇水执行函数 (含乱码预防) ---
void triggerPump(const char* msg) {
  Spray = true;
  
  // 1. 关泵前先清屏，留出 CPU 给 I2C
  lcd.clear();
  lcd.print("PUMP STARTING...");
  
  delay(100); // 避开瞬间电流冲击
  digitalWrite(pumpPin, HIGH);
  Serial.println(msg);

  // 显示倒计时信息
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
  char durMsg[17];
  snprintf(durMsg, sizeof(durMsg), "DUR:%ds COOL:10s", sprayDuration);
  lcd.setCursor(0, 1);
  lcd.print(durMsg);

  // 设置关泵定时器
  timer.setTimeout(sprayDuration * 1000L, []() {
    digitalWrite(pumpPin, LOW); // 关泵
    Spray = false;
    Cool = true;
    
    delay(200); // 等待电压稳定
    lcd.init(); // <--- 关键：关泵后重新初始化 LCD，修复可能出现的乱码
    lcd.backlight();
    lcd.clear();
    lcd.print("COOLING DOWN...");

    // 冷却 10 秒后恢复正常
    timer.setTimeout(10000L, []() {
      Cool = false;
      lcd.clear();
    });
  });
}

// --- 传感器读取 ---
void readSensors() {
  float airTemp = dht.readTemperature();
  float airHum = dht.readHumidity();
  int lightValue = analogRead(ldrPin);
  int soilRaw = analogRead(soilPin);

  if (isnan(airTemp)) return;

  int soilPercent = map(soilRaw, soilDry, soilWet, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  // 上传 Blynk
  Blynk.virtualWrite(V0, airTemp);
  Blynk.virtualWrite(V1, lightValue);
  Blynk.virtualWrite(V2, soilPercent);

  // LCD 状态显示逻辑
  if (Cool) {
    lcd.setCursor(0, 0);
    lcd.print("SYSTEM COOLING ");
  } 
  else if (!Spray) {
    char l1[17], l2[17];
    const char* mNames[] = {"MAN", "AUTO", "SCD"};
    
    snprintf(l1, sizeof(l1), "Soil:%d%% T:%.1f", soilPercent, airTemp);
    
    if (systemMode == 1)
      snprintf(l2, sizeof(l2), "M:AUTO Th:%d", soilThreshold);
    else if (systemMode == 2)
      snprintf(l2, sizeof(l2), "M:SCD  %02d:%02d", scheduleHour, scheduleMinute);
    else
      snprintf(l2, sizeof(l2), "Mode: MANUAL  ");

    updateLCD(l1, l2);
  }

  // 自动模式触发
  if (systemMode == 1 && !Spray && !Cool) {
    if (soilPercent <= soilThreshold) {
      triggerPump("AUTO WATER");
    }
  }

  // 遮阳帘逻辑
  if (autoShade) {
    if (lightValue > 2500 && airTemp > 30) {
      shadeServo.write(90);
    } else {
      shadeServo.write(0);
    }
  } else {
    shadeServo.write(manualServoAngle);
  }
}

// --- 计划任务 ---
void checkSchedule() {
  if (systemMode != 2) return;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  if (timeinfo.tm_hour == scheduleHour && timeinfo.tm_min == scheduleMinute && timeinfo.tm_sec == 0) {
    if (!Spray && !Cool) triggerPump("SCHED WATER");
  }
}

// --- 初始化 ---
void setup() {
  Serial.begin(115200);
  
  lcd.init();
  lcd.backlight();
  lcd.print("System Ready!");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  dht.begin();
  
  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW);
  
  shadeServo.attach(shadeServoPin);
  shadeServo.write(0);

  timer.setInterval(3000L, readSensors);
  timer.setInterval(1000L, checkSchedule);
}

void loop() {
  Blynk.run();
  timer.run();
}