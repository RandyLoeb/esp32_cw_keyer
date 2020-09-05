
#ifndef WIFIUTILS_H
#define WIFIUTILS_H
#include <Arduino.h>
#include <vector>
#include <string>
#include <ArduinoJson.h>
#include <WiFi.h>      // Include the Wi-Fi library
#include <DNSServer.h> //Local DNS Server used for redirecting all requests to the configuration portal
#include "SPIFFS.h"
#define WIFI_FILE_NAME "/wifi.json"

class WifiUtils
{

    byte DNS_PORT;

    DNSServer dnsServer;

    DynamicJsonDocument _WIFI_CONFIG_JSON;
    // extract the AP names from listPlus vector that has ap, rssi, isOpen - Paul
    std::vector<String> extractAPnames(std::vector<String> listPlus);

    std::vector<String> getLocalWifiAps();

    int connectToWifi(char *apName, char *pass);

    bool tryWifis();

    void disconnectWiFi();

    void writeToApFile(String out);

    String getEmptyConfigString();

    void initWifiFile();

    File getConfigFile();

    void initializeGlobalJsonConfig(String strJson);

    void readConfigFileIntoGlobalJson(File file);

    JsonArray getApsStored();

    String _ipAddr;
    String _apName;

    void startupWifiConfigSites();
    void writeJsonConfigToFile(DynamicJsonDocument doc);

public:
    WifiUtils() : _WIFI_CONFIG_JSON(4096)
    {
        this->_ipAddr = "";
        this->_apName = "";
        this->DNS_PORT = 53;
    };
    void initialize();
    String getIp()
    {
        return this->_ipAddr;
    }

    String getApName()
    {
        return this->_apName;
    }

    String getWiFiScan();
    void updateAp(String jsonIn);
    void forgetAp(String jsonIn);
    void processNextDNSRequest();
    void showJson();
};
#endif
