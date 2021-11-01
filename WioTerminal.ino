#include <rpcWiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include <Multichannel_Gas_GMXXX.h>
#include <Wire.h>
#include "Free_Fonts.h"
TFT_eSPI tft;
GAS_GMXXX<TwoWire> gas;
WiFiClientSecure wifiClient;
PubSubClient pubsubClient(wifiClient);

//-------------------- EDIT THIS DATA --------------------

const bool send_enabled = true; //Enable or disable link to AirQua.
const char* mqtt_token = "abcdefghijklmnopqrstuvwxyz0123456789"; //Replace with your device's token from airqua.ru account
const char* mqtt_device = "d0"; //Replace with your device's id from airqua.ru account (d in front of number is mandatory!)

//-------------------- DO NOT EDIT ALL BELOW --------------------

const bool debug_wait = false;

const char* mqtt_host = "mqtt.airqua.ru";
const int mqtt_port = 8883;
const char* mqtt_login = "apikey";
const char* mqtt_ca = "-----BEGIN CERTIFICATE-----\nMIID9zCCAt+gAwIBAgIUU1XukMadIT3zZ8NGi3yGsv3kGHkwDQYJKoZIhvcNAQEL\nBQAwgYoxCzAJBgNVBAYTAlJVMQ8wDQYDVQQIDAZNb3Njb3cxDzANBgNVBAcMBk1v\nc2NvdzEPMA0GA1UECgwGQWlycXVhMQ0wCwYDVQQLDARNUVRUMRcwFQYDVQQDDA5t\ncXR0LmFpcnF1YS5ydTEgMB4GCSqGSIb3DQEJARYRc3VwcG9ydEBhaXJxdWEucnUw\nHhcNMjExMDI5MjE0NTIwWhcNMjIxMDI5MjE0NTIwWjCBijELMAkGA1UEBhMCUlUx\nDzANBgNVBAgMBk1vc2NvdzEPMA0GA1UEBwwGTW9zY293MQ8wDQYDVQQKDAZBaXJx\ndWExDTALBgNVBAsMBE1RVFQxFzAVBgNVBAMMDm1xdHQuYWlycXVhLnJ1MSAwHgYJ\nKoZIhvcNAQkBFhFzdXBwb3J0QGFpcnF1YS5ydTCCASIwDQYJKoZIhvcNAQEBBQAD\nggEPADCCAQoCggEBAOsH/ZhfLvYDsPA9uOGdO/2/tCBhJ4x+INW+l4SyqdPTMsbt\njx6FO0uPV07dw0Ar95httH1nNAgyHsZgn4LVH3GV+8z+3/oHNmC7iedJS1j0yswh\nUSgOWO2qDrd3Eh3zkKdEIIdLIq2ZWEXP3J/9WkAp4JSfi9Y2dMZlWdwN5GQLIbSQ\nrpyHM/u7wJ8+j2boIH878moK9p9bAmThISfYc3BS64Yn4ep1oK2iRrc1Za5N5es2\ntElCq40l+VBNYWrouILteIuqr42lrBMA6HFnQcg6XYKVuzz9ZByHYs+ejcJ9fmP5\nakftk+C+EieEDEaX41NuV8UlxFUv4q9ompGLPrUCAwEAAaNTMFEwHQYDVR0OBBYE\nFMp92N2E+IFkZK6qFYCP98egqEz/MB8GA1UdIwQYMBaAFMp92N2E+IFkZK6qFYCP\n98egqEz/MA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBAExK81i7\nygx3J7GQiUPf+EV028gae+j46f431osmn+WBSr/lEaZS8J3OO7AqNz0NXMIFzTks\nafLqBRtWPe+q0gA4Igkvte/WR6QLAMwvTEwon6UomHfwZvgBl56FZePjXy4nxRYP\nEnwLrLC5ZXfwUf2akI0atqTK29DDKDJtha+QlxHEqJpDhD1GvC4dB1zjFqGeHCw7\ngQLIoN6bPTrvoRgHFLOXRwQGTcxDwJitmdkcB+/KVIZBOMouBgPH2kdWbCG0rVp4\nyHtQcWJob39ko0+s4JfQdZ4/QvRvFe0dAjqBoJZMB9TwgpLHZF2mkXWH01ywf7CE\nGr9YHgXfFW7cOII=\n-----END CERTIFICATE-----\n";

