#include <esp_now.h>
#include "WiFi.h"
//A4:CF:12:75:BB:68
#define SLAVE_ESPNOW
// Structure example to receive data
// Must match the sender structure
#define ESPNOW_CHARBUFSIZE 32
#define ESPNOW_DIT true
#define ESPNOW_DAH false
#define ESPNOW_DITDAHMODE

bool ESPNOW_DIT_BUFF;
bool ESPNOW_DAH_BUFF;
bool ESPNOW_ISSENDING;

typedef struct struct_message
{
    char charactersToSend[ESPNOW_CHARBUFSIZE];
    int wpm;
    float c;
    String d;
    bool ditdahmode;
    bool ditdah;
} struct_message;

// Create a struct_message called myData
struct_message myData;
// REPLACE WITH YOUR RECEIVER MAC Address
//A4:CF:12:75:BB:68
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//24:6F:28:0B:A6:20
uint8_t broadcastAddress[] = {0xA4, 0xCF, 0x12, 0x75, 0xBB, 0x68};
void (*_speedCallback)(int);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    //Serial.print("\r\nLast Packet Send Status:\t");
    //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    ESPNOW_ISSENDING = false;
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    memcpy(&myData, incomingData, sizeof(myData));
    /*Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Char: ");
    Serial.println(myData.charactersToSend);
    Serial.print("Int: ");
    Serial.println(myData.wpm);
    Serial.print("Float: ");
    Serial.println(myData.c);
    Serial.print("String: ");
    Serial.println(myData.d);
    Serial.print("Bool: ");
    Serial.println(myData.e);
    Serial.println();*/

    if (myData.wpm != configuration.wpm)
    {
        _speedCallback(myData.wpm);
    }
    //for (int i = 0; i <= (ESPNOW_CHARBUFSIZE - 1); i++)
    //{
    if (!myData.ditdahmode)
    {
        if (myData.charactersToSend[0] != NULL)
        {
            String temp = String(myData.charactersToSend[0]);
            String allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            if (allowed.indexOf(temp) >= 0)
            {
                add_to_send_buffer(myData.charactersToSend[0]);
            }
        }
    }
    else
    {
        if (myData.ditdah == ESPNOW_DIT)
        {
            //Serial.println("got a dit!");
            ESPNOW_DIT_BUFF = true;
        }
        else
        {
            ESPNOW_DAH_BUFF = true;
        }
    }
    //}
}

void initialize_espnow_wireless(void (*speedCallback)(int))
{
    _speedCallback = speedCallback;
    WiFi.mode(WIFI_MODE_STA);
    Serial.println(WiFi.macAddress());

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

#ifndef SLAVE_ESPNOW
    // Register peer
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return;
    }
#endif
    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
    esp_now_register_send_cb(OnDataSent);

    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info
    esp_now_register_recv_cb(OnDataRecv);
}

void sendEspNowDitDah(bool ditdah)
{
//#ifdef ESPNOW_DITDAHMODE
#ifndef SLAVE_ESPNOW

    strcpy(myData.charactersToSend, "");
    myData.wpm = configuration.wpm;
    myData.c = 1.2;
    myData.d = "Hello";
    myData.ditdahmode = true;
    myData.ditdah = ditdah;
    //Serial.println(ditdah);
    //while (ESPNOW_ISSENDING)
    //{
    //    delay(1);
    //}
    // Send message via ESP-NOW
    //ESPNOW_ISSENDING = true;
    //Serial.println("about to send ditdah...");
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    //while (ESPNOW_ISSENDING)
    //{
    //    delay(1);
    //}
    if (result == ESP_OK)
    {
        //Serial.println("Sent with success");
    }
    else
    {
        //Serial.println("Error sending the data");
    }

#endif
    //#endif
}

void sendEspNowData(char character)
{
    //sendEspNowDitDah(ESPNOW_DIT);

#ifndef ESPNOW_DITDAHMODE
#ifndef SLAVE_ESPNOW
    if (String(character) != " ")
    {
        char singleBuf[1];
        singleBuf[0] = character;
        // put your main code here, to run repeatedly:
        // Set values to send
        strcpy(myData.charactersToSend, singleBuf);
        myData.wpm = configuration.wpm;
        myData.c = 1.2;
        myData.d = "Hello";
        myData.ditdahmode = false;
        myData.ditdah = false;

        // Send message via ESP-NOW
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

        if (result == ESP_OK)
        {
            Serial.println("Sent with success");
        }
        else
        {
            Serial.println("Error sending the data");
        }
    }
#endif
#endif
}

int getEspNowBuff(bool ditdah)
{

    if (ditdah == ESPNOW_DIT && ESPNOW_DIT_BUFF)
    {
        ESPNOW_DIT_BUFF = false;
        return 0;
    }
    else if (ditdah == ESPNOW_DAH && ESPNOW_DAH_BUFF)
    {
        ESPNOW_DAH_BUFF = false;
        return 0;
    }

    return 1;
}