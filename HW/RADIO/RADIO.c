/*********************************************************************************************************
* 模块名称：RADIO.c
* 摘    要：无线模块，包括无线模块初始化，以及中断服务函数处理，以及读写串口函数实现
* 当前版本：1.0.0
* 作    者：LYL(COPYRIGHT 2021 - 2021 LYL. All rights reserved.)
* 完成日期：2022年2月22日 
* 内    容：接线如下:               
            STM32f103RCT6   |   E22-400T22S1C
                PB6     ------->    M0
                PB5     ------->    M1
                PB4     ------->    AUX
                TX1     ------->    TXD  (无误)
                RX1     ------->    RXD  (无误)
                GND     ------->    GND
* 注    意：LORA模块电压要求比单片机高，若单片机拉电线给LORA供电，有时射频电压不够大，发送失败。                
**********************************************************************************************************
* 取代版本：
* 作    者：
* 完成日期：
* 修改内容：
* 修改文件：
*********************************************************************************************************/

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "RADIO.h"
#include "Timer.h"
#include "stm32f10x_conf.h"
#include "SysTick.h"
#include "UART1.h"
#include <stdlib.h>

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
// registers  LORA模块寄存器地址
#define REG_ADDH        0x00
#define REG_ADDL        0x01
#define REG_NETID       0x02
#define REG_REG0        0x03
#define REG_REG1        0x04
#define REG_REG2        0x05
#define REG_REG3        0x06
#define REG_CRYPT_H     0x07
#define REG_CRYPT_L     0x08
#define PID1            0x80
#define PID2            0x81
#define PID3            0x82
#define PID4            0x83
#define PID5            0x84
#define PID6            0x85
#define PID7            0x86

#define LoRaBufMax      1000
/*********************************************************************************************************
*                                              枚举结构体定义
*********************************************************************************************************/
typedef enum
{
  AUX_STATE_BUSSY = 0x00,
  AUX_STATE_FREE  = 0x01,
}Enum_s_Aux_State;

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
static uint8 s_curMode;       //当前模式
volatile static uint8  s_Aux; //Aux位
const static uint8 s_arrSetting[9] = {0xC0, 0x00, 0x00, 0x62, 0x00, 0x17, 0x03, 0x00, 0x00 }; //存放已设置的参数
static uint16 s_address;      //Lora模块地址
static uint16 s_RadioBuf;     //Lora模块缓冲区当前大小

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static  void  DeInitLora(void);                                 //初始化模块参数---恢复出厂设置
static  void  ConfigLRGPIO(void);                               //配置LOAR串口模块与单片机相连的GPIO
static  uint8    ConfigLRMode(uint8 mode);                 //配置LORA模块的模式
static  uint8    readRegister(uint8 address);                         //读寄存器
static  uint8    readRegisters(uint8 addHead, uint8 len, uint8* pBufdata);  //读寄存器组
static  uint8    writeRegister(uint8 address, uint8 value);              //写寄存器
static  uint8    writeRegisters(uint8 addHead, uint8 value);             //写寄存器组
static  uint8    GetAuxState(void);                                //查询LORA模块状态,1--空闲, 0--繁忙

/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：ConfigLRGPIO
* 函数功能：配置LORA串口模块相关的GPIO,初始化连接模块M1,M0,Aux位的GPIO
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2021年10月14日
* 注    意：
*********************************************************************************************************/
static  void  ConfigLRGPIO(void)  
{
  GPIO_InitTypeDef GPIO_InitStructure;      //GPIO_InitStructure用于存放GPIO的参数
  EXTI_InitTypeDef EXTI_InitStructure;      //EXTI_InitStructure用于存放外部中断的参数
  NVIC_InitTypeDef NVIC_InitStructure;      //NVIC_InitStructure用于存放NVIC(设置中断优先级)的参数
  
  //使能RCC相关时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); //使能GPIOB的时钟，GPIOB输出电平使用
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);  //使能AFIO的时钟，输入捕获使用
  
  //配置Lora模块的M1位
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;           //设置引脚
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;     //设置I/O输出速度
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;     //设置模式
  GPIO_Init(GPIOB, &GPIO_InitStructure);                //根据参数初始化M1的GPIO

  GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET);          //将M1默认状态设置为0
  
  //配置Lora模块的M0位
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;           //设置引脚
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;     //设置I/O输出速度
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;     //设置模式
  GPIO_Init(GPIOB, &GPIO_InitStructure);                //根据参数初始化M0的GPIO

  GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_RESET);           //将M0默认状态设置为0
  
  //配置Lora模块的Aux位,当 AUX 输出低电平时，表示模块繁忙
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;            //设置Aux的引脚
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;         //设置Aux的模式,上拉，无输入时为1.表示模块空闲
  GPIO_Init(GPIOB, &GPIO_InitStructure);                 //根据参数初始化Aux的GPIO
   
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource4);//为PB4配置外部中断
  //配置外部中断，检测Aux位
  EXTI_InitStructure.EXTI_Line    = EXTI_Line4;           //选择EXTI4通道
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;               //使能中断
  EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;  //模式选择
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  //上升沿和下降沿中断
  EXTI_Init(&EXTI_InitStructure);                         //根据参数初始化EXTI
  
  //配置NVIC
  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;              //配置的中断通道号
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;     //设置抢占优先级
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;            //设置子优先级
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;               //使能中断
  NVIC_Init(&NVIC_InitStructure);                               //根据参数初始化NVIC 

}