const char* test_request_url = "http://worldtimeapi.org/api/ip.txt";

const int preheat_msec = 300000;
const int preheat_pb_sections = 10;

const int update_msec = 60000; // 1 min
const int send_msec = 3600000; // 60 min


const int width = 320;
const int height = 240;

const double molweight_co = 28.01;
const double molweight_no2 = 46.01;
const double molweight_c2h5oh = 46.07;
double PpmToUgm3(double ppm, double molweight) {
  return 0.0409 * ppm * molweight;
}

void redrawWiFiScreen() {
  Serial.println("redrawing wifi screen");
  tft.fillScreen(TFT_BLACK);

  tft.setFreeFont(FF7);
  tft.drawString("AirQua Terminal", (width - tft.textWidth("AirQua Terminal"))/2, height/2 - 30);

  tft.setFreeFont(FF2);
  tft.drawString("Connecting to WiFi...", (width - tft.textWidth("Connecting to WiFi..."))/2, height/2 + 30);

  tft.setFreeFont(FF1);
  tft.drawString("SSID: AirQuaTerminal", (width - tft.textWidth("SSID: AirQuaTerminal"))/2, height/2 + 60);
  tft.drawString("Wait a bit, then connect", (width - tft.textWidth("Wait a bit, then connect,"))/2, height/2 + 80);
  tft.drawString("if network appeared", (width - tft.textWidth("if network appeared"))/2, height/2 + 100);
}

void redrawPreheat() {
  Serial.println("redrawing preheat");
  tft.fillScreen(TFT_BLACK);

  tft.setFreeFont(FF7);
  tft.drawString("AirQua Terminal", (width - tft.textWidth("AirQua Terminal"))/2, height/2 - 30);

  tft.setFreeFont(FF2);
  tft.drawString("Preheating sensors...", (width - tft.textWidth("Preheating sensors..."))/2, height/2 + 30);

  tft.setFreeFont(FF1);
  tft.drawString("^", (width - tft.textWidth("^"))/2 + 15, 5);
  tft.drawString("Cancel", (width - tft.textWidth("Cancel"))/2 + 10, 15);
  tft.drawString("preheat", (width - tft.textWidth("preheat"))/2 + 10, 35);
}
int preheat_pb_last_drawn = -1;
void drawPreheatProgressbar(const int preheat_start) {
  int cusec = (millis() - preheat_start) / (preheat_msec / preheat_pb_sections); //section to draw
  if(cusec == preheat_pb_last_drawn) return;
  tft.fillRect(10 + 30 * cusec, 200, 30, 30, TFT_WHITE);
  preheat_pb_last_drawn = cusec;
}

unsigned long sendt = 0;
unsigned long updatet = 0;
void redrawInterface() {
  Serial.println("redrawing interface");
  tft.fillScreen(TFT_BLACK);

  //Title
  tft.setFreeFont(FF5);
  tft.drawString("AirQua Terminal", 1, 1);
  tft.drawLine(0, 20, width, 20, TFT_WHITE);

  //Footer
  tft.drawLine(0, height - 30, width, height - 30, TFT_WHITE);
  tft.setFreeFont(FF1);
  int tm = (int)((send_msec - (millis() - sendt)) / 60000);
  
  char* buf = new char[25];
  sprintf(buf, "Next upload: %d min", tm);
  tft.drawString(buf, (width - tft.textWidth(buf)) - 1, height - 20);
  delete[] buf;

  //Data // TODO REMAKE THIS WHOLE SHIT
  tft.setFreeFont(FF5);

  const char* sensors[] = {
    "CO", "C2H5OH", "NULL", "NULL", "NO2", "NULL", "NULL", "NULL"
  };
  int data[] = {
    0, 0, 0, 0, 0, 0, 0, 0
  };
  data[0] = PpmToUgm3(gas.getGM702B(), molweight_co);
  data[1] = PpmToUgm3(gas.getGM302B(), molweight_c2h5oh);
  data[4] = PpmToUgm3(gas.getGM102B(), molweight_no2);
  
  
  int i; //columns
  int j; //rows
  int si = 0;
  for(i = 1; i < 4; i+=2)
  {
    for(j = 1; j <= 4; j++)
    {
      if(sensors[si] != "NULL") {
        char* str = new char[25];
        sprintf(str, "%s: %d ug/m3", sensors[si], data[si]);
        tft.drawString(str, 
          (si + 4 <= 7 && sensors[si + 4] == "NULL" && j % 2 == 0) || (si >= 4 && sensors[si - 4] == "NULL" && j % 2 == 1) ? 
          (width - tft.textWidth(str))/2 :
          (width / 4 * i) - tft.textWidth(str)/2, 
        height / 7 * j + 15);
        delete[] str;
      }
      si += 1;
    }
  }
}

