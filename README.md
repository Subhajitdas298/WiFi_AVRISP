#WiFi_AVRISP

  AVR ISP programmer over WiFi. Build upon ESP8266 and Arduino.

  This project is build upon Arduino platform for ESP8266.

  The system provides-

  An AVR ISP programmer for programming over ISP for any generic AVR chip.
  Provides a we based configuration interface using library at ESP_ConfigStorage at git.
  Simple to use and implement. Instructions provided at instructables (working).
  
  #ISP connection
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
  Note:
  MISO,MOSI,SCK connections are unchanable, 
  but RESET (default GIPO5 canbe programatically changed.
  Use a GPIO other than 0, 2 and 15 (bootselect pins), and apply an
  external pullup/down so that the target is normally running.
  external pullup/down so that the target is normally running.
  
  #Wifi config
  The system will boot with saved previous wifi configuratoions (if any). Confuguration ca be set with web interface.
  