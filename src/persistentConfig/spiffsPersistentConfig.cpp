#include "FS.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "persistentConfig.h"
#define GENERIC_CHARGRAB

#define CONFIG_JSON_FILENAME "/configuration.json"
#define FORMAT_SPIFFS_IF_FAILED true
#define SPIFFS_LOG_SERIAL false

//

class SpiffsPersistentConfig : public persistentConfig
{
    int _SPIFFS_MADE_READY = 0;
    PRIMARY_SERIAL_CLS *esp32_port_to_use;

    void makeSpiffsReady()
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
    int configFileExists()
    {
        if (!_SPIFFS_MADE_READY)
        {
            makeSpiffsReady();
        }
        File file = SPIFFS.open(CONFIG_JSON_FILENAME, FILE_READ);
        int exists = 1;
        if (!file || file.isDirectory())
        {
            /* if (SPIFFS_LOG_SERIAL)
        {
            esp32_port_to_use->println("Config file doesn't exist");
        } */
            exists = 0;
        }
        else
        {
            /* if (SPIFFS_LOG_SERIAL)
        {
            esp32_port_to_use->println("Config file was found.");
        } */
        }
        file.close();
        return exists;
    }
    String getConfigFileJsonString()
    {
        if (!_SPIFFS_MADE_READY)
        {
            makeSpiffsReady();
        }
        File file = SPIFFS.open(CONFIG_JSON_FILENAME, FILE_READ);
        String jsonReturn;
        while (file.available())
        {
            jsonReturn = jsonReturn + file.readString();
        }
        file.close();
        return jsonReturn;
    }
    void setConfigurationFromFile()
    {
        String jsonString = getConfigFileJsonString();
        //const size_t capacity = 6 * JSON_ARRAY_SIZE(2) + 4 * JSON_ARRAY_SIZE(4) + 2 * JSON_ARRAY_SIZE(5) + 2 * JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(23) + 410;
        const size_t capacity = 4096;
        DynamicJsonDocument doc(capacity);

        //const char *json = "{\"cli_mode\":1,\"ptt_buffer_hold_active\":1,\"wpm\":1,\"hz_sidetone\":1,\"dah_to_dit_ratio\":1,\"wpm_farnsworth\":1,\"memory_repeat_time\":1,\"wpm_command_mode\":1,\"link_receive_udp_port\":1,\"wpm_ps2_usb_keyboard\":1,\"wpm_cli\":1,\"wpm_winkey\":1,\"ip\":[4,4,4,4],\"gateway\":[4,4,4,4],\"subnet\":[4,4,4,4],\"link_send_ip\":[[2,2],[2,2],[2,2],[2,2]],\"link_send_enabled\":[2,2],\"link_send_udp_port\":[2,2],\"ptt_lead_time\":[6,6,6,6,6,6],\"ptt_tail_time\":[6,6,6,6,6,6],\"ptt_active_to_sequencer_active_time\":[5,5,5,5,5],\"ptt_inactive_to_sequencer_inactive_time\":[5,5,5,5,5],\"sidetone_volume\":1}";

        deserializeJson(doc, jsonString);

        configuration.cli_mode = doc["cli_mode"];                             // 1
        configuration.ptt_buffer_hold_active = doc["ptt_buffer_hold_active"]; // 1
        configuration.wpm = doc["wpm"];
        if (SPIFFS_LOG_SERIAL)
        {
            esp32_port_to_use->print("Configuration.wpm was just set from json to:");
            esp32_port_to_use->println(configuration.wpm);
        }                                                                   // 1
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

    String
    getJsonStringFromConfiguration()
    {
        //const size_t capacity = 6 * JSON_ARRAY_SIZE(2) + 4 * JSON_ARRAY_SIZE(4) + 2 * JSON_ARRAY_SIZE(5) + 2 * JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(23);
        const size_t capacity = 4096;
        DynamicJsonDocument doc(capacity);

        doc["cli_mode"] = configuration.cli_mode;
        doc["ptt_buffer_hold_active"] = configuration.ptt_buffer_hold_active;
        if (SPIFFS_LOG_SERIAL)
        {
            esp32_port_to_use->print("Configuration.wpm is about to set json to:");
            esp32_port_to_use->println(configuration.wpm);
        }
        doc["wpm"] = configuration.wpm;
        doc["hz_sidetone"] = configuration.hz_sidetone;
        doc["dah_to_dit_ratio"] = configuration.dah_to_dit_ratio;
        doc["wpm_farnsworth"] = configuration.wpm_farnsworth;
        doc["memory_repeat_time"] = configuration.memory_repeat_time;
        doc["wpm_command_mode"] = configuration.wpm_command_mode;
        doc["link_receive_udp_port"] = configuration.link_receive_udp_port;
        doc["wpm_ps2_usb_keyboard"] = configuration.wpm_ps2_usb_keyboard;
        doc["wpm_cli"] = configuration.wpm_cli;
        doc["wpm_winkey"] = configuration.wpm_winkey;

        /*
    JsonArray ip = doc.createNestedArray("ip");
    ip.add(4);
    ip.add(4);
    ip.add(4);
    ip.add(4);

    JsonArray gateway = doc.createNestedArray("gateway");
    gateway.add(4);
    gateway.add(4);
    gateway.add(4);
    gateway.add(4);

    JsonArray subnet = doc.createNestedArray("subnet");
    subnet.add(4);
    subnet.add(4);
    subnet.add(4);
    subnet.add(4);

    JsonArray link_send_ip = doc.createNestedArray("link_send_ip");

    JsonArray link_send_ip_0 = link_send_ip.createNestedArray();
    link_send_ip_0.add(2);
    link_send_ip_0.add(2);

    JsonArray link_send_ip_1 = link_send_ip.createNestedArray();
    link_send_ip_1.add(2);
    link_send_ip_1.add(2);

    JsonArray link_send_ip_2 = link_send_ip.createNestedArray();
    link_send_ip_2.add(2);
    link_send_ip_2.add(2);

    JsonArray link_send_ip_3 = link_send_ip.createNestedArray();
    link_send_ip_3.add(2);
    link_send_ip_3.add(2);

    JsonArray link_send_enabled = doc.createNestedArray("link_send_enabled");
    link_send_enabled.add(2);
    link_send_enabled.add(2);

    JsonArray link_send_udp_port = doc.createNestedArray("link_send_udp_port");
    link_send_udp_port.add(2);
    link_send_udp_port.add(2);

    JsonArray ptt_lead_time = doc.createNestedArray("ptt_lead_time");
    ptt_lead_time.add(6);
    ptt_lead_time.add(6);
    ptt_lead_time.add(6);
    ptt_lead_time.add(6);
    ptt_lead_time.add(6);
    ptt_lead_time.add(6);

    JsonArray ptt_tail_time = doc.createNestedArray("ptt_tail_time");
    ptt_tail_time.add(6);
    ptt_tail_time.add(6);
    ptt_tail_time.add(6);
    ptt_tail_time.add(6);
    ptt_tail_time.add(6);
    ptt_tail_time.add(6);

    JsonArray ptt_active_to_sequencer_active_time = doc.createNestedArray("ptt_active_to_sequencer_active_time");
    ptt_active_to_sequencer_active_time.add(5);
    ptt_active_to_sequencer_active_time.add(5);
    ptt_active_to_sequencer_active_time.add(5);
    ptt_active_to_sequencer_active_time.add(5);
    ptt_active_to_sequencer_active_time.add(5);

    JsonArray ptt_inactive_to_sequencer_inactive_time = doc.createNestedArray("ptt_inactive_to_sequencer_inactive_time");
    ptt_inactive_to_sequencer_inactive_time.add(5);
    ptt_inactive_to_sequencer_inactive_time.add(5);
    ptt_inactive_to_sequencer_inactive_time.add(5);
    ptt_inactive_to_sequencer_inactive_time.add(5);
    ptt_inactive_to_sequencer_inactive_time.add(5);
    */
        doc["sidetone_volume"] = configuration.sidetone_volume;

        String returnString;
        serializeJson(doc, returnString);

        return returnString;
    }

    void writeConfigurationToFile()
    {
        if (!_SPIFFS_MADE_READY)
        {
            makeSpiffsReady();
        }
        String jsonToWrite = getJsonStringFromConfiguration();
        File newfile = SPIFFS.open(CONFIG_JSON_FILENAME, FILE_WRITE);
        if (!newfile)
        {
            if (SPIFFS_LOG_SERIAL)
            {
                esp32_port_to_use->println("- failed to open file for writing");
            }
            //return;
        }
        if (newfile.print(jsonToWrite))
        {
            if (SPIFFS_LOG_SERIAL)
            {
                esp32_port_to_use->println("- file written");
            }
            newfile.close();
        }
        else
        {
            if (SPIFFS_LOG_SERIAL)
            {
                esp32_port_to_use->println("- write failed");
            }
        }
        delay(200);
    }

    void initializeSpiffs(PRIMARY_SERIAL_CLS *port_to_use)
    {
        esp32_port_to_use = port_to_use;
    }

public:
    virtual void initialize(PRIMARY_SERIAL_CLS *loggingPortToUse)
    {
        esp32_port_to_use = loggingPortToUse;
        if (!configFileExists())
        {
            writeConfigurationToFile();
        }

        setConfigurationFromFile();
    }
    virtual void save()
    {
        writeConfigurationToFile();
    };
};
