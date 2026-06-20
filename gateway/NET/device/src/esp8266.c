//��Ƭ��ͷ�ļ�
#include "stm32f10x.h"

//�����豸����
#include "esp8266.h"

//Ӳ������
#include "delay.h"
#include "usart.h"

//C��
#include <string.h>
#include <stdio.h>

unsigned char esp8266_buf[1024];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;

//==========================================================
//	�������ƣ�	ESP8266_Clear
//
//	�������ܣ�	��ջ���
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;

}

//==========================================================
//	�������ƣ�	ESP8266_WaitRecive
//
//	�������ܣ�	�ȴ��������
//
//	��ڲ�����	��
//
//	���ز�����	REV_OK-�������		REV_WAIT-���ճ�ʱδ���
//
//	˵����		ѭ�����ü���Ƿ�������
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0) 							//������ռ���Ϊ0 ��˵��û�д��ڽ��������У�����ֱ����������������
		return REV_WAIT;
		
	if(esp8266_cnt == esp8266_cntPre)				//�����һ�ε�ֵ�������ͬ����˵���������
	{
		esp8266_cnt = 0;							//��0���ռ���
			
		return REV_OK;								//���ؽ�����ɱ�־
	}
		
	esp8266_cntPre = esp8266_cnt;					//��Ϊ��ͬ
	
	return REV_WAIT;								//���ؽ���δ��ɱ�־

}

//==========================================================
//	�������ƣ�	ESP8266_SendCmd
//
//	�������ܣ�	��������
//
//	��ڲ�����	cmd������
//				res����Ҫ���ķ���ָ��
//
//	���ز�����	0-�ɹ�	1-ʧ��
//
//	˵����		
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	
	unsigned char timeOut = 250;

	ESP8266_Clear();                                             /* 发送前清空缓冲区，防止旧数据干扰 */
	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//����յ�����
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//����������ؼ���
			{
				ESP8266_Clear();									//��ջ���
				
				return 0;
			}
		}
		
		DelayXms(10);
	}
	
	return 1;

}

//==========================================================
//	�������ƣ�	ESP8266_SendData
//
//	�������ܣ�	��������
//
//	��ڲ�����	data������
//				len������
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
	ESP8266_Clear();								//��ս��ջ���
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//��������
	if(!ESP8266_SendCmd(cmdBuf, ">"))				//�յ���>��ʱ���Է�������
	{
		Usart_SendString(USART2, data, len);		//�����豸������������
	}

}

//==========================================================
//	�������ƣ�	ESP8266_GetIPD
//
//	�������ܣ�	��ȡƽ̨���ص�����
//
//	��ڲ�����	�ȴ���ʱ��(����10ms)
//
//	���ز�����	ƽ̨���ص�ԭʼ����
//
//	˵����		��ͬ�����豸���صĸ�ʽ��ͬ����Ҫȥ����
//				��ESP8266�ķ��ظ�ʽΪ	"+IPD,x:yyy"	x�������ݳ��ȣ�yyy����������
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//����������
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//������IPD��ͷ
			if(ptrIPD == NULL)											//���û�ҵ���������IPDͷ���ӳ٣�������Ҫ�ȴ�һ�ᣬ�����ᳬ���趨��ʱ��
			{
				//UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//�ҵ�':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
				
			}
		}
		
		DelayXms(5);													//��ʱ�ȴ�
	} while(timeOut--);
	
	return NULL;														//��ʱ��δ�ҵ������ؿ�ָ��

}

//==========================================================
//	�������ƣ�	ESP8266_Init
//
//	�������ܣ�	��ʼ��ESP8266������WiFi
//
//	��ڲ�����	ssid��WiFi����
//              pwd�� WiFi����
//
//	���ز�����	��
//
//	˵����		
//==========================================================
_Bool ESP8266_Init(char *ssid, char *pwd)
{
	char wifi_cmd[64];
	unsigned char retry;

	ESP8266_Clear();

	/* 测试AT通信，重试10次 */
	UsartPrintf(USART_DEBUG, "1. AT Test...\r\n");
	retry = 10;
	while(retry--)
	{
		if(ESP8266_SendCmd("AT\r\n", "OK") == 0)
			break;
		DelayXms(500);
	}
	if(retry == 0)
	{
		UsartPrintf(USART_DEBUG, "ESP8266 No Response\r\n");
		return 1;
	}
	UsartPrintf(USART_DEBUG, "   AT OK\r\n");

	/* 设置Station模式 */
	UsartPrintf(USART_DEBUG, "2. CWMODE=1...\r\n");
	while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
		DelayXms(500);
	UsartPrintf(USART_DEBUG, "   CWMODE OK\r\n");

	/* 开启DHCP */
	UsartPrintf(USART_DEBUG, "3. CWDHCP...\r\n");
	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
		DelayXms(500);
	UsartPrintf(USART_DEBUG, "   CWDHCP OK\r\n");

	/* 连接WiFi，最多重试30次（约15秒） */
	UsartPrintf(USART_DEBUG, "4. CWJAP -> %s...\r\n", ssid);
	sprintf(wifi_cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);
	retry = 30;
	while(retry--)
	{
		if(ESP8266_SendCmd(wifi_cmd, "GOT IP") == 0)
		{
			UsartPrintf(USART_DEBUG, "   GOT IP OK\r\n");
			return 0;
		}

		/* 检查是否返回了FAIL或ERROR，若是则打印提示 */
		if(strstr((const char *)esp8266_buf, "FAIL") != NULL ||
		   strstr((const char *)esp8266_buf, "ERROR") != NULL)
		{
			UsartPrintf(USART_DEBUG, "   WiFi Err: %s\r\n", esp8266_buf);
		}
		DelayXms(500);
	}

	UsartPrintf(USART_DEBUG, "   WiFi Timeout!\r\n");
	return 1;
}

//==========================================================
//	�������ƣ�	USART2_IRQHandler
//
//	�������ܣ�	����2�շ��ж�
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void USART2_IRQHandler(void)
{

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) //�����ж�
	{
		if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0; //��ֹ���ڱ�ˢ��
		esp8266_buf[esp8266_cnt++] = USART2->DR;
		
		USART_ClearFlag(USART2, USART_FLAG_RXNE);
	}

}