/*********************************************************************************************************
* 函数名称：EXTI4_IRQHandler
* 函数功能：EXTI4中断服务函数 
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2022年02月10日
* 注    意：PB4(Aux位)上升沿中断处理，捕获LORA模块响应信号
*********************************************************************************************************/
void EXTI4_IRQHandler(void)  
{  
  if(EXTI_GetITStatus(EXTI_Line4) == SET)   //判断外部中断是否发生
  {
    EXTI_ClearITPendingBit(EXTI_Line4);     //清除外部中断标志
    
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == 1)
    {
      DelayNus(100);//消抖
      if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == 1)
      {
        s_Aux = AUX_STATE_FREE;                 //上升沿中断表示LORA模块空闲
      }
    }
    else
    {
      DelayNus(100);//消抖
      if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == 0)
      {
        s_Aux = AUX_STATE_BUSSY;                 //下降沿中断表示LORA模块空闲
      }
    }
    
  }
}

/*********************************************************************************************************
* 函数名称：ConfigLRMode
* 函数功能：模块模式设置
* 输入参数：mode,要设置的模式
* 输出参数：无
* 返 回 值：1---成功
* 创建日期：2021年10月14日
* 注    意：0-传输模式(支持特殊指令空中配置), 1-WOR模式(支持空中唤醒), 2-配置模式, 3-深度休眠
           当改变 M1、M0 后：若模块空闲，1ms 后，即可按照新的模式开始工作；
*********************************************************************************************************/
static  uint8  ConfigLRMode(uint8 mode)
{
  uint8  ok = 1;
  uint32 lastMillis;
  
//  lastMillis = millis();    //当前毫秒数
//  while((ok = !isLoRaReady()) && (millis() - lastMillis) <= 100);  //等待模块空闲或超时(ms)
//  DelayNms(5);
//  s_Aux = AUX_STATE_BUSSY;  //先置忙标志位
  s_curMode = mode;         //更新当前模式
  GPIO_WriteBit(GPIOB, GPIO_Pin_6, ((uint8)mode)&0x01);       //配置Lora模块的M0位
  GPIO_WriteBit(GPIOB, GPIO_Pin_5, (((uint8)mode)>>1)&0x01);  //配置Lora模块的M1位
  lastMillis = millis();    //当前毫秒数
  //若接线正确，模块先空闲(高电平)，再繁忙（低电平），再空闲(高电平)
  while((ok = !isLoRaReady()) && (millis() - lastMillis) <= 100);  //等待设置完成或超时(ms)

  return !ok;
}

/*********************************************************************************************************
* 函数名称：readRegister
* 函数功能：读寄存器
* 输入参数：address--待读寄存器地址
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月6日
* 注    意：
*********************************************************************************************************/
static uint8 readRegister(uint8 address)
{
  //               指令, 地址 , 长度
  uint8 pBufCMD[3] = {0xc1, 0x00, 0x01 };
  uint8 pBufRes[4];
  uint8 i,response;
  uint32 lastMillis;
  
  pBufCMD[1] = address;
  
  ConfigLRMode(MODEM_CONFIG); //设置配置模式
  while(!isLoRaReady());      //等待设置完成
  
  WriteUART1(pBufCMD, 3);     //发送指令
  
  i=0;
  lastMillis = millis();      //当前毫秒数
  while(i < 4 || millis() - lastMillis <= 100)//收到4字节或等待超过0.1S退出循环
  {
    i += ReadUART1(&pBufRes[i] , 4);
  }
  ConfigLRMode(MODEM_TRANSFER);   //设置退出配置模式，回到上次模式
  
  for(i=0; i<3; i++)
  {
    if(pBufRes[i] != pBufCMD[i])
    {
      return 0; //读取失败
    }
  }
  response = pBufRes[3];
  
  return response;
}

