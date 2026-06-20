/**
 ****************************************************************************************************
 * @file        atk_mw1278d_uart.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-01-28
 * @brief       ATK-MW1278D模块UART接口驱动（已适配标准外设库）
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#ifndef __ATK_MW1278D_UART_H
#define __ATK_MW1278D_UART_H

#include "stm32f10x.h"

/* UART接收缓冲区大小 */
#define ATK_MW1278D_UART_RX_BUF_SIZE            128
#define ATK_MW1278D_UART_TX_BUF_SIZE            128

/* 函数声明 */
void atk_mw1278d_uart_printf(char *fmt, ...);       /* ATK-MW1278D UART printf */
void atk_mw1278d_uart_rx_restart(void);             /* ATK-MW1278D UART重新开始接收数据 */
uint8_t *atk_mw1278d_uart_rx_get_frame(void);       /* 获取ATK-MW1278D UART接收到的完整一帧数据 */
uint16_t atk_mw1278d_uart_rx_get_frame_len(void);   /* 获取ATK-MW1278D UART接收到的完整一帧数据的长度 */
void atk_mw1278d_uart_init(uint32_t baudrate);      /* ATK-MW1278D UART初始化 */

#endif
