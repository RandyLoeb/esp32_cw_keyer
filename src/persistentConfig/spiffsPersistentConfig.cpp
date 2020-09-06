#include "FS.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "spiffsPersistentConfig.h"
#define GENERIC_CHARGRAB

#define CONFIG_JSON_FILENAME "/configuration.json"
#define FORMAT_SPIFFS_IF_FAILED true
#define SPIFFS_LOG_SERIAL false


void SpiffsPersistentConfig::makeSpiffsReady()
{
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {

        /* if (SPIFFS_LOG_SERIAL)
        {
            esp32_port_to_use->println("SPIFFS Mount Failed");
        } */
    }
    else
    {
        /* File root = SPIFFS.open("/");

        File file = root.openNextFile();

        while (file)
        {

            
            if (SPIFFS_LOG_SERIAL)
            {
                esp32_port_to_use->print("FILE: ");
            }
            
            if (SPIFFS_LOG_SERIAL)
            {
                esp32_port_to_use->println(file.name());
            }
            file = root.openNextFile();
        }
 */
        _SPIFFS_MADE_READY = 1;
    }
}
int SpiffsPersistentConfig::configFileExists()
{
    if (!_SPIFFS_MADE_READY)
    {
        makeSpiffsReady();
    }
    File file = SPIFFS.open(CONFIG_JSON_FILENAME, FILE_READ);
    int exists = 1;
    if (!file || file.isDirectory())
    {

#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_LOG_FILE_EXISTS
        Serial.println("Config file doesn't exist");
#endif
        exists = 0;
    }
    else
    {
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_LOG_FILE_EXISTS
        Serial.println("Config file was found.");
#endif
    }
    file.close();
    return exists;
}
String SpiffsPersistentConfig::getConfigFileJsonString()
{
    if (!_SPIFFS_MADE_READY)
    {
        makeSpiffsReady();
    }
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_GET_FILE_JSON
    Serial.println("About to open config file");
#endif
    File file = SPIFFS.open(CONFIG_JSON_FILENAME, FILE_READ);
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_GET_FILE_JSON
    Serial.println("config file contents:");
#endif
    String jsonReturn = "";
    while (file.available())
    {
        jsonReturn = jsonReturn + file.readString();
    }
    file.close();
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_GET_FILE_JSON
    Serial.println(jsonReturn);
#endif
    return jsonReturn;
}
void SpiffsPersistentConfig::setConfigurationFromFile()
{
    String jsonString = getConfigFileJsonString();
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_SET_CONFIG_FROM_FILE
    Serial.println(jsonString);
#endif
    //const size_t capacity = 6 * JSON_ARRAY_SIZE(2) + 4 * JSON_ARRAY_SIZE(4) + 2 * JSON_ARRAY_SIZE(5) + 2 * JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(23) + 410;
    const size_t capacity = 4096;
    DynamicJsonDocument doc(capacity);

    //const char *json = "{\"cli_mode\":1,\"ptt_buffer_hold_active\":1,\"wpm\":1,\"hz_sidetone\":1,\"dah_to_dit_ratio\":1,\"wpm_farnsworth\":1,\"memory_repeat_time\":1,\"wpm_command_mode\":1,\"link_receive_udp_port\":1,\"wpm_ps2_usb_keyboard\":1,\"wpm_cli\":1,\"wpm_winkey\":1,\"ip\":[4,4,4,4],\"gateway\":[4,4,4,4],\"subnet\":[4,4,4,4],\"link_send_ip\":[[2,2],[2,2],[2,2],[2,2]],\"link_send_enabled\":[2,2],\"link_send_udp_port\":[2,2],\"ptt_lead_time\":[6,6,6,6,6,6],\"ptt_tail_time\":[6,6,6,6,6,6],\"ptt_active_to_sequencer_active_time\":[5,5,5,5,5],\"ptt_inactive_to_sequencer_inactive_time\":[5,5,5,5,5],\"sidetone_volume\":1}";

    deserializeJson(doc, jsonString);

    configuration.cli_mode = doc["cli_mode"];                             // 1
    configuration.ptt_buffer_hold_active = doc["ptt_buffer_hold_active"]; // 1
    configuration.wpm = doc["wpm"];
    /* if (SPIFFS_LOG_SERIAL)
    {
        esp32_port_to_use->print("Configuration.wpm was just set from json to:");
        esp32_port_to_use->println(configuration.wpm);
    }      */
    // 1
    configuration.hz_sidetone = doc["hz_sidetone"];                     // 1
    configuration.dah_to_dit_ratio = doc["dah_to_dit_ratio"];           // 1
    configuration.wpm_farnsworth = doc["wpm_farnsworth"];               // 1
    configuration.memory_repeat_time = doc["memory_repeat_time"];       // 1
    configuration.wpm_command_mode = doc["wpm_command_mode"];           // 1
    configuration.link_receive_udp_port = doc["link_receive_udp_port"]; // 1
    configuration.wpm_ps2_usb_keyboard = doc["wpm_ps2_usb_keyboard"];   // 1
    configuration.wpm_cli = doc["wpm_cli"];                             // 1
    configuration.wpm_winkey = doc["wpm_winkey"];                       // 1

    /*
    JsonArray ip = doc["ip"];
    int ip_0 = ip[0]; // 4
    int ip_1 = ip[1]; // 4
    int ip_2 = ip[2]; // 4
    int ip_3 = ip[3]; // 4

    JsonArray gateway = doc["gateway"];
    int gateway_0 = gateway[0]; // 4
    int gateway_1 = gateway[1]; // 4
    int gateway_2 = gateway[2]; // 4
    int gateway_3 = gateway[3]; // 4

    JsonArray subnet = doc["subnet"];
    int subnet_0 = subnet[0]; // 4
    int subnet_1 = subnet[1]; // 4
    int subnet_2 = subnet[2]; // 4
    int subnet_3 = subnet[3]; // 4

    JsonArray link_send_ip = doc["link_send_ip"];

    int link_send_ip_0_0 = link_send_ip[0][0]; // 2
    int link_send_ip_0_1 = link_send_ip[0][1]; // 2

    int link_send_ip_1_0 = link_send_ip[1][0]; // 2
    int link_send_ip_1_1 = link_send_ip[1][1]; // 2

    int link_send_ip_2_0 = link_send_ip[2][0]; // 2
    int link_send_ip_2_1 = link_send_ip[2][1]; // 2

    int link_send_ip_3_0 = link_send_ip[3][0]; // 2
    int link_send_ip_3_1 = link_send_ip[3][1]; // 2

    int link_send_enabled_0 = doc["link_send_enabled"][0]; // 2
    int link_send_enabled_1 = doc["link_send_enabled"][1]; // 2

    int link_send_udp_port_0 = doc["link_send_udp_port"][0]; // 2
    int link_send_udp_port_1 = doc["link_send_udp_port"][1]; // 2

    JsonArray ptt_lead_time = doc["ptt_lead_time"];
    int ptt_lead_time_0 = ptt_lead_time[0]; // 6
    int ptt_lead_time_1 = ptt_lead_time[1]; // 6
    int ptt_lead_time_2 = ptt_lead_time[2]; // 6
    int ptt_lead_time_3 = ptt_lead_time[3]; // 6
    int ptt_lead_time_4 = ptt_lead_time[4]; // 6
    int ptt_lead_time_5 = ptt_lead_time[5]; // 6

    JsonArray ptt_tail_time = doc["ptt_tail_time"];
    int ptt_tail_time_0 = ptt_tail_time[0]; // 6
    int ptt_tail_time_1 = ptt_tail_time[1]; // 6
    int ptt_tail_time_2 = ptt_tail_time[2]; // 6
    int ptt_tail_time_3 = ptt_tail_time[3]; // 6
    int ptt_tail_time_4 = ptt_tail_time[4]; // 6
    int ptt_tail_time_5 = ptt_tail_time[5]; // 6

    JsonArray ptt_active_to_sequencer_active_time = doc["ptt_active_to_sequencer_active_time"];
    int ptt_active_to_sequencer_active_time_0 = ptt_active_to_sequencer_active_time[0]; // 5
    int ptt_active_to_sequencer_active_time_1 = ptt_active_to_sequencer_active_time[1]; // 5
    int ptt_active_to_sequencer_active_time_2 = ptt_active_to_sequencer_active_time[2]; // 5
    int ptt_active_to_sequencer_active_time_3 = ptt_active_to_sequencer_active_time[3]; // 5
    int ptt_active_to_sequencer_active_time_4 = ptt_active_to_sequencer_active_time[4]; // 5

    JsonArray ptt_inactive_to_sequencer_inactive_time = doc["ptt_inactive_to_sequencer_inactive_time"];
    int ptt_inactive_to_sequencer_inactive_time_0 = ptt_inactive_to_sequencer_inactive_time[0]; // 5
    int ptt_inactive_to_sequencer_inactive_time_1 = ptt_inactive_to_sequencer_inactive_time[1]; // 5
    int ptt_inactive_to_sequencer_inactive_time_2 = ptt_inactive_to_sequencer_inactive_time[2]; // 5
    int ptt_inactive_to_sequencer_inactive_time_3 = ptt_inactive_to_sequencer_inactive_time[3]; // 5
    int ptt_inactive_to_sequencer_inactive_time_4 = ptt_inactive_to_sequencer_inactive_time[4]; // 5
*/
    configuration.sidetone_volume = doc["sidetone_volume"]; // 1
}

