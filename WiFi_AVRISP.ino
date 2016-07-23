/*
  WiFi AVRISP
  Copyright (c) 2016 Subhajit Das

  Licence Disclaimer:
    This file is part of WiFi AVRISP.

    WiFi AVRISP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    WiFi AVRISP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266AVRISP.h>
#include <ESP_ConfigStorage.h>

#define BAUD_RATE 115200

boolean WiFiConnected;

Storage DB(0);

void setup() {
  Serial.begin(BAUD_RATE);  // initiate serial debug
  if (DB.getHostname().length() >= 8)
    MDNS.begin(DB.getHostname().c_str()); // start DNS server
  else
    MDNS.begin("WiFi_AVRISP"); // start DNS server
  setupWifi();  // setup wifi
  initISP();
  initConfigServer();
}

void loop() {
  serveISP();
  ConfigServerRun();
  yield(); // this is most important part of the code, as it tells the esp8266 keep running background wifi work,
  //without this, your code  will disconnect from wifi, after long run of code.
}

void setupWifi() {
  if (DB.getWiFiMode() == "sta") {
    WiFi.mode(WIFI_STA);
    if (DB.getConfigMode()) { //manual
      IPAddress ip, gateway, subnet;
      ip.fromString(DB.getIP().c_str());
      gateway.fromString(DB.getGateway().c_str());
      subnet.fromString(DB.getSubnet().c_str());
      WiFi.config(ip, gateway, subnet);
    }
    // starting wifi
    String ssid = DB.getSSID();
    if (DB.getProtection()) { // protected
      String pass = DB.getPassword();
      WiFi.begin(ssid.c_str(), pass.c_str());
    } else {
      WiFi.begin(ssid.c_str());
    }
    if (WiFiAvailable()) {
      return;
    }
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  if (DB.getConfigMode()) { //manual
    String ip = DB.getIP();
    String gateway = DB.getGateway();
    String subnet = DB.getSubnet();
    if (DB.getConfigMode()) { //manual
      IPAddress ip, gateway, subnet;
      ip.fromString(DB.getIP().c_str());
      gateway.fromString(DB.getGateway().c_str());
      subnet.fromString(DB.getSubnet().c_str());
      WiFi.softAPConfig(ip, gateway, subnet);
    }
  }

  //getting cleanest wifi channel
  byte channel = getCleanestChannel();

  // starting wifi
  boolean APStarted = false;

  //checking for wifi under DB SSID names
  String ssid = DB.getSSID();
  byte count = 0;
  byte noOfRunnigWiFi = WiFi.scanNetworks();
  for (byte i = 0; i < noOfRunnigWiFi; i++) {
    if (ssid == WiFi.SSID(i)) {
      ssid = "WiFi_AVRISP-" + String(count);
      count++;
      i = -1;
    }
  }
  
  if (DB.getProtection()) { // protected
    String pass = DB.getPassword();
    APStarted = WiFi.softAP(ssid.c_str(), pass.c_str(), channel);
  }
  
  if (!APStarted) {
    WiFi.softAP(ssid.c_str(), NULL, channel);
  }
}

bool WiFiAvailable() {
  int c = 0;
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      WiFiConnected = true;
      //blinkIndicator();
      return true;
    }
    delay(500);
    c++;
  }
  return false;
}

byte getCleanestChannel(){
  byte channel_to_use = 1;
  byte noOfRunnigWiFi = WiFi.scanNetworks();
  byte ch[3][2] = {{1, 0}, {6, 0}, {11, 0}};
  for (byte i = 0; i <= 2; i++) {
    for (byte j = 0; j <= noOfRunnigWiFi; j++) {
      if (WiFi.channel(j) == ch[i][0]) {
        ch[i][1]++;
      }
    }
  }
  //get min channels
  byte min_ch_no = (ch[0][1] < ch[1][1]) ? ch[0][1] : ch[1][1];
  min_ch_no = (min_ch_no < ch[2][1]) ? min_ch_no : ch[2][1];
  for (byte i = 0; i <= 2; i++) {
    if (ch[i][1] == min_ch_no) {
      channel_to_use = ch[i][0];
      break;
    }
  }

  return channel_to_use;
}
