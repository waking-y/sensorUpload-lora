#include "atk_mw1278d_uart.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* USART3接收帧缓冲区结构体 */
static struct
{
    uint8_t buf[ATK_MW1278D_UART_RX_BUF_SIZE];          /* 帧接收缓冲区 */
    volatile uint16_t len;                              /* 已接收数据的长度 */
} g_uart_rx = {0};                                      /* ATK-MW1278D UART接收帧缓冲区 */

static volatile uint16_t g_rx_last_tick = 0;            /* 最后一次收到数据时的TIM2计数值，用于超时判断 */
static uint8_t g_uart_tx_buf[ATK_MW1278D_UART_TX_BUF_SIZE]; /* ATK-MW1278D UART发送缓冲区 */

/**
 * @brief       ATK-MW1278D UART printf
 * @param       fmt: 要打印的字符串
 * @retval      无
 */
void atk_mw1278d_uart_printf(char *fmt, ...)
{
    va_list ap;
    uint16_t len;
    uint16_t i;

    va_start(ap, fmt);
    vsnprintf((char *)g_uart_tx_buf, sizeof(g_uart_tx_buf), fmt, ap);
    va_end(ap);

    len = strlen((const char *)g_uart_tx_buf);
    for (i = 0; i < len; i++)
    {
        USART_SendData(USART3, g_uart_tx_buf[i]);
        while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
    }
}

/**
 * @brief       ATK-MW1278D UART重新开始接收数据
 * @param       无
 * @retval      无
 */
void atk_mw1278d_uart_rx_restart(void)
{
    g_uart_rx.len = 0;
}

/**
 * @brief       获取ATK-MW1278D UART接收到的完整一帧数据
 * @param       无
 * @retval      NULL: 未接收到一帧数据
 *              非NULL: 已接收到一帧数据
 * @note        使用软件超时法判断帧结束：连续两次读取长度不变则认为接收完成
 */
uint8_t *atk_mw1278d_uart_rx_get_frame(void)
{
    uint16_t now;

    if (g_uart_rx.len == 0)
    {
        return NULL;
    }

    now = (uint16_t)TIM2->CNT;

    /* 超过20ms没有新数据到达，认为帧结束 */
    if ((uint16_t)(now - g_rx_last_tick) > 20)
    {
        g_uart_rx.buf[g_uart_rx.len] = '\0';
        return g_uart_rx.buf;
    }

    return NULL;
}

/**
 * @brief       获取ATK-MW1278D UART接收到的完整一帧数据的长度
 * @param       无
 * @retval      0   : 未接收到一帧数据
 *              非0 : 已接收到一帧数据的长度
 */
uint16_t atk_mw1278d_uart_rx_get_frame_len(void)
{
    uint16_t now;

    if (g_uart_rx.len == 0)
    {
        return 0;
    }

    now = (uint16_t)TIM2->CNT;

    if ((uint16_t)(now - g_rx_last_tick) > 20)
    {
        return g_uart_rx.len;
    }

    return 0;
}

/**
 * @brief       ATK-MW1278D UART初始化
 * @param       baudrate: UART通讯波特率
 * @retval      无
 */
void atk_mw1278d_uart_init(uint32_t baudrate)
{
    Usart3_Init(baudrate);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM2->PSC = 72000 - 1;
    TIM2->ARR = 0xFFFF;
    TIM2->CR1 = TIM_CR1_CEN;
}

/**
 * @brief       USART3中断服务函数
 * @param       无
 * @retval      无
 * @note        接收到的字节存入帧缓冲区，用于AT指令应答接收
 */
void USART3_IRQHandler(void)
{
    uint8_t tmp;

    if (USART_GetFlagStatus(USART3, USART_FLAG_ORE) != RESET)   /* UART溢出错误中断 */
    {
        USART_ClearFlag(USART3, USART_FLAG_ORE);
        tmp = USART3->DR;
    }

    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)      /* UART接收中断 */
    {
        tmp = USART_ReceiveData(USART3);

        if (g_uart_rx.len < (ATK_MW1278D_UART_RX_BUF_SIZE - 1)) /* 缓冲区还有空间 */
        {
            g_uart_rx.buf[g_uart_rx.len] = tmp;
            g_uart_rx.len++;
        }
        else                                                    /* 缓冲区满，从头覆盖 */
        {
            g_uart_rx.len = 0;
            g_uart_rx.buf[g_uart_rx.len] = tmp;
            g_uart_rx.len++;
        }

        g_rx_last_tick = (uint16_t)TIM2->CNT;
    }
}