/*********************************************************************************************************
* 函数名称：readRegisters
* 函数功能：读寄存器组
* 输入参数：address--待读寄存器起始地址，len-要读寄存器个数，pBufdata-存放读出值地址
* 输出参数：无
* 返 回 值：1--成功，0--失败
* 创建日期：2022年2月2日
* 注    意：
*********************************************************************************************************/
static uint8 readRegisters(uint8 addHead, uint8 len, uint8* pBufdata)
{
  uint8 i;
  uint8 Cnt;
  uint8 pBufRes[3] = {0};
  //               指令,起始地址, 长度
  uint8 pBufCMD[3] = {0xC1, 0x00, 0x01 };

  pBufCMD[1] = addHead;
  pBufCMD[2] = len;
  
  ConfigLRMode(MODEM_CONFIG); //设置配置模式
  DelayNms(12);               //延时给对方反应时间
  WriteUART1(pBufCMD, 3);     //发送指令
  DelayNms(12);
  Cnt = ReadUART1(&pBufRes[0], 3);//接收响应
  
  for(i=0; i<3; i++)
  {
    if(pBufRes[i] != pBufCMD[i])//接收正确则重复命令
    {
      return 0; //响应不对
    }
  }
  
  return ReadUART1(pBufdata, len)==len? 1:0;
}  

/*********************************************************************************************************
* 函数名称：writeRegister
* 函数功能：写寄存器
* 输入参数：address->寄存器地址, value->待写入的值
* 输出参数：无
* 返 回 值：1-写入成功, 0-写入失败
* 创建日期：2021年11月6日
* 注    意：
*********************************************************************************************************/
static uint8 writeRegister(uint8 address, uint8 value)
{
  //               指令, 地址 , 长度 ,参数
  uint8 pBufCMD[4] = {0xc0, 0x00, 0x01 ,0x00};
  uint8 pBufRes[4];
  uint8 i;
  uint32 lastMillis;
  
  pBufCMD[1] = address;
  pBufCMD[3] = value;
  
  ConfigLRMode(MODEM_CONFIG); //设置配置模式
  
  WriteUART1(pBufCMD, 4);     //发送指令
  
  i=0;
  lastMillis = millis();      //当前毫秒数
  while(i < 4 || millis() - lastMillis <= 100)//收到4字节或等待超过0.1S退出循环
  {
    i += ReadUART1(&pBufRes[i] , 4);
    
  }
  
  ConfigLRMode(MODEM_TRANSFER);   //设置退出配置模式，回到上次模式

  for(i=1; i<4; i++)
  {
    if(pBufRes[i] != pBufCMD[i])
    {
      return 0; //写入失败
    }
  }
  
  return 1;
}

/*********************************************************************************************************
* 函数名称：
* 函数功能：初始化模块参数---恢复出厂设置
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月6日
* 注    意：
*********************************************************************************************************/
static void  DeInitLora()
{
  //出厂默认参数
  uint8 pBufTemp[9] = {0x00, 0x00, 0x00, 0x62, 0x00, 0x17, 0x03, 0x00, 0x00 };
  
  if(!SetLRMode(MODEM_CONFIG))//设置配置模式成功
  {
    WriteUART1(pBufTemp, 9);  //写入串口缓冲区，返回已写入数据的个数
  }
} 
/*********************************************************************************************************
* 函数名称：GetAuxState
* 函数功能：查询模块状态
* 输入参数：
* 输出参数：
* 返 回 值：1-就绪， 0-繁忙
* 创建日期：2021年10月15日
* 注    意：
*********************************************************************************************************/
static uint8  GetAuxState(void)
{
  uint8 ok = 0;
  
  //ok = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4);
  ok = s_Aux;//外部中断更改状态
  
  return ok;
}

/*********************************************************************************************************
*                                              API函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：
* 函数功能：初始化LORA模块,配置LORA串口模块相关的GPIO
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年10月14日
* 注    意：
*********************************************************************************************************/
void  InitLORA(void)         
{
  ConfigLRGPIO();               //初始化连接Lora模块的GPIO
  ConfigLRMode(MODEM_TRANSFER); //初始化Lora模块的模式
  s_Aux      = AUX_STATE_FREE;  //初始化Aux标志位为空闲
  s_RadioBuf = LoRaBufMax;      //初始化Lora接收缓冲区
  s_address  = 0xffff;          //初始化模块默认地址
  s_address  = getAddress();    //读取模块地址
}

/*********************************************************************************************************
* 函数名称：
* 函数功能：设置LORA工作模式
* 输入参数：mode
* 输出参数：
* 返 回 值：0---成功设置模式，1---设置模式失败
* 创建日期：2021年10月14日
* 注    意：mode只能为 0 ,1 ,2 ,3
*********************************************************************************************************/
uint8  SetLRMode(uint8 mode)
{
  uint8 ok = 1;
  ok = ConfigLRMode(mode);//设置LORA工作模式
  return ok;
}

