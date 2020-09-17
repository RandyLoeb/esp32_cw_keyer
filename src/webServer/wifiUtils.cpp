#include "wifiUtils.h"
#include <Arduino.h>
#include <vector>
#include <string>
#include <ArduinoJson.h>
#include <WiFi.h> // Include the Wi-Fi library
#include "SPIFFS.h"

// extract the AP names from listPlus vector that has ap, rssi, isOpen - Paul
std::vector<String> WifiUtils::extractAPnames(std::vector<String> listPlus)
{
    std::vector<String> apList;
    String apname;

    int lpsize = listPlus.size();

    for (int i = 0; i < lpsize; i++)
    {
        if (i % 3 == 0)
        {
            apname = listPlus.at(i);
            apList.push_back(apname);
        }
    }

    return apList;
}

std::vector<String> WifiUtils::getLocalWifiAps()
{
    std::vector<String> list;
    std::vector<String> testlist;

    // listPlus augments list with extra info for scanned APs to display -- signal strength, open or closed network
    std::vector<String> listPlus;
    std::vector<int> RSSI;
    std::vector<int> isOpen;

    Serial.println("scan start");
    //String list = "";
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
    {
        Serial.println("no networks found");
    }
    else
    {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i)
        {

            list.push_back(WiFi.SSID(i));

            RSSI.push_back(WiFi.RSSI(i));
            isOpen.push_back((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? true : false);

            listPlus.push_back(WiFi.SSID(i));
            int iRSSI = WiFi.RSSI(i);
            String RSSIstring = String(iRSSI);
            listPlus.push_back(RSSIstring);
            listPlus.push_back((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "true" : "false");

            //delay(10);
        }
    }

    testlist = extractAPnames(listPlus);

    for (int i = 0; i < listPlus.size(); i++)
    {
        Serial.println(listPlus.at(i));
    }
    return listPlus;
}

JsonArray WifiUtils::getApsStored()
{

    return this->_WIFI_CONFIG_JSON["aps"];
}

String WifiUtils::getWiFiScan()
{

    std::vector<String> listPlus = getLocalWifiAps();
    std::vector<String> scannedAps = extractAPnames(listPlus);
    std::vector<String> knownAps;

    String outStr = "";

    JsonArray apsStored;
    int apsCount;
    apsStored = getApsStored();
    Serial.print("Aps count:");
    Serial.println(apsStored.size());
    apsCount = apsStored.size();
    String knownApJsonList = "";

    if (apsCount > 0)
    {
        for (int i = 0; i < apsCount; i++)
        {
            String tempKnown = apsStored[i]["ap"];
            knownApJsonList += knownApJsonList.length() > 0 ? "," : "";
            knownApJsonList += "\"" + tempKnown + "\"";
            knownAps.push_back(tempKnown);
        }
    }

    for (int i = 0; i < scannedAps.size(); i++)
    {
        String apName = scannedAps.at(i);
        outStr += (outStr.length() > 0 ? "," : "");

        outStr += "{";

        outStr += "\"name\":";
        outStr += "\"" + apName + "\"";
        outStr += ",\"isKnown\":";
        int isKnown = 0;
        if (apsCount > 0)
        {
            for (int i = 0; i < apsCount; i++)
            {

                String aptemp = knownAps.at(i);
                Serial.print("apTemp:");
                Serial.println(aptemp);
                Serial.print("apName:");
                Serial.println(apName);
                Serial.print("i:");
                Serial.println(i);

                if (aptemp == apName)
                {
                    isKnown = 1;
                }
            }
        }

        outStr += isKnown == 1 ? "true" : "false";

        outStr += ", \"RSSI\":";
        outStr += listPlus.at(i * 3 + 1);
        outStr += ", \"isOpen\":";
        outStr += listPlus.at(i * 3 + 2);
        outStr += "}";
    }

    return "{\"aps\":[" + outStr + "], \"knownAps\":[" + knownApJsonList + "]}";
    // Serial.println("{\"aps\":[" + outStr + "], \"knownAps\":[" + knownApJsonList + "]}"); // - Paul for debugging
    //server.send(200, "application/json", "{\"aps\":[" + outStr + "], \"knownAps\":[" + knownApJsonList + "]}");
}

int WifiUtils::connectToWifi(char *apName, char *pass)
{

    //dnsServer.stop();

    //WiFi.softAPdisconnect();
    //dnsServer.stop();
    //server.stop();
    WiFi.mode(WIFI_STA);
    delay(500);
    Serial.print("About to connect with:");
    Serial.print(apName);
    Serial.println(" ");
    Serial.println(pass);
    WiFi.begin(apName, pass); // Connect to the network

    Serial.println("Connecting from within connectToWifi...");
    int delayCount = 0;
    int delayThresh = 30;

    while (WiFi.status() != WL_CONNECTED && delayCount <= delayThresh)
    { // Wait for the Wi-Fi to connect
        delay(1000);
        Serial.print('.');
        delayCount++;
    }

    if (delayCount <= delayThresh)
    {

        //Serial.print("Wifi sleep:");
        return 1;
    }
    else
    {

        return 0;
    }
}

bool WifiUtils::tryWifis()
{

    //scan for what's available
    std::vector<String> seenAps = extractAPnames(getLocalWifiAps());
    int seenApsCount = seenAps.size();

    int j = 0;
    int connected = 0;

    while (j < seenApsCount && connected == 0)
    {
        String apToTry = seenAps.at(j);
        String pwdToTry = "";
        JsonArray apsStored = this->_WIFI_CONFIG_JSON["aps"].as<JsonArray>();

        int apsCount = apsStored.size();

        int i = 0;
        int found = 0;

        while (i < apsCount && found == 0)
        {
            String aptemp = apsStored[i]["ap"];
            String pwdtemp = apsStored[i]["pwd"];
#ifdef SABOTAGE_WIFI_PWD
            pwdtemp = "SABOTAGED";
#endif
            pwdToTry = pwdtemp;
            if (aptemp == apToTry)
            {
                found = 1;
            }
            i++;
        }

        if (found == 1)
        {
            Serial.print("ap and pwd were: ");
            Serial.print(apToTry);
            Serial.print(" ");
            Serial.println(pwdToTry);
            char apx[50];
            char passx[50];
            apToTry.toCharArray(apx, apToTry.length() + 1);
            pwdToTry.toCharArray(passx, pwdToTry.length() + 1);

            connected = connectToWifi(apx, passx);
            if (connected)
            {
                this->_ipAddr = WiFi.localIP().toString();
                this->_apName = apToTry;
            }
        }
        j++;
    }

    if (connected == 0)
    {

        Serial.println("No Connection established");
    }
    else
    {
        Serial.println('\n');
        Serial.println("Connection established!");
        Serial.print("IP address:\t");
        Serial.println(WiFi.localIP());
    }

    return connected;
}

void WifiUtils::disconnectWiFi()
{
    WiFi.disconnect(true, false);
    delay(1);
    WiFi.mode(WIFI_OFF);
    delay(1);
    btStop();
    delay(1);
}

void WifiUtils::writeToApFile(String out)
{
    File newfile = SPIFFS.open(WIFI_FILE_NAME, FILE_WRITE);
    if (!newfile)
    {
        Serial.println("- failed to open file for writing");
        //return;
    }
    if (newfile.print(out))
    {
        Serial.println("- file written");
        newfile.close();
    }
    else
    {
        Serial.println("- write failed");
    }
    delay(200);
}

String WifiUtils::getEmptyConfigString()
{
    String outstr;
    DynamicJsonDocument doc(1024);
    DynamicJsonDocument docEmpty(1024);
    docEmpty.to<JsonArray>();
    doc["aps"] = docEmpty;
    serializeJsonPretty(doc, outstr);
    // Serial.print("BytesWritten:"); Serial.println(bytesWritten);
    return outstr;
}

void WifiUtils::initWifiFile()
{
    writeToApFile(getEmptyConfigString());

    delay(2000);
    //SPIFFS.end();
    //ESP.restart();
}

File WifiUtils::getConfigFile()
{
    SPIFFS.begin(false);
    return SPIFFS.open(WIFI_FILE_NAME, FILE_READ);
}

void WifiUtils::initializeGlobalJsonConfig(String strJson)
{
    deserializeJson(this->_WIFI_CONFIG_JSON, strJson);
    String outstr;
    serializeJsonPretty(this->_WIFI_CONFIG_JSON, outstr);
    Serial.println("now the json variable:");
    Serial.println(outstr);
}

void WifiUtils::readConfigFileIntoGlobalJson(File file)
{
    String wifiJson;
    while (file.available())
    {
        wifiJson = wifiJson + file.readString();
        Serial.println(wifiJson);
    }
    file.close();

    this->initializeGlobalJsonConfig(wifiJson);
}

void WifiUtils::initialize()
{

    File file = this->getConfigFile();
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open wifi file for reading");
        this->initWifiFile();
        file = this->getConfigFile();
    }

    this->readConfigFileIntoGlobalJson(file);
    bool wifiResult = this->tryWifis();
    if (!wifiResult)
    {
        startupWifiConfigSites();
    }
}

