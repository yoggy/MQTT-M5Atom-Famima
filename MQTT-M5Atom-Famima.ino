//
// M5Atom + Seeed Serial MP3 Player sample
//
#include <M5Atom.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

// EspSoftwareSerial
#include <SoftwareSerial.h>

#include "config.h"

WiFiClient wifi_client;
void mqtt_sub_callback(char* topic, byte* payload, unsigned int length);
PubSubClient mqtt_client(mqtt_host, mqtt_port, mqtt_sub_callback, wifi_client);

SoftwareSerial mySerial;
byte paramBuffer[16];

void setup() {
  Serial.begin(115200);
  M5.begin(true, false, true);
  M5.dis.drawpix(0, 0xffffff);  // white

  // for Seeed Serial MP3 Player
  mySerial.begin(9600, SWSERIAL_8N1, /* rx */32, /* tx */26 , false, 256);

  delay(100);
  GroveMP3V3_SetVolume(20); // 1-31


  // Wifi
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.setSleep(false);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    switch (count % 4) {
      case 0:
        Serial.println("|");
        M5.dis.drawpix(0, 0xffff00);  // yellow
        break;
      case 1:
        Serial.println("/");
        break;
      case 2:
        M5.dis.drawpix(0, 0x000000);  // black
        Serial.println("-");
        break;
      case 3:
        Serial.println("\\");
        break;
    }
    count ++;
    if (count >= 240) reboot(); // 240 / 4 = 60sec
  }
  Serial.println("WiFi connected!");
  delay(1000);

  // MQTT
  bool rv = false;
  if (mqtt_use_auth == true) {
    rv = mqtt_client.connect(mqtt_client_id, mqtt_username, mqtt_password);
  }
  else {
    rv = mqtt_client.connect(mqtt_client_id);
  }
  if (rv == false) {
    Serial.println("mqtt connecting failed...");
    reboot();
  }
  Serial.println("MQTT connected!");
  delay(1000);

  M5.dis.drawpix(0, 0x000088);  // blue

  mqtt_client.subscribe(mqtt_subscribe_topic);


  // configTime
  configTime(9 * 3600, 0, "ntp.nict.jp");
  struct tm t;
  if (!getLocalTime(&t)) {
    Serial.println("getLocalTime() failed...");
    delay(1000);
    reboot();
  }
  Serial.println("configTime() success!");
  M5.dis.drawpix(0, 0x008800);  // green

  delay(1000);
}

void reboot() {
  Serial.println("REBOOT!!!!!");
  for (int i = 0; i < 30; ++i) {
    M5.dis.drawpix(0,0xff00ff); // magenta
    delay(100);
    M5.dis.drawpix(0,0x000000);
    delay(100);
  }

  ESP.restart();
}

void loop() {
  mqtt_client.loop();
  if (!mqtt_client.connected()) {
    Serial.println("MQTT disconnected...");
    reboot();
  }
  
  M5.update();
  if ( M5.Btn.wasPressed() ) {
    Serial.println("Btn presswd");
    GroveMP3V3_PlaySDRootSong(1);
  }
}

#define BUF_LEN 16
char buf[BUF_LEN];

void mqtt_sub_callback(char* topic, byte* payload, unsigned int length) {

  int len = BUF_LEN - 1 < length ? (BUF_LEN - 1) : length;
  memset(buf, 0, BUF_LEN);
  strncpy(buf, (const char*)payload, len);

  String cmd = String(buf);
  Serial.print("payload=");
  Serial.println(cmd);

  if (cmd == "play") {
    Serial.println("Play Song 1");
    GroveMP3V3_PlaySDRootSong(1);
  }
}

// see also...
// https://github.com/Seeed-Studio/Seeed_Serial_MP3_Player/blob/master/src/WT2003S_Player.h
// https://github.com/Seeed-Studio/Seeed_Serial_MP3_Player/blob/master/src/WT2003S_Player.cpp
// https://uepon.hatenadiary.com/entry/2019/12/29/164445
void GroveMP3V3_SetVolume(uint val)
{
  paramBuffer[0] = val;
  GroveMP3V3_WriteCommand(0xAE, paramBuffer, 1);
}

void GroveMP3V3_PlaySDRootSong(uint16_t val)
{
  paramBuffer[0] = (byte)((val >> 8) & 0xFF);
  paramBuffer[1] = (byte)(0xFF & val);
  GroveMP3V3_WriteCommand(0xA2, paramBuffer, 2);
}

// see also...
// https://uepon.hatenadiary.com/entry/2019/12/27/223818
void GroveMP3V3_WriteCommand(const byte &commandCode, const byte *parameter, const int parameterSize)
{
  byte length = 1 + 1 + (byte)parameterSize + 1;
  byte sum = 0;

  sum += length;
  sum += commandCode;
  for (int i = 0; i < parameterSize; i++) sum += parameter[i];

  mySerial.write(0x7E);
  mySerial.write(length);
  mySerial.write(commandCode);
  mySerial.write(parameter, parameterSize);
  mySerial.write(sum);
  mySerial.write(0xEF);
}
