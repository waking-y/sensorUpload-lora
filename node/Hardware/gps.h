#ifndef __GPS_H
#define __GPS_H

#include "stm32f10x.h"

// GPS数据结构体
typedef struct {
    double latitude;    // 纬度 (十进制: dd.dddddd)
    double longitude;   // 经度 (十进制: ddd.dddddd)
    char lat_dir;       // N (北纬) 或 S (南纬)
    char lon_dir;       // E (东经) 或 W (西经)
    uint8_t fix_status; // 定位状态：0未定位，1已定位
    uint8_t sats_used;  // 使用的卫星数量
} GPS_Info_t;

extern GPS_Info_t GPS_Data;

void GPS_Init(void);
void GPS_Parse_Data(void);

#endif
