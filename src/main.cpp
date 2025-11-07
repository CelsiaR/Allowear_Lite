#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include <DHT.h>

#define DHTPIN      4
#define DHTTYPE     DHT22
#define POT_PIN     34
#define ALERT_LED   2

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPU6050 mpu;

const char* ssid        = "Wokwi-GUEST";
const char* password    = "";
const char* mqtt_server = "broker.hivemq.com";
const int   mqtt_port   = 1883;
const char* mqtt_client_id = "esp32_allowear_demo";
const char* topic_data  = "celsia/health/data";
const char* topic_alert = "celsia/alert";

WiFiClient espClient;
PubSubClient client(espClient);

float simHR = 76.0f;
const int HRV_BUF = 20;
float rrBuf[HRV_BUF]; int rrIdx = 0; int rrCount = 0;
uint32_t steps = 0;
bool stepLatch = false;

static float clamp01(float x){ if(x<0) return 0; if(x>1) return 1; return x; }

float computeRMSSD() {
  if (rrCount < 3) return 80.0f;
  double sumSq = 0; int n = 0;
  for (int i = 1; i < rrCount; ++i) {
    double diff = rrBuf[i] - rrBuf[i-1];
    sumSq += diff * diff;
    n++;
  }
  return n ? sqrt(sumSq / n) : 80.0f;
}

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT ... ");
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
    } else {
      Serial.printf("failed, rc=%d; retry in 3s\n", client.state());
      delay(3000);
    }
  }
}

void oledPrint(float tempC, float accelMag, int heart, float hrv, uint32_t stepCount, float stress, bool cry) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.printf("Temp : %.1f C\n", tempC);
  display.printf("Accel: %.2f m/s^2\n", accelMag);
  display.printf("Steps: %lu\n", (unsigned long)stepCount);
  display.printf("HR   : %d bpm\n", heart);
  display.printf("HRV  : %.0f ms\n", hrv);
  display.printf("Stress: %.2f\n", stress);
  if (cry) {
    display.setCursor(0, 54);
    display.print("ALERT: Baby Cry!");
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(ALERT_LED, OUTPUT);
  pinMode(POT_PIN, INPUT);

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 failed"); while (true) {}
  }
  display.clearDisplay(); display.display();

  dht.begin();

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found"); while (true) {}
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  float rr = 60000.0f / simHR;
  for (int i=0;i<HRV_BUF;i++){ rrBuf[i]=rr; }
  rrCount = HRV_BUF;
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  float tempC = dht.readTemperature();
  if (isnan(tempC)) tempC = 36.5f;

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  float accelMag = sqrtf(a.acceleration.x*a.acceleration.x +
                         a.acceleration.y*a.acceleration.y +
                         a.acceleration.z*a.acceleration.z);
  if (!stepLatch && accelMag > 12.0f) { steps++; stepLatch = true; }
  if (stepLatch && accelMag < 10.0f)  { stepLatch = false; }

  int audioRaw = analogRead(POT_PIN);
  float audioPct = audioRaw / 4095.0f;
  bool babyCry = (audioRaw > 2600);

  simHR += (random(-2,3)) + (audioPct * 0.5f);
  if (simHR < 60) simHR = 60;
  if (simHR > 110) simHR = 110;
  int heart = (int)roundf(simHR);

  float rr = 60000.0f / simHR;
  rr += (random(-15,16));
  rrBuf[rrIdx] = rr;
  rrIdx = (rrIdx + 1) % HRV_BUF;
  if (rrCount < HRV_BUF) rrCount++;
  float hrv_ms = computeRMSSD();

  float normHR   = clamp01((simHR - 60.0f) / (110.0f - 60.0f));
  float normHRV  = clamp01((hrv_ms - 20.0f) / (120.0f - 20.0f));
  float normTemp = clamp01((tempC  - 35.0f) / (38.0f - 35.0f));
  float stress   = clamp01(0.45f*normHR + 0.4f*(1.0f - normHRV) + 0.15f*normTemp);

  char payload[300];
  snprintf(payload, sizeof(payload),
    "{\"temp\":%.1f,\"motion\":%.2f,\"steps\":%lu,"
    "\"heart\":%d,\"hrv_ms\":%.0f,\"audio\":%.2f,\"stress\":%.2f}",
    tempC, accelMag, (unsigned long)steps, heart, hrv_ms, audioPct, stress);

client.publish(topic_data, payload);
Serial.print("Published JSON: ");
Serial.println(payload);

char humanMsg[150];
snprintf(humanMsg, sizeof(humanMsg),
         "Temp=%.1fC | Heart=%dbpm | HRV=%.0fms | Steps=%lu | Stress=%.2f",
         tempC, heart, hrv_ms, (unsigned long)steps, stress);

client.publish("celsia/health/readable", humanMsg);
Serial.print("Published Text: ");
Serial.println(humanMsg);


  bool tempHigh = tempC >= 38.0f;
  bool hrAbn    = (heart <= 50 || heart >= 120);
  if (babyCry || tempHigh || hrAbn) {
    digitalWrite(ALERT_LED, HIGH);
    if (babyCry) client.publish(topic_alert, "ALERT: Baby Cry Detected!");
    if (tempHigh) client.publish(topic_alert, "ALERT: High Temperature!");
    if (hrAbn) client.publish(topic_alert, "ALERT: Abnormal Heart Rate!");
  } else {
    digitalWrite(ALERT_LED, LOW);
  }

  oledPrint(tempC, accelMag, heart, hrv_ms, steps, stress, babyCry);
  delay(5000);
}
