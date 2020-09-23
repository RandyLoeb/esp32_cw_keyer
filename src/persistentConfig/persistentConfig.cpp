#include <Arduino.h>
#define PRIMARY_SERIAL_CLS HardwareSerial
#include "persistentConfig.h"
#include "ArduinoJson.h"
#include "keyer_settings.h"
#include "keyer.h"
persistentConfig::persistentConfig()
{
    this->configuration.wpm = initial_speed_wpm;

    this->configuration.paddle_interruption_quiet_time_element_lengths = default_paddle_interruption_quiet_time_element_lengths;
    this->configuration.hz_sidetone = initial_sidetone_freq;
    this->configuration.memory_repeat_time = default_memory_repeat_time;
    this->configuration.cmos_super_keyer_iambic_b_timing_percent = default_cmos_super_keyer_iambic_b_timing_percent;
    this->configuration.dah_to_dit_ratio = initial_dah_to_dit_ratio;
    this->configuration.length_wordspace = default_length_wordspace;
    this->configuration.weighting = default_weighting;
    this->configuration.wordsworth_wordspace = default_wordsworth_wordspace;
    this->configuration.wordsworth_repetition = default_wordsworth_repetition;
    this->configuration.wpm_farnsworth = initial_speed_wpm;
    this->configuration.cli_mode = CLI_NORMAL_MODE;
    this->configuration.wpm_command_mode = initial_command_mode_speed_wpm;
    this->configuration.ptt_buffer_hold_active = 0;
    this->configuration.sidetone_volume = sidetone_volume_low_limit + ((sidetone_volume_high_limit - sidetone_volume_low_limit) / 2);

    this->configuration.ptt_lead_time[0] = initial_ptt_lead_time_tx1;
    this->configuration.ptt_tail_time[0] = initial_ptt_tail_time_tx1;
    this->configuration.ptt_lead_time[1] = initial_ptt_lead_time_tx2;
    this->configuration.ptt_tail_time[1] = initial_ptt_tail_time_tx2;

    this->configuration.ptt_lead_time[2] = initial_ptt_lead_time_tx3;
    this->configuration.ptt_tail_time[2] = initial_ptt_tail_time_tx3;
    this->configuration.ptt_lead_time[3] = initial_ptt_lead_time_tx4;
    this->configuration.ptt_tail_time[3] = initial_ptt_tail_time_tx4;
    this->configuration.ptt_lead_time[4] = initial_ptt_lead_time_tx5;
    this->configuration.ptt_tail_time[4] = initial_ptt_tail_time_tx5;
    this->configuration.ptt_lead_time[5] = initial_ptt_lead_time_tx6;
    this->configuration.ptt_tail_time[5] = initial_ptt_tail_time_tx6;

    for (int x = 0; x < 5; x++)
    {
        this->configuration.ptt_active_to_sequencer_active_time[x] = 0;
        this->configuration.ptt_inactive_to_sequencer_inactive_time[x] = 0;
    }
    this->configuration.paddle_mode = PADDLE_NORMAL;
    this->configuration.keyer_mode = IAMBIC_B;
    this->configuration.sidetone_mode = SIDETONE_ON;

#ifdef initial_sidetone_mode
    this->configuration.sidetone_mode = initial_sidetone_mode;
#endif
}

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
    doc["wpm_farnsworth_slow"] = configuration.wpm_farnsworth_slow;
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

    doc["ip"] = this->IPAddress;
    doc["mac"] = this->MacAddress;
    doc["tx"] = this->configuration.tx;
    String returnString;
    serializeJson(doc, returnString);

    return returnString;
}
