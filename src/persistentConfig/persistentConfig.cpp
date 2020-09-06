#include <Arduino.h>
#define PRIMARY_SERIAL_CLS HardwareSerial
#include "persistentConfig.h"
#include "ArduinoJson.h"

void persistentConfig::initialize(PRIMARY_SERIAL_CLS *loggingPortToUse)
{
}

void persistentConfig::save()
{
    Serial.println("in base save...bad!");
};

String
persistentConfig::getJsonStringFromConfiguration()
{
    //const size_t capacity = 6 * JSON_ARRAY_SIZE(2) + 4 * JSON_ARRAY_SIZE(4) + 2 * JSON_ARRAY_SIZE(5) + 2 * JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(23);
    const size_t capacity = 4096;
    DynamicJsonDocument doc(capacity);

    doc["cli_mode"] = configuration.cli_mode;
    doc["ptt_buffer_hold_active"] = configuration.ptt_buffer_hold_active;
    /* if (SPIFFS_LOG_SERIAL)
    {
        esp32_port_to_use->print("Configuration.wpm is about to set json to:");
        esp32_port_to_use->println(configuration.wpm);
    } */
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
