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

/*
   If the AVR target is powered by a different Vcc than what powers your ESP8266
  chip, you **must provide voltage level shifting** or some other form of buffers.
  Exposing the pins of ESP8266 to anything larger than 3.6V will damage it.

  Connections are as follows:
  -------------------------------
 | ESP8266 | AVR / SPI | Arduino |
 | --------|---------------------|
 | GPIO12  | MISO      | D12     | *Voltage divider required*
 | GPIO13  | MOSI      | D11     |
 | GPIO14  | SCK       | D13     |
 | GPIO5   | RESET     | RST     |
  -------------------------------
  Note: MISO,MOSI,SCK connections are unchanable, 
        but RESET (default GIPO5 canbe programatically changed.

  For RESET use a GPIO other than 0, 2 and 15 (bootselect pins), and apply an
  external pullup/down so that the target is normally running.
*/

// socket port to run ISP
const uint16_t port = 384;
const uint16_t reset_pin = 5;

ESP8266AVRISP avrprog(port, reset_pin);  // ISP system

extern boolean WiFiConnected;

void initISP() {
  Serial.println("");
  Serial.println("Arduino AVR-ISP over TCP started");
  avrprog.setReset(false); // let the AVR run

  MDNS.addService("avrisp", "tcp", port);

  IPAddress local_ip = WiFiConnected ? WiFi.localIP() : WiFi.softAPIP();
  Serial.print("IP address: ");
  Serial.println((DB.getIP().length() < 3 ? local_ip.toString() : DB.getIP()));
  Serial.print("SSID: ");
  if (DB.getSSID().length() >= 3)
    Serial.println(DB.getSSID());
  else
    Serial.println("WiFi_AVRISP");
  Serial.print("Host: ");
  if (DB.getHostname().length() >= 3)
    Serial.println(DB.getHostname());
  else
    Serial.println("WiFi_AVRISP");
  Serial.println("Use your avrdude:");
  Serial.print("avrdude -c arduino -p <device> -P net:");
  Serial.print(local_ip);
  Serial.print(":");
  Serial.print(port);
  Serial.println(" -t # or -U ...");

  // listen for avrdudes
  avrprog.begin();
}

AVRISPState_t last_state = AVRISP_STATE_IDLE; // string current status

void serveISP() {
  updateISPStatus();

  // Serve the client
  if (last_state != AVRISP_STATE_IDLE) {
    avrprog.serve();
  }
}

void updateISPStatus() {
  AVRISPState_t new_state = avrprog.update();
  if (last_state != new_state) {
    switch (new_state) {
      case AVRISP_STATE_IDLE: {
          Serial.printf("[AVRISP] now idle\r\n");
          // Use the SPI bus for other purposes
          break;
        }
      case AVRISP_STATE_PENDING: {
          Serial.printf("[AVRISP] connection pending\r\n");
          // Clean up your other purposes and prepare for programming mode
          break;
        }
      case AVRISP_STATE_ACTIVE: {
          Serial.printf("[AVRISP] programming mode\r\n");
          // Stand by for completion
          break;
        }
    }
    last_state = new_state;
  }
}

