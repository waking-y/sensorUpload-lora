#ifndef _ESP8266_H_
#define _ESP8266_H_


#define REV_OK		0	//������ɱ�־
#define REV_WAIT	1	//����δ��ɱ�־


_Bool ESP8266_Init(char *ssid, char *pwd);

void ESP8266_Clear(void);

_Bool ESP8266_SendCmd(char *cmd, char *res);

void ESP8266_SendData(unsigned char *data, unsigned short len);

unsigned char *ESP8266_GetIPD(unsigned short timeOut);


#endif
