#include <Firebase_ESP_Client.h>
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PZEM004Tv30.h>

// ใส่ข้อมูลการเชื่อมต่อ Wi-Fi
#define WIFI_SSID "comprojecy"
#define WIFI_PASSWORD "00000000"

// ใส่ API Key ของ Firebase
#define API_KEY "AIzaSyAyYV2zyx3B1KhB9QlhLWjVBpp31aMnXFw"

// ใส่ URL ของ RTDB
#define DATABASE_URL "proj-3ab96-default-rtdb.firebaseio.com"

// กำหนดอ็อบเจ็กต์ Firebase Data
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// กำหนดอ็อบเจ็กต์ PZEM004Tv30
PZEM004Tv30 pzem(D5, D6); // RX, TX

// รหัสบางอย่าง (device or location ID)
#define DEVICE_ID "DEVICE001"

// ช่วงเวลาที่ส่งข้อมูล (ในมิลลิวินาที)
#define SEND_INTERVAL 10000 // 10 seconds

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// ตั้งค่า NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC +0, 1 นาที

#define ELECTRICITY_RATE 3.0 // อัตราค่าไฟฟ้าเป็นบาทต่อ kWh

// ฟังก์ชันคำนวณค่าไฟฟ้า
float calculateElectricityCost(float energy) {
  return energy * ELECTRICITY_RATE;
}

void setup() {
  Serial.begin(115200);

  // เชื่อมต่อ Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // ตั้งค่า Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // ลงทะเบียนผู้ใช้
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // ตั้งค่าฟังก์ชัน callback สำหรับการสร้างโทเคนที่ใช้เวลานาน
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // เริ่มต้น NTP Client
  timeClient.begin();
  timeClient.setTimeOffset(25200);
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > SEND_INTERVAL || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // อ่านข้อมูลจากเซนเซอร์ PZEM004T
    float voltage = pzem.voltage();
    float current = pzem.current();
    float power = pzem.power();
    float energy = pzem.energy();
    float powerFactor = pzem.pf();

    // คำนวณค่าไฟฟ้า
    float electricityCost = calculateElectricityCost(energy);

    // อัปเดตเวลา NTP Client
    timeClient.update();
    String timestamp = timeClient.getFormattedTime();

    // สร้างโหนดใหม่ใน Firebase
    String basePath = "devices/" + String(DEVICE_ID) + "/data/";

    // ส่งข้อมูลไปยัง Firebase
    if (Firebase.RTDB.setFloat(&fbdo, basePath + "voltage/" + timestamp, random(0,200))) {
      Serial.println("ส่งข้อมูลแรงดันไฟฟ้าสำเร็จ.");
    } else {
      Serial.println("ไม่สามารถส่งข้อมูลแรงดันไฟฟ้าได้: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, basePath + "current/" + timestamp, random(0,200))) {
      Serial.println("ส่งข้อมูลกระแสไฟฟ้าสำเร็จ.");
    } else {
      Serial.println("ไม่สามารถส่งข้อมูลกระแสไฟฟ้าได้: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, basePath + "power/" + timestamp, random(0,200))) {
      Serial.println("ส่งข้อมูลกำลังไฟฟ้าสำเร็จ.");
    } else {
      Serial.println("ไม่สามารถส่งข้อมูลกำลังไฟฟ้าได้: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, basePath + "energy/" + timestamp, random(0,200))) {
      Serial.println("ส่งข้อมูลพลังงานไฟฟ้าสำเร็จ.");
    } else {
      Serial.println("ไม่สามารถส่งข้อมูลพลังงานไฟฟ้าได้: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, basePath + "powerFactor/" + timestamp, random(0,200))) {
      Serial.println("ส่งข้อมูล power factor สำเร็จ.");
    } else {
      Serial.println("ไม่สามารถส่งข้อมูล power factor ได้: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, basePath + "electricityCost/" + timestamp, electricityCost)) {
      Serial.println("ส่งข้อมูลค่าไฟฟ้าสำเร็จ.");
    } else {
      Serial.println("ไม่สามารถส่งข้อมูลค่าไฟฟ้าได้: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setString(&fbdo, basePath + "timestamps/" + timestamp, timestamp)) {
      Serial.println("ส่งข้อมูลเวลาไฟฟ้าสำเร็จ.");
    } else {
      Serial.println("ไม่สามารถส่งข้อมูลเวลาไฟฟ้าได้: " + fbdo.errorReason());
    }
  }
}

// ฟังก์ชัน callback สำหรับสถานะโทเคน
void tokenStatusCallback(TokenInfo info) {
  Serial.print("Token status: ");
  Serial.println(info.status); // ปรับตามสมาชิกที่มีอยู่ใน TokenInfo
}
