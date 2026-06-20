//��Ƭ��ͷ�ļ�
#include "stm32f10x.h"

//delayͷ�ļ�
#include "delay.h"


//��ʱϵ��
unsigned char UsCount = 0;
unsigned short MsCount = 0;


/*
************************************************************
*	�������ƣ�	Delay_Init
*
*	�������ܣ�	systick��ʼ��
*
*	��ڲ�����	��
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void Delay_Init(void)
{

	SysTick->CTRL &= ~(1 << 2);		//选择时钟为HCLK(72MHz)/8 = 9MHz (STM32F103C8主频72MHz)

	UsCount = 9;					//微秒级延时系数
	
	MsCount = UsCount * 1000;		//���뼶��ʱϵ��

}

/*
************************************************************
*	�������ƣ�	DelayUs
*
*	�������ܣ�	΢�뼶��ʱ
*
*	��ڲ�����	us����ʱ��ʱ��
*
*	���ز�����	��
*
*	说明：		定时器(9MHz)最长定时798915us
************************************************************
*/
void DelayUs(unsigned short us)
{

	unsigned int ctrlResult = 0;
	
	us &= 0x00FFFFFF;											//ȡ��24λ
	
	SysTick->LOAD = us * UsCount;								//װ������
	SysTick->VAL = 0;
	SysTick->CTRL = 1;											//ʹ�ܵ�������
	
	do
	{
		ctrlResult = SysTick->CTRL;
	}
	while((ctrlResult & 0x01) && !(ctrlResult & (1 << 16)));	//��֤�����С�����Ƿ񵹼�����0
	
	SysTick->CTRL = 0;											//�رյ�������
	SysTick->VAL = 0;

}

/*
************************************************************
*	�������ƣ�	DelayXms
*
*	�������ܣ�	���뼶��ʱ
*
*	��ڲ�����	ms����ʱ��ʱ��
*
*	���ز�����	��
*
*	˵����		
************************************************************
*/
void DelayXms(unsigned short ms)
{

	unsigned int ctrlResult = 0;
	
	if(ms == 0)
		return;
	
	ms &= 0x00FFFFFF;											//ȡ��24λ
	
	SysTick->LOAD = ms * MsCount;								//װ������
	SysTick->VAL = 0;
	SysTick->CTRL = 1;											//ʹ�ܵ�������
	
	do
	{
		ctrlResult = SysTick->CTRL;
	}
	while((ctrlResult & 0x01) && !(ctrlResult & (1 << 16)));	//��֤�����С�����Ƿ񵹼�����0
	
	SysTick->CTRL = 0;											//�رյ�������
	SysTick->VAL = 0;

}

/*
************************************************************
*	�������ƣ�	DelayMs
*
*	�������ܣ�	΢�뼶����ʱ
*
*	��ڲ�����	ms����ʱ��ʱ��
*
*	���ز�����	��
*
*	˵����		��ε���DelayXms����������ʱ
************************************************************
*/
void DelayMs(unsigned short ms)
{

	unsigned char repeat = 0;
	unsigned short remain = 0;
	
	repeat = ms / 500;
	remain = ms % 500;
	
	while(repeat)
	{
		DelayXms(500);
		repeat--;
	}
	
	if(remain)
		DelayXms(remain);

}
