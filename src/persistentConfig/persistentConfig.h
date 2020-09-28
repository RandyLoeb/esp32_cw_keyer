#ifndef PERSISTENTCONFIG_H
#define PERSISTENTCONFIG_H
#include <Arduino.h>
#include <string>
#include <vector>
#include <queue>
#define PRIMARY_SERIAL_CLS HardwareSerial
class persistentConfig
{

public:
    persistentConfig();

    struct config_t
    { // 111 bytes total

        uint8_t paddle_mode;
        uint8_t keyer_mode;
        uint8_t sidetone_mode;
        uint8_t pot_activated;
        uint8_t length_wordspace;
        uint8_t autospace_active;
        uint8_t current_ptt_line;
        uint8_t current_tx;
        uint8_t weighting;
        uint8_t dit_buffer_off;
        uint8_t dah_buffer_off;
        uint8_t cmos_super_keyer_iambic_b_timing_percent;
        uint8_t cmos_super_keyer_iambic_b_timing_on;
        uint8_t link_receive_enabled;
        uint8_t paddle_interruption_quiet_time_element_lengths;
        uint8_t wordsworth_wordspace;
        uint8_t wordsworth_repetition;
        uint8_t cli_mode;
        uint8_t ptt_buffer_hold_active;
        // 19 bytes

        unsigned int wpm;
        unsigned int hz_sidetone;
        unsigned int dah_to_dit_ratio;
        unsigned int wpm_farnsworth;
        unsigned int wpm_farnsworth_slow;
        unsigned int memory_repeat_time;
        unsigned int wpm_command_mode;
        unsigned int link_receive_udp_port;

        unsigned int wpm_ps2_usb_keyboard;
        unsigned int wpm_cli;
        unsigned int wpm_winkey;
        // 20 bytes

        unsigned int ptt_lead_time[6];
        unsigned int ptt_tail_time[6];
        unsigned int ptt_active_to_sequencer_active_time[5];
        unsigned int ptt_inactive_to_sequencer_inactive_time[5];
        // 44 bytes

        int sidetone_volume;
        // 2 bytes
        int tx;
    } configuration;

    String IPAddress;
    String MacAddress;

    virtual void initialize(PRIMARY_SERIAL_CLS *loggingPortToUse);

    virtual void save();

    virtual void preSave()
    {
        for (std::vector<void (*)()>::iterator it = preSaveCallbacks.begin(); it != preSaveCallbacks.end(); ++it)
        {
            (*it)();
        }
    };

    virtual void postSave()
    {
        for (std::vector<void (*)()>::iterator it = postSaveCallbacks.begin(); it != postSaveCallbacks.end(); ++it)
        {
            (*it)();
        }

        while (!postSaveOneTimeCallbacks.empty())
        {
            void (*oneTime)() = postSaveOneTimeCallbacks.front();
            postSaveOneTimeCallbacks.pop();
            oneTime();
        }
    };

    String
    getJsonStringFromConfiguration();

    byte config_dirty = 0;

    std::vector<void (*)()> wpmChangeCallbacks;

    std::vector<void (*)()> preSaveCallbacks;
    std::vector<void (*)()> postSaveCallbacks;
    std::queue<void (*)()> postSaveOneTimeCallbacks;

    void setWpm(int newWpm, int newWpmFarnsworth, int newWpmFarnsworthSlow)
    {

        this->configuration.wpm = newWpm;
        if (newWpmFarnsworth >= newWpmFarnsworthSlow)
        {
            this->configuration.wpm_farnsworth = newWpmFarnsworth;
            this->configuration.wpm_farnsworth_slow = newWpmFarnsworthSlow;
        }
        for (std::vector<void (*)()>::iterator it = wpmChangeCallbacks.begin(); it != wpmChangeCallbacks.end(); ++it)
        {
            postSaveOneTimeCallbacks.push((*it));
        }
    };

    /* virtual String readFileIntoString(String fileName); */
};
#endif
