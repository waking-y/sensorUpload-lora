#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "atk_mw1278d.h"
#include "dht11.h"
#include <string.h>
#include <stdio.h>

void Hardware_Init(void)
{
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	Delay_Init();
	
	Usart1_Init(115200);
	
	UsartPrintf(USART_DEBUG, " Hardware init OK\r\n");
	
	while(DHT11_Init())
 	{
		UsartPrintf(USART_DEBUG, "DHT11 Error \r\n");
 		DelayMs(1000);
 	}
}

u8 temp,humi,i;
char my_data[64];
int main(void)
{

	Hardware_Init();

	while(atk_mw1278d_init(115200))
	{
		UsartPrintf(USART_DEBUG, "LoRa Init Error, Retrying...\r\n");
		DelayXms(1000);
	}
	UsartPrintf(USART_DEBUG, "LoRa Init OK\r\n");

	atk_mw1278d_enter_config();
	atk_mw1278d_uart_config(ATK_MW1278D_UARTRATE_115200BPS, ATK_MW1278D_UARTPARI_NONE);
	atk_mw1278d_addr_config(0);
	atk_mw1278d_wlrate_channel_config(ATK_MW1278D_WLRATE_19K2, 0);
	atk_mw1278d_tmode_config(ATK_MW1278D_TMODE_TT);
	atk_mw1278d_exit_config();
	
	DelayXms(500);
	
	while(1)
	{
		if(DHT11_Read_Data(&temp, &humi) == 0)
		{
			if(atk_mw1278d_free() == ATK_MW1278D_EOK)
			{
				sprintf(my_data, "T:%d H:%d\r\n", temp, humi);
				atk_mw1278d_uart_printf(my_data);
				UsartPrintf(USART_DEBUG, "LoRa Sent: %s", my_data);
			}
		}
		else
		{
			UsartPrintf(USART_DEBUG, "DHT11 Read Error\r\n");
		}
		
		DelayXms(2000);	
	}
}
