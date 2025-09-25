#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
// test committer
// ------------ WiFi ------------
const char* ssid     = "junsukim";
const char* password = "junsukim";

// ------------ SMTP / Gmail ------------
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465  // SSL

#define AUTHOR_EMAIL    "s6503014612019@email.kmutnb.ac.th"
#define AUTHOR_PASSWORD "jtkn awwp zwlz elfd"   // ควรเป็น Gmail App Password
#define RECIPIENT_EMAIL "junsukim5555@gmail.com"

// ------------ NTP ------------
WiFiUDP ntpUDP;
// GMT+7, อัปเดตทุก 60 วินาที
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);

// ------------ Mail objects ------------
SMTPSession smtp;
Session_Config smtpConfig;

// เก็บเนื้อหาอีเมลให้มีอายุยืนพอระหว่างส่ง
String emailBody;

// ------------ บัฟเฟอร์ข้อมูล ------------
struct Record {
  String datetime;  // "YYYY-MM-DD HH:MM:SS"
  int v1;
  int v2;
};

Record buffer10[10];
int bufIdx = 0;

// ตัวจับเวลา 2 วินาที
unsigned long lastTick = 0;
const unsigned long intervalMs = 2000;

// ฟังก์ชันแปลง epoch เป็น "YYYY-MM-DD HH:MM:SS"
String formatDateTime(time_t epochSec) {
  struct tm t;
  localtime_r(&epochSec, &t);  // ใช้ offset จาก NTPClient แล้ว (GMT+7)
  char buf[20];
  // YYYY-MM-DD HH:MM:SS
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
           t.tm_hour, t.tm_min, t.tm_sec);
  return String(buf);
}

// เตรียมคอนฟิก SMTP (เรียกครั้งเดียว)
void setupSMTP() {
  smtp.debug(1);
  smtp.callback([](SMTP_Status status) {
    Serial.println(status.info());
  });

  smtpConfig.server.host_name = SMTP_HOST;
  smtpConfig.server.port      = SMTP_PORT;
  smtpConfig.login.email      = AUTHOR_EMAIL;
  smtpConfig.login.password   = AUTHOR_PASSWORD;
  smtpConfig.login.user_domain = "";
}

// ประกอบเนื้อหาอีเมลจาก buffer10
void buildEmailBody() {
  emailBody = "";
  emailBody += "ESP32 Data Report (10 rows)\n";
  emailBody += "----------------------------------------\n";
  emailBody += "DateTime,Value1,Value2\n";
  for (int i = 0; i < 10; i++) {
    emailBody += buffer10[i].datetime;
    emailBody += ",";
    emailBody += String(buffer10[i].v1);
    emailBody += ",";
    emailBody += String(buffer10[i].v2);
    emailBody += "\n";
  }
  emailBody += "----------------------------------------\n";
}

// ส่งอีเมล 1 ฉบับ
bool sendEmailNow() {
  SMTP_Message message;  // รีเซ็ตทุกครั้ง เพื่อหลีกเลี่ยงซ้ำซ้อนผู้รับ/หัวข้อ

  message.sender.name  = "ESP32 Mail Sender";
  message.sender.email = AUTHOR_EMAIL;
  message.subject      = "ESP32 Email: 10 Random Records";

  message.addRecipient("Receiver", RECIPIENT_EMAIL);

  // ประกอบเนื้อหา
  buildEmailBody();

  message.text.content            = emailBody.c_str();
  message.text.charSet            = "utf-8";
  message.text.transfer_encoding  = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&smtpConfig)) {
    Serial.println("SMTP connect failed.");
    return false;
  }

  bool ok = MailClient.sendMail(&smtp, &message);
  if (!ok) {
    Serial.print("Error sending Email: ");
    Serial.println(smtp.errorReason());
  } else {
    Serial.println("Email sent successfully!");
  }

  smtp.closeSession();
  return ok;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // ---- WiFi ----
  Serial.printf("Connecting to WiFi: %s", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.printf("\nWiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());

  // ---- NTP ----
  timeClient.begin();
  Serial.print("Wait NTP sync");
  while (!timeClient.update()) {
    timeClient.forceUpdate();
    Serial.print(".");
    delay(200);
  }
  Serial.println("\nTime synced!");

  // ---- SMTP config ----
  setupSMTP();

  // seed random
  randomSeed(esp_random());

  lastTick = millis();
}

void loop() {
  // อัปเดตเวลาเป็นระยะ (เพื่อกัน drift)
  if (!timeClient.update()) {
    // ไม่ต้องถี่นักก็ได้
    static unsigned long lastForce = 0;
    if (millis() - lastForce > 5000) {
      timeClient.forceUpdate();
      lastForce = millis();
    }
  }

  // ทุกๆ 2 วินาที สุ่มและเก็บค่า
  unsigned long now = millis();
  if (now - lastTick >= intervalMs) {
    lastTick = now;

    // อ่านเวลาปัจจุบันแบบ epoch (ถูกชดเชยเป็น GMT+7 แล้วจาก NTPClient)
    time_t epoch = timeClient.getEpochTime();
    String dt = formatDateTime(epoch);

    // สุ่มค่า 2 ค่า (ปรับช่วงได้ตามต้องการ)
    int v1 = random(0, 1001);    // 0..1000
    int v2 = random(0, 1001);    // 0..1000

    // เก็บลงบัฟเฟอร์
    buffer10[bufIdx].datetime = dt;
    buffer10[bufIdx].v1 = v1;
    buffer10[bufIdx].v2 = v2;

    Serial.printf("Add [%02d] %s, %d, %d\n", bufIdx, dt.c_str(), v1, v2);

    bufIdx++;

    // ครบ 10 แถว → ส่งอีเมล แล้วเริ่มรอบใหม่
    if (bufIdx >= 10) {
      Serial.println("Collected 10 records. Sending email...");
      bool ok = sendEmailNow();
      if (ok) {
        // ล้าง index เพื่อเริ่มสะสมใหม่
        bufIdx = 0;
      } else {
        // ถ้าส่งไม่สำเร็จ ให้ลองคงข้อมูลไว้เพื่อพยายามส่งรอบหน้า
        Serial.println("Send failed. Will retry after next cycle.");
      }
    }
  }
}
