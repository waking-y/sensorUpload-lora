#include "stm32f10x.h"
#include "delay.h"
#include "usart.h"
#include "atk_mw1278d.h"
#include "dht11.h"
#include "gps.h"       
#include <string.h>
#include <stdio.h>

void Hardware_Init(void)
{
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	Delay_Init();
	
	Usart1_Init(115200); // Debug
    
    GPS_Init(); 
	
	UsartPrintf(USART1, " Hardware init OK\r\n"); // 修改为具体的串口，之前代码用的是USART_DEBUG宏，确保你的宏是对的
	
	while(DHT11_Init())
 	{
		UsartPrintf(USART1, "DHT11 Error \r\n");
 		DelayMs(1000);
 	}
}

u8 temp, humi;
// 增加缓冲区大小，因为带有经纬度的字符串比较长
char my_data[128]; 

int main(void)
{
	Hardware_Init();

	while(atk_mw1278d_init(115200))
	{
		UsartPrintf(USART1, "LoRa Init Error, Retrying...\r\n");
		DelayMs(1000); // 你的原代码是 DelayXms，确保函数名正确
	}
	UsartPrintf(USART1, "LoRa Init OK\r\n");

	atk_mw1278d_enter_config();
	atk_mw1278d_uart_config(ATK_MW1278D_UARTRATE_115200BPS, ATK_MW1278D_UARTPARI_NONE);
	atk_mw1278d_addr_config(0);
	atk_mw1278d_wlrate_channel_config(ATK_MW1278D_WLRATE_19K2, 0);
	atk_mw1278d_tmode_config(ATK_MW1278D_TMODE_TT);
	atk_mw1278d_exit_config();
	
	DelayMs(500);
	
	while(1)
	{
    GPS_Parse_Data();
		// 2. 读取温湿度并发送
		if(DHT11_Read_Data(&temp, &humi) == 0)
		{
			if(atk_mw1278d_free() == ATK_MW1278D_EOK)
			{
                // 如果定位成功 (fix_status > 0)，发送经纬度；否则提示未定位
                if (GPS_Data.fix_status > 0) 
                {
                    // 格式：T:温度 H:湿度 Lat:纬度 Lon:经度 Sats:卫星数
                    // %f 在单片机中比较占空间，这里保留6位小数，满足地图经纬度要求
				    sprintf(my_data, "T:%d H:%d Lat:%.6f Lon:%.6f S:%d\r\n", 
                            temp, humi, GPS_Data.latitude, GPS_Data.longitude, GPS_Data.sats_used);
                }
                else
                {
                    sprintf(my_data, "T:%d H:%d GPS:Searching...\r\n", temp, humi);
                }

				atk_mw1278d_uart_printf(my_data);
				UsartPrintf(USART1, "LoRa Sent: %s", my_data);
			}
		}
		else
		{
			UsartPrintf(USART1, "DHT11 Read Error\r\n");
		}
		
		DelayMs(2000);	 // 原代码 DelayXms，请根据你的延时函数库确认名字
	}
}