void WifiUtils::startupWifiConfigSites()
{

    WiFi.mode(WIFI_AP);
    WiFi.softAP("K3NG Keyer");

    Serial.print("WIFI AP IP address:\t");
    //Serial.println(WiFi.localIP());
    Serial.println(WiFi.softAPIP());
    this->_ipAddr = String(WiFi.softAPIP());
    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    this->dnsServer.start(this->DNS_PORT, "*", WiFi.softAPIP());
}

void WifiUtils::processNextDNSRequest()
{
    this->dnsServer.processNextRequest();
}

void WifiUtils::updateAp(String jsonIn)
{

    String outstr;
    serializeJsonPretty(this->_WIFI_CONFIG_JSON, outstr);
    Serial.println("in update ap:");
    Serial.println(outstr);
    //_MS_SINCE_LAST_WEB = 0;
    //Serial.println(server.args());
    //Serial.println(server.arg(0));
    DynamicJsonDocument jBuffer(4096);
    deserializeJson(jBuffer, jsonIn);

    String apName = jBuffer["ap"];
    String pass = jBuffer["pass"];

    Serial.println("got apname as: " + apName);

    int apsCount;
    JsonArray apsStored = this->_WIFI_CONFIG_JSON["aps"].as<JsonArray>();
    Serial.print("Aps count:");
    Serial.println(apsStored.size());
    apsCount = apsStored.size();

    bool replaced = false;

    // if we have it just replace the password
    for (int i = 0; i < apsCount; i++)
    {

        if (apsStored[i]["ap"] == apName)
        {
            apsStored[i]["pwd"] = pass;
            replaced = true;
        }
    }

    // if there was no update above then we're adding new
    if (!replaced)
    {
        apsStored[apsCount + 1]["ap"] = apName;
        apsStored[apsCount + 1]["pwd"] = pass;
    }

    serializeJsonPretty(this->_WIFI_CONFIG_JSON, outstr);
    Serial.println("in update ap2:");
    Serial.println(outstr);
    Serial.print("saving wifi:");

    // not 100% sure apsStored is not just a copy that needs to be set
    // back to global object, so if this isn't saving right might need
    // to check
    this->writeJsonConfigToFile(this->_WIFI_CONFIG_JSON);
}

