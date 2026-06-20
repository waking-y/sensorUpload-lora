#include "gps.h"
#include "usart.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define GPS_BUFFER_SIZE 256

char GPS_RX_Buffer[GPS_BUFFER_SIZE];      // 串口接收缓存
char GPS_Parse_Buffer[GPS_BUFFER_SIZE];   // 数据解析缓存 (双缓冲，防止主循环解析时被中断覆盖)
uint16_t GPS_RX_Index = 0;
uint8_t GPS_Frame_Ready = 0;              // 接收完成标志

GPS_Info_t GPS_Data = {0.0, 0.0, 'N', 'E', 0, 0};

// GPS初始化
void GPS_Init(void)
{
    // GPS模块通常波特率为9600。如果你的模块是115200，请改为115200
    Usart2_Init(9600); 
    
    // 初始化清空结构体
    memset(&GPS_Data, 0, sizeof(GPS_Info_t));
}

// 辅助函数：将 ddmm.mmmmm (度分格式) 转换为 dd.dddddd (十进制度格式)
// 例如: 3258.01973 -> 32 + 58.01973/60 = 32.9669955
double NMEA_To_Decimal(char *nmea_str)
{
    char degree_str[4] = {0};
    char *dot_ptr = strchr(nmea_str, '.');
    
    if (dot_ptr == NULL) return 0.0;
    
    // 计算"度"所占的字符长度 (小数点位置减去2就是度的长度)
    // 纬度是 2位度，经度是 3位度
    int degree_len = (dot_ptr - nmea_str) - 2; 
    
    strncpy(degree_str, nmea_str, degree_len);
    int degrees = atoi(degree_str);          // 提取度
    double minutes = atof(nmea_str + degree_len); // 提取分
    
    return (double)degrees + (minutes / 60.0);
}

// 解析 GPS 数据 (主要解析 $GNGGA)
void GPS_Parse_Data(void)
{
    if (GPS_Frame_Ready)
    {
        // 寻找 $GNGGA 或 $GPGGA 报文
        // 格式: $GNGGA,hhmmss.ss,纬度,南北,经度,东西,定位状态,卫星数,...
        if (strncmp(GPS_Parse_Buffer, "$GNGGA", 6) == 0 || strncmp(GPS_Parse_Buffer, "$GPGGA", 6) == 0)
        {
            char *token;
            int comma_count = 0;
            
            // 复制一份，因为 strtok 会破坏原字符串
            char temp_buf[GPS_BUFFER_SIZE];
            strcpy(temp_buf, GPS_Parse_Buffer);
            
            token = strtok(temp_buf, ",");
            while (token != NULL)
            {
                comma_count++;
                token = strtok(NULL, ",");
                
                if (token == NULL) break;
                
                switch (comma_count)
                {
                    case 2: // 纬度 (Latitude)
                        if (strlen(token) > 5) GPS_Data.latitude = NMEA_To_Decimal(token);
                        break;
                    case 3: // 南北纬 (N/S)
                        GPS_Data.lat_dir = token[0];
                        break;
                    case 4: // 经度 (Longitude)
                        if (strlen(token) > 5) GPS_Data.longitude = NMEA_To_Decimal(token);
                        break;
                    case 5: // 东西经 (E/W)
                        GPS_Data.lon_dir = token[0];
                        break;
                    case 6: // 定位状态 (Fix Quality)
                        GPS_Data.fix_status = atoi(token);
                        break;
                    case 7: // 使用的卫星数量
                        GPS_Data.sats_used = atoi(token);
                        break;
                }
            }
        }
        GPS_Frame_Ready = 0; // 处理完毕，清除标志
    }
}

// 串口2中断服务函数 (接管接收)
void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        char res = USART_ReceiveData(USART2);
        
        if (GPS_RX_Index < GPS_BUFFER_SIZE - 1)
        {
            GPS_RX_Buffer[GPS_RX_Index++] = res;
            
            // 收到换行符表示一帧 NMEA 数据结束
            if (res == '\n')
            {
                GPS_RX_Buffer[GPS_RX_Index] = '\0';
                // 将数据拷贝到解析缓冲区，防止被下一次中断覆盖
                strcpy(GPS_Parse_Buffer, GPS_RX_Buffer); 
                GPS_Frame_Ready = 1;
                GPS_RX_Index = 0; // 重置索引，准备接收下一句
            }
        }
        else
        {
            GPS_RX_Index = 0; // 溢出保护
        }
    }
}