/*********************************************************************************************************
* 函数名称：DeInitLORA
* 函数功能：初始化模块设置
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年10月16日
* 注    意：配置模式下LoRa模块串口波特率固定为9600,8N1。
*********************************************************************************************************/
void  DeInitLORA(void)
{
  DeInitLora();
  
}

/*********************************************************************************************************
* 函数名称：isLoRaReady
* 函数功能：查询模块是否准备好
* 输入参数：void
* 输出参数：无
* 返 回 值：1--已准备好， 0--未准备好
* 创建日期：2021年11月6日
* 注    意：
*********************************************************************************************************/
uint8 isLoRaReady()
{
  return GetAuxState();
}

/*********************************************************************************************************
* 函数名称：RadioSendData
* 函数功能：无线发送数据
* 输入参数：pBufData,size
* 输出参数：无
* 返 回 值：1--发送成功， 0--发送失败
* 创建日期：2021年11月7日
* 注    意：
*********************************************************************************************************/
uint8  RadioSendData(uint8 *pBufData, uint8 size)
{
  uint8 ok_config = 0;//切换模式标志，0---切换失败
  uint32 lastMillis;
  
  if(!GetUART1TxSts() && GetAuxState())// 上一个数据包已发完，串口缓冲区为 空
  {
    DelayNms(5);
    if(GetAuxState())//lora模块空闲，缓冲区为 空
    {
      s_RadioBuf = LoRaBufMax;
    }
    else//Lora模块还在发上一个数据包
    {
      lastMillis = millis();      //当前毫秒数
      while(!GetAuxState() && millis() - lastMillis <= 500)//最多等待x mS
      {
      
      }
      DelayNms(3 + rand()%4);
    }
  }
  else//Lora模块还在发上一个数据包
  {
    lastMillis = millis();      //当前毫秒数
    while(GetUART1TxSts() && millis() - lastMillis <= 500)//最多等待x mS
    {
      
    }
    DelayNms(10 + rand()%4);//等待时间太长，检测模块空闲已经过时
  }
  
  if(s_curMode == MODEM_TRANSFER)
  {
    ok_config = 1;
  }
  else//切换到传输模式
  {
    ok_config = ConfigLRMode(MODEM_TRANSFER);
    s_RadioBuf = LoRaBufMax;
  }
  
  if(ok_config && s_RadioBuf >= size)
  {
    if(size > WriteUART1(pBufData, size))
    {
      debug("RadioSendData中串口缓冲区溢出\r\n");
      return 0;
    }
    s_RadioBuf -= size;
    return 1;
  }
  else if(!ok_config)
  {
    debug("切换传输模式失败462\r\n");
  }
  else if(s_RadioBuf < size)
  {
    debug("数据太多lora接收缓冲区溢出\r\n");
  }
  return 0;
}

/*********************************************************************************************************
* 函数名称：
* 函数功能：
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月7日
* 注    意：
*********************************************************************************************************/
void  RadioSendCMD(void)
{
  RadioModems_t lastStatu = s_curMode;
  
  if(s_curMode != MODEM_CONFIG)
  {
    ConfigLRMode(MODEM_CONFIG);
    while(!GetAuxState());    //等待切换模式成功
  }
  
  //RadioSendData();
  
  
  
  ConfigLRMode(lastStatu);
}

/*********************************************************************************************************
* 函数名称：getAddress
* 函数功能：返回模块地址
* 输入参数：
* 输出参数：
* 返 回 值：uint16地址
* 创建日期：2021年11月7日
* 注    意：读取失败，返回0xffff,
*********************************************************************************************************/
uint16 getAddress(void)
{
  uint16 add = 0xFFFF;
  uint8 pAdd[3] = {0x00};
  
  if(s_address != 0xFFFF)
  {
    return s_address;
  }
  
  if(readRegisters(REG_ADDH, 2, pAdd))//读取成功
  {
    add = pAdd[0];
    add <<= 8;
    add = pAdd[1];
    s_address = add;
  }
  else{
    debug("读取Lora模块寄存器地址失败，当前地址%d\r\n",add);
  }
  debug("s_address:%d\r\n",s_address);
  return add;
}

/*********************************************************************************************************
* 函数名称：
* 函数功能：
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月6日
* 注    意：
*********************************************************************************************************/

/*********************************************************************************************************
* 函数名称：
* 函数功能：
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月6日
* 注    意：
*********************************************************************************************************/

/*********************************************************************************************************
* 函数名称：
* 函数功能：
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月6日
* 注    意：
*********************************************************************************************************/

/*********************************************************************************************************
* 函数名称：
* 函数功能：
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月6日
* 注    意：
*********************************************************************************************************/
