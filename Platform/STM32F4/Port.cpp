﻿#include "Kernel\Sys.h"
#include "Kernel\Interrupt.h"
#include "Device\Port.h"

#include "Platform\stm32.h"

static const byte PORT_IRQns[] = {
    EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn, // 5个基础的
    EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,    // EXTI9_5
    EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn   // EXTI15_10
};

/******************************** Port ********************************/

// 端口基本功能
#define REGION_Port 1
#ifdef REGION_Port

extern GPIO_TypeDef* IndexToGroup(byte index);

void OpenPeriphClock(Pin pin, bool flag)
{
    int gi = pin >> 4;
	FunctionalState fs = flag ? ENABLE : DISABLE;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA << gi, fs);
}

void Port::AFConfig(GPIO_AF GPIO_AF) const
{
	assert(Opened, "打开后才能配置AF");

	GPIO_PinAFConfig(IndexToGroup(_Pin >> 4), _Pin & 0x000F, GPIO_AF);
}
#endif

/******************************** OutputPort ********************************/

// 输出端口
#define REGION_Output 1
#ifdef REGION_Output

void OutputPort::OpenPin()
{
	assert(Speed == 2 || Speed == 25 || Speed == 50 || Speed == 100, "Speed");

	auto gpio	= (GPIO_InitTypeDef*)&State;

	switch(Speed)
	{
		case 2:		gpio->GPIO_Speed = GPIO_Speed_2MHz;	break;
		case 25:	gpio->GPIO_Speed = GPIO_Speed_25MHz;	break;
		case 100:	gpio->GPIO_Speed = GPIO_Speed_100MHz;break;
		case 50:	gpio->GPIO_Speed = GPIO_Speed_50MHz;	break;
	}

	gpio->GPIO_Mode	= GPIO_Mode_OUT;
	gpio->GPIO_OType	= OpenDrain ? GPIO_OType_OD : GPIO_OType_PP;

    GPIO_Init(IndexToGroup(_Pin >> 4), gpio);
}

/******************************** AlternatePort ********************************/

void AlternatePort::OpenPin()
{
	auto gpio	= (GPIO_InitTypeDef*)&State;

	gpio->GPIO_Mode	= GPIO_Mode_AF;
	gpio->GPIO_OType	= OpenDrain ? GPIO_OType_OD : GPIO_OType_PP;

    GPIO_Init(IndexToGroup(_Pin >> 4), gpio);
}

#endif

/******************************** InputPort ********************************/

// 输入端口
#define REGION_Input 1
#ifdef REGION_Input

extern int Bits2Index(ushort value);
extern bool InputPort_HasEXTI(int line, const InputPort& port);
extern void GPIO_ISR(int num);
extern void SetEXIT(int pinIndex, bool enable, InputPort::Trigger mode);

void InputPort::OpenPin()
{
	auto gpio	= (GPIO_InitTypeDef*)&State;

	gpio->GPIO_Mode = GPIO_Mode_IN;
	//gpio->GPIO_OType = !Floating ? GPIO_OType_OD : GPIO_OType_PP;

    GPIO_Init(IndexToGroup(_Pin >> 4), gpio);
}

// 是否独享中断号
static bool IsOnlyExOfInt(const InputPort& port, int idx)
{
	int s=0, e=0;
	if(idx <= 4) return true;

	if(idx <= 9)
	{
		s	= 5;
		e	= 9;
	}
	else if(idx <= 15)
	{
		s	= 10;
		e	= 15;
	}
	for(int i = s; i <= e; i++)
		if(InputPort_HasEXTI(i, port)) return false;

	return true;
}

void InputPort_CloseEXTI(const InputPort& port)
{
	Pin pin	= port._Pin;
	int idx = Bits2Index(1 << (pin & 0x0F));

	SetEXIT(idx, false, InputPort::Both);
	if(!IsOnlyExOfInt(port, idx))return;
	Interrupt.Deactivate(PORT_IRQns[idx]);
}

void EXTI_IRQHandler(ushort num, void* param)
{
	// EXTI0 - EXTI4
	if(num <= EXTI4_IRQn)
		GPIO_ISR(num - EXTI0_IRQn);
	else
	{
		uint pending = EXTI->PR & EXTI->IMR;
		for(int i=0; i < 16 && pending != 0; i++, pending >>= 1)
		{
			if (pending & 1) GPIO_ISR(i);
		}
	}
}

void InputPort_OpenEXTI(InputPort& port)
{
	Pin pin	= port._Pin;
	byte gi = pin >> 4;
	int idx = Bits2Index(1 << (pin & 0x0F));

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    SYSCFG_EXTILineConfig(gi, idx);

	SetEXIT(idx, true, InputPort::Both);

    // 打开并设置EXTI中断为低优先级
    Interrupt.SetPriority(PORT_IRQns[idx], 1);

    Interrupt.Activate(PORT_IRQns[idx], EXTI_IRQHandler, &port);
}

#endif

/******************************** AnalogInPort ********************************/

void AnalogInPort::OpenPin()
{
	auto gpio	= (GPIO_InitTypeDef*)&State;

	gpio->GPIO_Mode	= GPIO_Mode_AN;
	//gpio->GPIO_OType = !Floating ? GPIO_OType_OD : GPIO_OType_PP;

    GPIO_Init(IndexToGroup(_Pin >> 4), gpio);
}