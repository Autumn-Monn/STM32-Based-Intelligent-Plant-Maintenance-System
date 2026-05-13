#ifndef __ESP8266_H__
#define __ESP8266_H__

#define WIFI_NAME         "STARD-WIFI"
#define WIFI_PASSWORD     "www123456"

void ESP8266_Init();
void MQTT_Init();
void MQTT_Publish_Data(float temperature,float humidity);
uint8_t MQTT_Get_Data(char *name,char *data1,char *data2);
uint8_t ESP_WaitResp(char *wait_str,uint16_t timeout_ms);

extern uint8_t ESP8266_Init_Success;
extern uint8_t MQTT_Init_Success;

#endif