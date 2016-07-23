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

#include <ESP8266WebServer.h>  // reuired for Server opertaions
#include<FS.h>  // reuired for SPIFFS opertaions

ESP8266WebServer ConfigServer(80);

extern boolean WiFiConnected;

extern Storage DB;

void initConfigServer() {
  SPIFFS.begin();

  // host is dynamically generated
  // local IP address is send as host
  ConfigServer.on("/host.js", []() {
    String content = String("var host = \"") + (WiFiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "\";\r\n";
    ConfigServer.send(200, "text/javascript", content);
  });

  ConfigServer.on("/data.js", []() {
    String content = String("var ip = '") + (DB.getIP().length() < 3 ? (WiFiConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) : DB.getIP()) + "';\r\n";
    content += String("var gateway = '") + DB.getGateway() + "';\r\n";
    content += String("var host = '") + DB.getHostname() + "';\r\n";
    content += String("var subnet = '") + DB.getSubnet() + "';\r\n";
    content += String("var wifimode = '") + (WiFiConnected ? "STA" : "AP") + "';\r\n";
    content += String("var ssid = '") + DB.getSSID() + "';\r\n";
    content += String("var enc = '") + DB.getProtection() + "';\r\n";
    content += String("var configmode = '") + DB.getConfigMode() + "';\r\n";
    ConfigServer.send(200, "text/javascript", content);
  });

  // a javascript array of options is send
  ConfigServer.on("/wifilist.js", []() {
    String Options = "var Options = [";
    byte n = WiFi.scanNetworks();
    if (n > 0) {
      for (byte i = 0; i < n; ++i) {
        Options += "{\"enc\":\"" + String((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "f" : "t") + "\",";
        Options += "\"ssid\":\"" + WiFi.SSID(i) + "\"}";
        if (i < (n - 1))
          Options += ",\r\n";
        else
          Options += "\r\n";
      }
    }
    Options += "];";
    ConfigServer.send(200, "text/javascript", Options);
  });

  ConfigServer.on("/", []() {
    File page = SPIFFS.open("/index.html", "r");
    ConfigServer.streamFile(page, "text/html");
    page.close();
  });

  // configuration page
  ConfigServer.on("/config.html", []() {
    File page = SPIFFS.open("/config.html", "r");
    ConfigServer.streamFile(page, "text/html");
    page.close();
  });

  // config update command
  ConfigServer.on("/updateconfig", []() {
    // get all post data
    String wifimode = URLDecode(ConfigServer.arg("wifimode"));
    String ssid = URLDecode(ConfigServer.arg("ssid"));
    String encryption = URLDecode(ConfigServer.arg("encryption")); // protected/open
    String pass = URLDecode(ConfigServer.arg("pass"));
    String host = URLDecode(ConfigServer.arg("host"));
    String configmode = URLDecode(ConfigServer.arg("configmode"));
    String ip = URLDecode(ConfigServer.arg("ip"));
    String gateway = URLDecode(ConfigServer.arg("gateway"));
    String subnet = URLDecode(ConfigServer.arg("subnet"));

    /* Input error checking */
    String ErrMsg;
    boolean err = false;
    IPAddress test;

    if (host.length() < 3 || host.length() > 20) {
      ErrMsg = "Invalid host name length (min 3, max 20)";
      err = true;
    } else if (!(wifimode == "ap" || wifimode == "sta")) {
      ErrMsg = "Invalid wifi mode";
      err = true;
    } else if (ssid.length() < 3 || ssid.length() > 30) {
      ErrMsg = "Invalid SSID length (min 3, max 30)";
      err = true;
    } else if (encryption != "true" && encryption != "false") { // important parameters missing
      ErrMsg = "Invalid protection type";
      err = true;
    } else if (configmode != "auto" && configmode != "manual") { // important parameters missing
      ErrMsg = "Invalid config mode";
      err = true;
    } else if (encryption == "true" && pass.length() < 8) { // password missing for encrypted wifi
      ErrMsg = "Invalid password length (min 8)";
      err = true;
    } else if (configmode == "manual" && (ip.length() < 7 || gateway.length() < 7 || subnet.length() < 7)) { // too small ip,gateway or subnet
      ErrMsg = "Too small ip address(s) for config settings";
      err = true;
    } else if (configmode == "manual" && (ip.length() > 15 || gateway.length() > 15 || subnet.length() > 15)) { // too long ip,gateway or subnet
      ErrMsg = "Too long ip address(s) for config settings";
      err = true;
    } else if (configmode == "manual" && (!test.fromString(ip.c_str()) || !test.fromString(gateway.c_str()) || !test.fromString(subnet.c_str()))) { // invalid ip format
      ErrMsg = "Invalid ip";
      err = true;
    }
    if (err) {
      String content = String(F("<!DOCTYPE HTML>\r\n"
                                "<html>\n"
                                "  <body style=\"text-align: center;\">\n")) +
                       ErrMsg
                       + F("  </body>\n"
                           "</html>");
      ConfigServer.send(200, "text/html", content);
      return;
    }
    /* Input error checking ends */

    /* writing to storage start */
    DB.clear();
    DB.setWiFiMode(wifimode);
    DB.setSSID(ssid);
    if (encryption == "true") {
      DB.setProtection(byte(1));
      DB.setPassword(pass);
    } else {
      DB.setProtection(byte(0));
    }
    DB.setHostname(host);
    if (configmode == "manual") {
      DB.setConfigMode(byte (1));
      DB.setIP(ip);
      DB.setGateway(gateway);
      DB.setSubnet(subnet);
    } else {
      DB.setConfigMode(byte (0));
    }
    DB.finalize();
    /* writing to storage end */

    String content = "<!DOCTYPE HTML>\r\n"
                     "<html>\n"
                     "  <body style=\"text-align: center;\">\n"
                     "    <h3>WiFi settings updated</h3>\n"
                     "    <h4>SSID: " + ssid + "</h4>\n"
                     "    <h4>Protection: " + encryption + "</h4>\n"
                     "    <h4>Host: " + host + "</h4>\n"
                     "    <h3>Restart to Connect</h3>\n"
                     "  </body>\n"
                     "</html>";

    ConfigServer.send(200, "text/html", content);

    delay(100);
    // restart ESP
    ESP.restart();
  });

  // error page
  ConfigServer.onNotFound([]() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += ConfigServer.uri();
    message += "\nMethod: ";
    message += (ConfigServer.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += ConfigServer.args();
    message += "\n";
    for (uint8_t i = 0; i < ConfigServer.args(); i++) {
      message += " " + ConfigServer.argName(i) + ": " + ConfigServer.arg(i) + "\n";
    }
    ConfigServer.send(404, "text/plain", message);
  });

  // starting the local server with the defined handlers
  ConfigServer.begin();
}

void ConfigServerRun() {
  ConfigServer.handleClient();
}

String URLDecode(String param) {
  param.replace("+", " ");
  param.replace("%21", "!");
  param.replace("%23", "#");
  param.replace("%24", "$");
  param.replace("%26", "&");
  param.replace("%27", "'");
  param.replace("%28", "(");
  param.replace("%29", ")");
  param.replace("%2A", "*");
  param.replace("%2B", "+");
  param.replace("%2C", ",");
  param.replace("%2F", "/");
  param.replace("%3A", ":");
  param.replace("%3B", ";");
  param.replace("%3D", "=");
  param.replace("%3F", "?");
  param.replace("%40", "@");
  param.replace("%5B", "[");
  param.replace("%5D", "]");
  return param;
}