void makeTestRequest() {
  HTTPClient http;
  Serial.println("Making test request...");
  http.begin(test_request_url);
  int httpCode = http.GET();
  if(httpCode > 0) {
    Serial.println("Test request made. Data:");
    if(httpCode == HTTP_CODE_OK) {
      Serial.println(http.getString());
    } else {
      Serial.println("http code not ok");
    }
  } else {
    Serial.printf("Test request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void sendMqttData() {
  if (!pubsubClient.connected()) {
    Serial.println("MQTT not connected. Connecting...");
    if(pubsubClient.connect(mqtt_device, mqtt_login, mqtt_token)) {
      Serial.println("MQTT connected successfully.");
    } else {
      Serial.printf("MQTT connection error. State: %d", pubsubClient.state());
      return;
    }
  }
  char* buf = new char[45];
  sprintf(buf, "{\"co\":%d,\"no2\":%d,\"c2h5oh\":%d}", 
    (int)(PpmToUgm3(gas.getGM702B(), molweight_co)), 
    (int)(PpmToUgm3(gas.getGM102B(), molweight_no2)), 
    (int)(PpmToUgm3(gas.getGM302B(), molweight_c2h5oh))
  );
  pubsubClient.publish(mqtt_device, buf);
  Serial.println("Published data to MQTT:");
  Serial.println(buf);
  delete[] buf;
}

int preheat = -1;
// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  while(debug_wait && !Serial) delay(500); //wait for serial
  
  pinMode(WIO_BUZZER, OUTPUT);
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);

  Serial.printf("RTL8720 Firmware Version: %s\n", rpc_system_version());
  
  tft.begin();
  tft.setRotation(3);

  gas.begin(Wire, 0x08);

  if(send_enabled) {
      redrawWiFiScreen();

      WiFiManager wifiManager;
      wifiManager.autoConnect("AirQuaTerminal");
      
      delay(1000);
      makeTestRequest();

      wifiClient.setCACert(mqtt_ca);
      pubsubClient.setServer(mqtt_host, mqtt_port);    
  }

  analogWrite(WIO_BUZZER, 128);
  delay(1000);
  analogWrite(WIO_BUZZER, 0);

  // Preheat screen
  redrawPreheat();
  preheat = millis();
}

void loop() {  
  if(preheat != -1) { // handle preheat screen
    if(digitalRead(WIO_KEY_A) == LOW or millis() >= preheat + preheat_msec) { //end preheat
      analogWrite(WIO_BUZZER, 128);
      delay(1000);
      analogWrite(WIO_BUZZER, 0);
      preheat = -1;
      redrawInterface();
      return;
    }
    drawPreheatProgressbar(preheat);
  } else {
    if (digitalRead(WIO_KEY_C) == LOW) {  // handle force update via C btn
      analogWrite(WIO_BUZZER, 128);
      delay(200);
      analogWrite(WIO_BUZZER, 0);
      redrawInterface();
    }
    if (digitalRead(WIO_KEY_A) == LOW) { // DEBUG
      analogWrite(WIO_BUZZER, 128);
      delay(200);
      analogWrite(WIO_BUZZER, 0);
      sendMqttData();
    }
    if(send_enabled) { // handle sending
      if(millis() - sendt > send_msec) {
        sendt = millis();
        sendMqttData();
      }
    }
    if(millis() - updatet > update_msec) { // handle update once in update_msec mseconds
      updatet = millis();
      redrawInterface();
    } 
  }
}