void WifiUtils::forgetAp(String jsonIn)
{
    //_MS_SINCE_LAST_WEB = 0;
    //Serial.println(server.args());
    //Serial.println(server.arg(0));
    DynamicJsonDocument jBuffer(4096);
    deserializeJson(jBuffer, jsonIn);

    String apName = jBuffer["ap"];

    JsonArray apsStored;
    int apsCount;
    apsStored = getApsStored();

    apsCount = apsStored.size();

    int target = -1;
    for (int i = 0; i < apsCount; i++)
    {

        String apOut = apsStored[i]["ap"];

        if (apOut == apName)
        {
            target = i;
            break;
        }
    }

    if (target > 0)
    {
        apsStored.remove(target);
    }

    Serial.print("saving wifi:");
    // not 100% sure apsStored is not just a copy that needs to be set
    // back to global object, so if this isn't saving right might need
    // to check
    this->writeJsonConfigToFile(this->_WIFI_CONFIG_JSON);
    //server.send(200, "text/html", "ok");
}

void WifiUtils::writeJsonConfigToFile(DynamicJsonDocument doc)
{
    String outstr;
    serializeJsonPretty(doc, outstr);
    Serial.println("in writejsonconfigtofile");
    Serial.println(outstr);
    if (outstr != "null")
    {
        this->writeToApFile(outstr);
        this->readConfigFileIntoGlobalJson(getConfigFile());
    }
    else
    {
        Serial.println("Something wrong in writejsonconfigtofile...it was null");
    }
}

void WifiUtils::showJson()
{
    String outstr;
    serializeJsonPretty(this->_WIFI_CONFIG_JSON, outstr);
    Serial.println("now the json variable:");
    Serial.println(outstr);
}