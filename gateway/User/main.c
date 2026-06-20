#include "stm32f10x.h"
#include "onenet.h"
#include "esp8266.h"
#include "delay.h"
#include "usart.h"
#include "atk_mw1278d.h"
#include <string.h>
#include <stdio.h>

#define WIFI_SSID           "MyWifi"
#define WIFI_PASSWORD       "12345678"

#define ONENET_MQTT_SRV     "AT+CIPSTART=\"TCP\",\"47.101.162.175\",1883\r\n"

#define UPLOAD_INTERVAL     500

uint8_t temp = 0;
uint8_t humi = 0;

static void Hardware_Init(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    Usart1_Init(115200);    /* 调试串口 */
    Usart2_Init(115200);    /* ESP8266串口 */

    UsartPrintf(USART_DEBUG, "Hardware init OK\r\n");
}

static void LoRa_Init(void)
{
    while(atk_mw1278d_init(115200))
    {
        UsartPrintf(USART_DEBUG, "LoRa Init Error, Retrying...\r\n");
        DelayXms(1000);
    }
    UsartPrintf(USART_DEBUG, "LoRa Init OK\r\n");

    /* 配置LoRa模块参数：115200-8N1、透明传输、地址0、信道0 */
    atk_mw1278d_enter_config();
    atk_mw1278d_uart_config(ATK_MW1278D_UARTRATE_115200BPS, ATK_MW1278D_UARTPARI_NONE);
    atk_mw1278d_addr_config(0);
    atk_mw1278d_wlrate_channel_config(ATK_MW1278D_WLRATE_19K2, 0);
    atk_mw1278d_tmode_config(ATK_MW1278D_TMODE_TT);
    atk_mw1278d_exit_config();
}

static _Bool WIFI_Init(void)
{
    UsartPrintf(USART_DEBUG, "--- ESP8266 Init ---\r\n");

    UsartPrintf(USART_DEBUG, "Connecting WiFi...\r\n");
    if(ESP8266_Init(WIFI_SSID, WIFI_PASSWORD) != 0)
    {
        UsartPrintf(USART_DEBUG, "WiFi Init Failed!\r\n");
        return 1;
    }
    UsartPrintf(USART_DEBUG, "WiFi Connected\r\n");

    UsartPrintf(USART_DEBUG, "Connect MQTT Server...\r\n");
    while(ESP8266_SendCmd(ONENET_MQTT_SRV, "CONNECT"))
        DelayXms(500);
    UsartPrintf(USART_DEBUG, "MQTT Server Connected\r\n");

    while(OneNet_DevLink())
    {
        ESP8266_SendCmd(ONENET_MQTT_SRV, "CONNECT");
        DelayXms(500);
    }

    OneNET_Subscribe();

    UsartPrintf(USART_DEBUG, "--- OneNET Ready ---\r\n");
    return 0;
}

static void LoRa_ProcessReceived(void)
{
    uint16_t rx_len = atk_mw1278d_uart_rx_get_frame_len();
    uint8_t *lora_data = atk_mw1278d_uart_rx_get_frame();

    if(lora_data != NULL && rx_len > 0)
    {
        int parse_temp, parse_humi;

        if(sscanf((const char *)lora_data, "T:%d H:%d", &parse_temp, &parse_humi) == 2)
        {
            temp = (uint8_t)parse_temp;
            humi = (uint8_t)parse_humi;
            UsartPrintf(USART_DEBUG, "Recv: T=%d C, H=%d %%\r\n", temp, humi);
        }
        else
        {
            UsartPrintf(USART_DEBUG, "Parse Error: %s\r\n", lora_data);
        }

        atk_mw1278d_uart_rx_restart();
    }
}

static void UploadToOneNET(void)
{
    char data_buf[64];

    sprintf(data_buf, "{\"temp\":%d, \"humi\":%d}", temp, humi);
    OneNET_Publish("hardware/dht11", data_buf);
    UsartPrintf(USART_DEBUG, "Upload: %s\r\n", data_buf);
}

int main(void)
{
    unsigned short timeCount = 0;

    Hardware_Init();

    LoRa_Init();

    WIFI_Init();

    atk_mw1278d_uart_rx_restart();

    while(1)
    {
        LoRa_ProcessReceived();

        if(++timeCount >= UPLOAD_INTERVAL)
        {
            UploadToOneNET();
            timeCount = 0;
            ESP8266_Clear();
        }

        DelayXms(10);
    }
}
