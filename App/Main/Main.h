/*********************************************************************************************************
* 模块名称：Main.h
* 摘    要：主文件，包含软硬件初始化函数和main函数
* 当前版本：1.0.0
* 作    者：SZLY(COPYRIGHT 2018 - 2020 SZLY. All rights reserved.)
* 完成日期：2020年01月01日 
* 内    容：
* 注    意：                                                                  
**********************************************************************************************************
* 取代版本：
* 作    者：
* 完成日期：
* 修改内容：
* 修改文件：
*********************************************************************************************************/
#ifndef _MAIN_H_
#define _MAIN_H_

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "stm32f10x_conf.h"
#include "DataType.h"
#include "NVIC.h"
#include "SysTick.h"
#include "RCC.h"
#include "Timer.h"
#include "UART1.h"
#include "UART2.h"
#include "LED.h"
#include "RADIO.h"
#include "Wave.h"
#include "ProcHostCmd.h"
#include "PackUnpack.h"
#include "DAC.h"
#include "SendDataToHost.h"
#include "ADC.h"
#include "Route.h"


/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define SINK TRUE  //汇聚节点设置为TRUE ,否则为FALSE
  
/*********************************************************************************************************
*                                              枚举结构体定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              API函数定义
*********************************************************************************************************/

#endif
