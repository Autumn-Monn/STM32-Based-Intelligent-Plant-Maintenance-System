#include "stm32f10x.h"                  // Device header
#include "Serial.h"
#include "Delay.h"
#include "OLED.h"
#include "esp8266.h"
#include <string.h>

uint8_t ESP8266_Init_Success=0;//esp-01s初始化成功标志位
uint8_t MQTT_Init_Success=0;//MQTT初始化成功标志位

/*ESP8266初始化
*/
void ESP8266_Init()
{
	Serial_Init(1);
	Serial_Init(2);
	OLED_Init();
	Delay_ms(500);
	
	/*1:AT测试*/
	for (uint8_t i=0;i<=1;i++)//发送两次AT，确保成功
	{
		USART2_RX_LEN=0;
		memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
		Serial_SendString(2,"AT\r\n");
		Serial_SendString(1,"AT测试\r\n");
		OLED_Clear();
		OLED_ShowString(0,0,"AT",OLED_8X16);
		OLED_Update();
		Delay_ms(1000);
	}
	
	if (ESP_WaitResp("OK",6000))
	{
		Serial_SendString(1,"AT测试正常\r\n");
		OLED_Clear();
		OLED_ShowString(0,0,"OK",OLED_8X16);
		OLED_Update();
	}
	else
	{
		Serial_SendString(1,"AT测试失败\r\n");
		OLED_ShowString(0,0,"ERROR",OLED_8X16);
		OLED_Clear();
		OLED_Update();
		Delay_ms(500);
		return;
	}
	
	/*2:模块复位*/
	USART2_RX_LEN=0;
	memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
	Serial_SendString(2,"AT+RST\r\n");
	Serial_SendString(1,"模块复位\r\n");
	OLED_Clear();
	OLED_ShowString(0,0,"AT+RST",OLED_8X16);
	OLED_Update();
	Delay_ms(1000);
	
	/*3:开启STA模式*/
	USART2_RX_LEN=0;
	memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
	Serial_SendString(2,"AT+CWMODE=1\r\n");
	Serial_SendString(1,"开启STA模式\r\n");
	OLED_Clear();
	OLED_ShowString(0,0,"AT+CWMODE=1",OLED_8X16);
	OLED_Update();
	Delay_ms(300);
	
	/*4:连接网络*/
	USART2_RX_LEN=0;
	memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
	Serial_SendString(2,"AT+CWJAP=\""WIFI_NAME"\",\""WIFI_PASSWORD"\"\r\n");
	Serial_SendString(1,"连接网络\r\n");
	OLED_Clear();
	OLED_ShowString(0,0,"AT+CWJAP",OLED_8X16);
	OLED_Update();
	
	uint8_t conn_flag=0;
	uint8_t ip_flag=0;
	uint32_t start_time=0;
	
	while (start_time<10000)//10秒
	{
		if (strstr(USART2_RX_BUF,"WIFI CONNECTED")!=NULL) conn_flag=1;
		if (strstr(USART2_RX_BUF,"WIFI GOT IP")!=NULL) ip_flag=1;
		if (conn_flag && ip_flag) break;//提前退出轮询
		Delay_ms(10);
		start_time+=10;
	}
	//Serial_SendString(1,USART2_RX_BUF);
	if (conn_flag && ip_flag)
	{
		Serial_SendString(1,"网络连接成功\r\n");
		OLED_Clear();
		OLED_ShowString(0,0,"OK",OLED_8X16);
		OLED_Update();
	}
	else
	{
		Serial_SendString(1,"网络连接失败\r\n");
		OLED_Clear();
		OLED_ShowString(0,0,"ERROR",OLED_8X16);
		OLED_Update();
		Delay_ms(500);
		return;
	}
	
	ESP8266_Init_Success=1;
	
	Delay_ms(2000);
}

