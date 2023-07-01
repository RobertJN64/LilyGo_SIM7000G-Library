#ifndef LilyGo_SIM7000G_H
#define LilyGo_SIM7000G_H

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS 13

#define UART_BAUD 115200

#define PIN_TX 27
#define PIN_RX 26
#define PWR_PIN 4
#define LED_PIN 12
#define PIN_DTR 25 // what is this for?

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <SPI.h>
#include <SD.h>

extern TinyGsm modem;

// start modem, restart takes longer so is disabled by default
void start_modem(bool restart = false);

#define NETWORK_MODE_AUTOMATIC 2
#define NETWORK_MODE_GSM_ONLY 13
#define NETWORK_MODE_LTE_ONLY 38
#define NETWORK_MODE_GSM_AND_LTE 51

#define NETWORK_TYPE_CAT_M 1
#define NETWORK_TYPE_NB_IOT 2
#define NETWORK_TYPE_CAT_M_AND_NB_IOT 3

// defaults to automatic and all network types, can be restricted to LTE or GSM only
void set_network_mode(int network_mode = NETWORK_MODE_AUTOMATIC, int network_type = NETWORK_TYPE_CAT_M_AND_NB_IOT);

// Wait for network while printing signal strength, timeout is in seconds
bool wait_for_network(int timeout = 60);

// Connect to General Packet Radio Service - apn, user, and pass may need to be specified for your carrier
bool connect_gprs(const char *apn = "", const char *gprsUser = "", const char *gprsPass = "");

bool disconnect_gprs();

// powerdown modem
void stop_modem();

// print CCID, IMEI, etc.
void print_modem_info();

void init_sd_card();

void enable_gps();

void disable_gps();

// This is only neccessary on first boot?
void set_PDP_context(String apn = "");

struct GPS {
  bool valid;
  float lat;
  float lon;
  String time_str;
};

// Get lat, lon, and time (YYYY-MM-DD HH:MM:SS), returns lat = 0, lon = 0 if fails
GPS get_gps(int timeout = 10);

#endif