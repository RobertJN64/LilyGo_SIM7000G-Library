# LilyGo_SIM7000G

Library with functions for managing the cell modem and GPS on the ESP32 LilyGo SIM7000G board.

Based on [AllFunctions.ino](https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7000G/blob/master/examples/Arduino_TinyGSM/AllFunctions/AllFunctions.ino) in the official LilyGO-T-SIM7000G repo.

## Usage
Place the `LilyGo_SIM700G` folder in the `lib` folder of a platform.io project.
`#include <LilyGo_SIM7000G.hpp>` at the top of your file.
If you need access to the modem or SerialAT directly, just use those keywords in your program.

## Example Startup Sequence
```cpp
#include <LilyGo_SIM7000G.hpp>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Serial connected!");

  // Set LED OFF
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  start_modem();
  set_network_mode();
  set_PDP_context();
  enable_gps();

  if (!wait_for_network()) {
    //handle failure
  }

  if (!connect_gprs()) {
    //handle failure
  }

  print_modem_info();
}
```

## Debugging
`#define DEBUG_AT` to enable StreamDebugger to send all AT cmds to the serial monitor. Requires the StreamDebugger library.

## Networking (with HTTPS)
```cpp
#include <LilyGo_SIM7000G.hpp>
#include <ArduinoHttpClient.h>
#include <SSLClient.h> //Uses SSLClient lib https://github.com/govorox/SSLClient (not distributed here because GPL)

#define SERVER "example.com"
#define PATH "/"
#define PORT 443

TinyGsmClient http_client(modem);     // handles networking
SSLClient https_client(&http_client); // handles ssl + https

void setup() {} //see above

void loop() {
  HttpClient http = HttpClient(https_client, SERVER, PORT);
  http.connectionKeepAlive(); // needed for https
  
  Serial.println(http.get(PATH));
  Serial.print("HTTP STATUS: ");
  Serial.println(http.responseStatusCode());
  Serial.print("HTTP RESP: ");
  Serial.println(http.responseBody());
  Serial.println();

  delay(30 * 1000);
}
```

If you don't need HTTPS, remove the https_client and use http_client instead.

## GPS
```cpp
enable_gps();
GPS gps = get_gps();
Serial.println(String(gps.lat, 6));
Serial.println(String(gps.lon, 6));
Serial.println(gps.time_str); //YYYY-MM-DD HH:MM:SS
```