/*MQTT初始化函数
*/
void MQTT_Init()
{
	/*1:设置用户属性*/
	USART2_RX_LEN=0;
	memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
	Serial_SendString(2,
	"AT+MQTTUSERCFG=0,1,\"mytext\",\"AceV67og0i\",\"version=2018-10-31&res=products%2FAceV67og0i%2Fdevices%2Fmytext&et=1805693871&method=md5&sign=PEUKCtejaA%2By49Qw78sOfw%3D%3D\",0,0,\"\"\r\n");
	Serial_SendString(1,"设置用户属性\r\n");
	OLED_Clear();
	OLED_ShowString(0,0,"AT+MQTTUSERCFG",OLED_8X16);
	OLED_Update();
	
	if (ESP_WaitResp("OK",6000))
	{
		Serial_SendString(1,"设置用户属性成功\r\n");
		OLED_Clear();
		OLED_ShowString(0,0,"OK",OLED_8X16);
		OLED_Update();
	}
	else
	{
		Serial_SendString(1,"设置用户属性失败\r\n");
		OLED_ShowString(0,0,"ERROR",OLED_8X16);
		OLED_Clear();
		OLED_Update();
		Delay_ms(500);
		return;
	}
	
	/*2:连接OneNET服务器*/
	Delay_ms(3000);
	USART2_RX_LEN=0;
	memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
	Serial_SendString(2,"AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1\r\n");
	Serial_SendString(1,"连接OneNET服务器\r\n");
	OLED_Clear();
	OLED_ShowString(0,0,"AT+MQTTCONN",OLED_8X16);
	OLED_Update();
	
	if (ESP_WaitResp("OK",8000))
	{
		Serial_SendString(1,"OneNET服务器连接成功\r\n");
		OLED_Clear();
		OLED_ShowString(0,0,"OK",OLED_8X16);
		OLED_Update();
	}
	else
	{
		Serial_SendString(1,"OneNET服务器连接失败\r\n");
		OLED_ShowString(0,0,"ERROR",OLED_8X16);
		OLED_Clear();
		OLED_Update();
		Delay_ms(500);
		return;
	}
	
	/*3:订阅主题*/
	Delay_ms(1000);
	USART2_RX_LEN=0;
	memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
	Serial_SendString(2,"AT+MQTTSUB=0,\"$sys/AceV67og0i/mytext/thing/property/post/reply\",0\r\n");
	Serial_SendString(1,"订阅“设备属性上报响应”主题\r\n");
	OLED_Clear();
	OLED_ShowString(0,0,"AT+MQTTSUB:reply",OLED_8X16);
	OLED_Update();
	
	if (ESP_WaitResp("OK",6000))
	{
		Serial_SendString(1,"订阅成功\r\n");
		OLED_Clear();
		OLED_ShowString(0,0,"OK",OLED_8X16);
		OLED_Update();
	}
	else
	{
		Serial_SendString(1,"订阅失败\r\n");
		OLED_ShowString(0,0,"ERROR",OLED_8X16);
		OLED_Clear();
		OLED_Update();
		Delay_ms(500);
		return;
	}
	
	Delay_ms(1000);
	USART2_RX_LEN=0;
	memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
	Serial_SendString(2,"AT+MQTTSUB=0,\"$sys/AceV67og0i/mytext/thing/property/set\",0\r\n");
	Serial_SendString(1,"订阅“设备属性设置请求”主题\r\n");
	OLED_Clear();
	OLED_ShowString(0,0,"AT+MQTTSUB:set",OLED_8X16);
	OLED_Update();
	
	if (ESP_WaitResp("OK",6000))
	{
		Serial_SendString(1,"订阅成功\r\n");
		OLED_Clear();
		OLED_ShowString(0,0,"OK",OLED_8X16);
		OLED_Update();
	}
	else
	{
		Serial_SendString(1,"订阅失败\r\n");
		OLED_ShowString(0,0,"ERROR",OLED_8X16);
		OLED_Clear();
		OLED_Update();
		Delay_ms(500);
		return;
	}
	
	MQTT_Init_Success=1;
	Delay_ms(2000);
}

/*上传数据函数
*/
void MQTT_Publish_Data(float temperature,float humidity)
{
	char data[128];
	snprintf(data,sizeof(data),"{\"id\":\"123456\",\"params\":{\"temperature\":{\"value\":%.2f},\"humidity\":{\"value\":%.2f}}}",temperature,humidity);
	
	uint16_t data_len=strlen(data);//计算长度，不需要+1
	
	char at_cmd[128];
	snprintf(at_cmd,sizeof(at_cmd),"AT+MQTTPUBRAW=0,\"$sys/AceV67og0i/mytext/thing/property/post\",%d,0,0\r\n",data_len);
	USART2_RX_LEN=0;
	memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
	Serial_SendString(2,at_cmd);
	Serial_SendString(1,"准备发送数据\r\n");
	OLED_Clear();
	OLED_ShowString(0,0,"AT+MQTTPUBRAW",OLED_8X16);
	OLED_Update();
	
	if (ESP_WaitResp(">",6000))
	{
		Serial_SendString(1,"准备发送数据成功\r\n");
		OLED_Clear();
		OLED_ShowString(0,0,"OK",OLED_8X16);
		OLED_Update();
	}
	else
	{
		Serial_SendString(1,"准备发送数据失败\r\n");
		OLED_ShowString(0,0,"ERROR",OLED_8X16);
		OLED_Clear();
		OLED_Update();
		Delay_ms(500);
		return;
	}
	
	USART2_RX_LEN=0;
	memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
	Serial_SendString(2,data);
	Serial_SendString(1,"正在发送数据\r\n");
	OLED_Clear();
	OLED_ShowString(0,0,"send...",OLED_8X16);
	OLED_Update();
	
	if (ESP_WaitResp("OK",6000))
	{
		Serial_SendString(1,"发送成功\r\n");
		OLED_Clear();
		OLED_ShowString(0,0,"OK",OLED_8X16);
		OLED_Update();
	}
	else
	{
		Serial_SendString(1,"发送失败\r\n");
		OLED_ShowString(0,0,"ERROR",OLED_8X16);
		OLED_Clear();
		OLED_Update();
		Delay_ms(500);
		return;
	}
	
	USART2_RX_LEN=0;
	memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
}

/*接收数据函数
*/
uint8_t MQTT_Get_Data(char *name,char *data1,char *data2)
{
	if (strstr(USART2_RX_BUF,name)!=NULL)//接收到带有标识符的数据
	{
		uint8_t data=0;
		Serial_SendString(1,"成功接收到数据\r\n");
		Serial_SendString(1,USART2_RX_BUF);
		if (strstr(USART2_RX_BUF,data1)!=NULL)//提取到指令1
		{
			data=1;
		}
		else if (strstr(USART2_RX_BUF,data2)!=NULL)//提取到指令2
		{
			data=2;
		}
		else
		{
			data=0;
		}
		if (data!=0)//提取到指令后，清空数组，防止对下次判断产生影响
		{
			USART2_RX_LEN=0;
			memset(USART2_RX_BUF,0,sizeof(USART2_RX_BUF));
		}
		
		return data;
	}
	else
	{
		return 0;
	}
}

/*轮询检测函数
*/
uint8_t ESP_WaitResp(char *wait_str,uint16_t timeout_ms)
{
	uint32_t start=0;
	
	while (start<timeout_ms)
	{
		if (strstr(USART2_RX_BUF,wait_str)!=NULL)
		{
			return 1;//查找成功
		}
		Delay_ms(1);
		start++;
	}
	return 0;//超时
}