void SpiffsPersistentConfig::writeConfigurationToFile()
{
    if (!_SPIFFS_MADE_READY)
    {
        makeSpiffsReady();
    }
    String jsonToWrite = getJsonStringFromConfiguration();
    File newfile = SPIFFS.open(CONFIG_JSON_FILENAME, FILE_WRITE);
    if (!newfile)
    {
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_WRITE_CONFIG
        Serial.println("- failed to open file for writing");
#endif
        //return;
    }
    if (newfile.print(jsonToWrite))
    {
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_WRITE_CONFIG
        Serial.println("- file written");
#endif
        newfile.close();
    }
    else
    {
#ifdef DEBUG_SPIFFS_PERSISTENT_CONFIG_WRITE_CONFIG
        Serial.println("- write failed");
#endif
    }
    delay(200);
}

void SpiffsPersistentConfig::initializeSpiffs(PRIMARY_SERIAL_CLS *port_to_use)
{
    esp32_port_to_use = port_to_use;
}

void SpiffsPersistentConfig::initialize(PRIMARY_SERIAL_CLS *loggingPortToUse)
{
    this->esp32_port_to_use = loggingPortToUse;
    Serial.println("In initialize spiffs persistent");
    if (!configFileExists())
    {
        Serial.println("did not find config file");
        writeConfigurationToFile();
    }
    else
    {
        String s = getConfigFileJsonString();
        if (s.length() == 0)
        {
            Serial.println("config string was empty, initialize");
            writeConfigurationToFile();
        }
    }

    Serial.println("setting config from file");
    setConfigurationFromFile();
}
void SpiffsPersistentConfig::save()
{
    writeConfigurationToFile();
};
