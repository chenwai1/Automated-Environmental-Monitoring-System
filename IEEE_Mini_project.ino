
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL63Wrm7ORO"
#define BLYNK_TEMPLATE_NAME "Plant Monitoring System"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

// --- Configuration ---
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo shadeServo;
int shadeServoPin = 13;
int pumpPin = 26;
int ldrPin = 34;
int soilPin = 35;

// Soil Moisture Calibration
int soilDry = 4095;
int soilWet = 1100;

BlynkTimer timer;

// NTP Time Configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 28800;
const int daylightOffset_sec = 0;

// System Status Variables
bool Spray = false;
bool Cool = false;
int systemMode = 1;
int soilThreshold = 30;
int scheduleHour = 8;
int scheduleMinute = 0;
int sprayDuration = 5;
bool autoShade = true;
int manualServoAngle = 0;

// --- LCD Refresh Optimization ---
// This function prevents screen flickering and display errors
// by avoiding frequent lcd.clear() operations during sensor readings.
void updateLCD(const char* line1, const char* line2) {
  if (Spray) return; // LCD is controlled by triggerPump during watering

  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(line1);

  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// --- Blynk Control Callbacks ---
BLYNK_WRITE(V3) { autoShade = param.asInt(); }
BLYNK_WRITE(V4) { manualServoAngle = param.asInt(); }
BLYNK_WRITE(V11) { if(systemMode == 1) soilThreshold = param.asInt(); }
BLYNK_WRITE(V13) { if(systemMode == 2) scheduleHour = param.asInt(); }
BLYNK_WRITE(V15) { if(systemMode == 2) scheduleMinute = param.asInt(); }
BLYNK_WRITE(V16) { sprayDuration = param.asInt(); }

BLYNK_WRITE(V14) {
  systemMode = param.asInt();

  // Widget visibility control
  Blynk.setProperty(V10, "isHidden", !(systemMode == 0));
  Blynk.setProperty(V11, "isHidden", !(systemMode == 1));
  Blynk.setProperty(V13, "isHidden", !(systemMode == 2));
  Blynk.setProperty(V15, "isHidden", !(systemMode == 2));

  lcd.clear();
}

// Manual Watering Control
BLYNK_WRITE(V10) {
  if (param.asInt() == 1 && !Spray && !Cool) {
    triggerPump("MANUAL WATER");
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V11, V13, V14, V15, V16);
}

// --- Water Pump Activation Function ---
// Includes LCD display error prevention
void triggerPump(const char* msg) {
  Spray = true;

  // Clear the LCD before activating the pump
  // to allow sufficient CPU time for I2C communication
  lcd.clear();
  lcd.print("PUMP STARTING...");

  delay(100); // Allow time for the initial power surge to settle

  digitalWrite(pumpPin, HIGH);
  Serial.println(msg);

  // Display watering information
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);

  char durMsg[17];
  snprintf(durMsg, sizeof(durMsg), "DUR:%ds COOL:10s", sprayDuration);

  lcd.setCursor(0, 1);
  lcd.print(durMsg);

  // Set pump shutdown timer
  timer.setTimeout(sprayDuration * 1000L, []() {
    digitalWrite(pumpPin, LOW); // Turn off the pump
    Spray = false;
    Cool = true;

    delay(200); // Allow the power supply voltage to stabilize

    lcd.init(); // Reinitialize LCD to prevent possible display errors
    lcd.backlight();
    lcd.clear();
    lcd.print("COOLING DOWN...");

    // Resume normal operation after a 10-second cooldown
    timer.setTimeout(10000L, []() {
      Cool = false;
      lcd.clear();
    });
  });
}

// --- Sensor Reading Function ---
void readSensors() {
  float airTemp = dht.readTemperature();
  float airHum = dht.readHumidity();
  int lightValue = analogRead(ldrPin);
  int soilRaw = analogRead(soilPin);

  if (isnan(airTemp)) return;

  int soilPercent = map(soilRaw, soilDry, soilWet, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  // Upload sensor readings to Blynk
  Blynk.virtualWrite(V0, airTemp);
  Blynk.virtualWrite(V1, lightValue);
  Blynk.virtualWrite(V2, soilPercent);

  // LCD Status Display Logic
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

  // Automatic Watering Control
  if (systemMode == 1 && !Spray && !Cool) {
    if (soilPercent <= soilThreshold) {
      triggerPump("AUTO WATER");
    }
  }

  // Automatic Sunshade Control
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

// --- Scheduled Watering Function ---
void checkSchedule() {
  if (systemMode != 2) return;

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) return;

  if (timeinfo.tm_hour == scheduleHour &&
      timeinfo.tm_min == scheduleMinute &&
      timeinfo.tm_sec == 0) {

    if (!Spray && !Cool) {
      triggerPump("SCHED WATER");
    }
  }
}

// --- System Initialization ---
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.print("System Ready!");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

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
