#include <LilyGo_SIM7000G.hpp>

#ifdef DEBUG_AT
#include <StreamDebugger.h>

StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

void start_modem(bool restart) {
  Serial.println("Power cycling modem...");
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  // Starting the machine requires at least 1 second of low level, and with a level conversion, the levels are opposite
  delay(1000);
  digitalWrite(PWR_PIN, LOW);
  delay(1000);

  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  if (restart) {
    Serial.println("Restarting modem...");
    if (!modem.restart()) {
      Serial.println("Failed to restart modem, attempting to continue without restart");
    }
  } else {
    Serial.println("Initializing modem...");
    if (!modem.init()) {
      Serial.println("Failed to init modem, attempting to continue without init");
    }
  }

  String name = modem.getModemName();
  Serial.println("Modem Name: " + name);

  String modemInfo = modem.getModemInfo();
  Serial.println("Modem Info: " + modemInfo);
}

void set_network_mode(int network_mode, int network_type) {
  // Temporarily "disable" modem
  modem.sendAT("+CFUN=0");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" +CFUN=0  false ");
  }
  delay(200);

  if (!modem.setNetworkMode(network_mode)) {
    Serial.println("Set network mode failed.");
  }
  delay(200);

  if (!modem.setPreferredMode(network_type)) {
    Serial.println("Set network type failed.");
  }
  delay(200);

  // Reset to full functionality
  modem.sendAT("+CFUN=1");
  if (modem.waitResponse(10000L) != 1) {
    DBG(" +CFUN=1  false ");
  }
  delay(200);
}

bool wait_for_network(int timeout) {
  Serial.println("Waiting for network...");
  for (int i = 0; i < timeout; i++) {
    Serial.println("Signal quality: " + String(modem.getSignalQuality()));
    if (modem.isNetworkConnected()) {
      Serial.println("Connected.");
      digitalWrite(LED_PIN, HIGH); // turn off
      return true;
    }
    digitalWrite(LED_PIN, i % 2); // new blink pattern
    delay(1000);
  }
  Serial.println("Could not connect to network.");
  return false;
}

bool connect_gprs(const char *apn, const char *gprsUser, const char *gprsPass) {
  Serial.println("Connecting GPRS");
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.print("GPRS connection failed.");
    return false;
  }

  if (modem.isGprsConnected()) {
    Serial.println("GPRS status: connected");
    return true;
  } else {
    Serial.println("GPRS status: not connected");
    return false;
  }
}

bool disconnect_gprs() {
  modem.gprsDisconnect();
  if (!modem.isGprsConnected()) {
    Serial.println("GPRS disconnected");
    return true;
  } else {
    Serial.println("GPRS disconnect: Failed.");
    return false;
  }
}

void stop_modem() {
  modem.sendAT("+CPOWD=1");
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("Modem power off failed.");
  }
  modem.poweroff();
  Serial.println("Poweroff.");
}

void print_modem_info() {
  Serial.println("CCID: " + modem.getSimCCID());
  Serial.println("IMEI: " + modem.getIMEI());
  Serial.println("Operator: " + modem.getOperator());
  Serial.println("Local IP: " + modem.localIP().toString());
  Serial.println("Signal quality: " + String(modem.getSignalQuality()));

  SerialAT.println("AT+CPSI?"); // Get connection type and band
  delay(500);
  if (SerialAT.available()) {
    String r = SerialAT.readString();
    Serial.println(r);
  }
}

void init_sd_card() {
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SDCard MOUNT FAIL");
  } else {
    uint32_t cardSize = SD.cardSize() / (1024 * 1024);
    String str = "SDCard Size: " + String(cardSize) + "MB";
    Serial.println(str);
  }
}

void enable_gps() {
  Serial.println("Enabling GPS!");

  // Set SIM7000G GPIO4 HIGH ,turn on GPS power
  // CMD:AT+SGPIO=0,4,1,1
  // Only in version 20200415 is there a function to control GPS power
  modem.sendAT("+SGPIO=0,4,1,1");
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("Setting GPS power high failed.");
  }

  modem.enableGPS();
}

void disable_gps() {
  Serial.println("Disabling GPS!");

  modem.disableGPS();

  // Set SIM7000G GPIO4 LOW ,turn off GPS power
  // CMD:AT+SGPIO=0,4,1,0
  // Only in version 20200415 is there a function to control GPS power
  modem.sendAT("+SGPIO=0,4,1,0");
  if (modem.waitResponse(10000L) != 1) {
    Serial.println("Setting GPS power low failed.");
  }
}

void set_PDP_context(String apn) {
#define NUMBER_OF_PIECES 24
  String pieces[NUMBER_OF_PIECES];
  String input;

  int counter;
  int lastIndex;

  SerialAT.println("AT+CGDCONT?");
  delay(500);
  if (SerialAT.available()) {
    input = SerialAT.readString();
    for (int i = 0; i < input.length(); i++) {
      if (input.substring(i, i + 1) == "\n") {
        pieces[counter] = input.substring(lastIndex, i);
        lastIndex = i + 1;
        counter++;
      }
      if (i == input.length() - 1) {
        pieces[counter] = input.substring(lastIndex, i);
      }
    }
    // Reset for reuse
    input = "";
    counter = 0;
    lastIndex = 0;

    for (int y = 0; y < NUMBER_OF_PIECES; y++) {
      for (int x = 0; x < pieces[y].length(); x++) {
        char c = pieces[y][x]; // gets one byte from buffer
        if (c == ',') {
          if (input.indexOf(": ") >= 0) {
            String data = input.substring((input.indexOf(": ") + 1));
            if (data.toInt() > 0 && data.toInt() < 25) {
              modem.sendAT("+CGDCONT=" + String(data.toInt()) + ",\"IP\",\"" + apn + "\",\"0.0.0.0\",0,0,0,0");
            }
            input = "";
            break;
          }
          // Reset for reuse
          input = "";
        } else {
          input += c;
        }
      }
    }
  } else {
    Serial.println("Failed to get PDP!");
  }
}

String p(int x) { // pad time
  if (x < 10) {
    return "0" + String(x);

  } else {
    return String(x);
  }
}

GPS get_gps(int timeout) {
  GPS gps;
  gps.lat = 0;
  gps.lon = 0;
  gps.valid = false;
  gps.time_str = "0000-00-00 00:00:00";

  for (int i = 0; i < timeout; i++) {
    if (modem.getGPS(&gps.lat, &gps.lon)) {
      gps.valid = true;
      break;
    }
    Serial.println("Waiting on GPS...");
    digitalWrite(LED_PIN, i % 3); // new blink pattern
    delay(1000);
  }

  int year, month, day, hour, min, sec;
  if (modem.getGPSTime(&year, &month, &day, &hour, &min, &sec)) {
    gps.time_str = p(year) + "-" + p(month) + "-" + p(day) + " " + p(hour) + ":" + p(min) + ":" + p(sec);
  } else {
    gps.valid = false;
  }
  digitalWrite(LED_PIN, HIGH); // LED off
  return gps;
}
