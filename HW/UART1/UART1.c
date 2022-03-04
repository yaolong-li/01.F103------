/*********************************************************************************************************
* 模块名称：UART1.c
* 摘    要：串口模块，包括串口模块初始化，以及中断服务函数处理，以及读写串口函数实现
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

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include "UART1.h"
#include "Queue.h"

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              枚举结构体定义
*********************************************************************************************************/
//串口发送状态
typedef enum
{
  UART_STATE_OFF, //串口未发送数据
  UART_STATE_ON,  //串口正在发送数据
  UART_STATE_MAX
}EnumUARTState;             

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/    
static  StructCirQue s_structUART1SendCirQue;  //发送串口循环队列
static  StructCirQue s_structUART1RecCirQue;   //接收串口循环队列
static  uint8  s_arrSendBuf1[UART1_BUF_SIZE];     //发送串口循环队列的缓冲区
static  uint8  s_arrRecBuf1[UART1_BUF_SIZE];      //接收串口循环队列的缓冲区

static  uint8  s_iUART1TxSts;                     //串口发送数据状态
          
/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static  void  InitUART1Buf(void);      //初始化串口缓冲区，包括发送缓冲区和接收缓冲区 
static  uint8    WriteReceiveBuf1(uint8 d);  //将接收到的数据写入UART1接收缓冲区
static  uint8    ReadSendBuf1(uint8 *p);     //读取UART1发送缓冲区中的数据
                                            
static  void  ConfigUART1(uint32 bound);  //配置UART1串口相关的参数，包括GPIO、RCC、USART和NVIC 
static  void  EnableUART1Tx(void);     //使能UART1串口发送，WriteUARTx中调用，每次发送数据之后需要调用                                      
                                            
static  void  SendCharUsedByFputc(uint16 ch);  //发送字符函数，专由fputc函数调用,与printf函数相关
  
/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：InitUART1Buf
* 函数功能：初始化串口缓冲区，包括发送缓冲区和接收缓冲区  
* 输入参数：void
* 输出参数：void
* 返 回 值：void 
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
static  void  InitUART1Buf(void)
{
  int16 i;

  for(i = 0; i < UART1_BUF_SIZE; i++)
  {
    s_arrSendBuf1[i] = 0;
    s_arrRecBuf1[i]  = 0;  
  }

  InitQueue(&s_structUART1SendCirQue, s_arrSendBuf1, UART1_BUF_SIZE);
  InitQueue(&s_structUART1RecCirQue,  s_arrRecBuf1,  UART1_BUF_SIZE);
}

/*********************************************************************************************************
* 函数名称：WriteReceiveBuf1
* 函数功能：写数据到串口接收缓冲区 
* 输入参数：d，待写入串口接收缓冲区的数据
* 输出参数：void
* 返 回 值：写入数据成功标志，0-不成功，1-成功 
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
static  uint8  WriteReceiveBuf1(uint8 d)
{
  uint8 ok = 0;  //写入数据成功标志，0-不成功，1-成功
                                                                    
  ok = EnQueue(&s_structUART1RecCirQue, &d, 1);   
                                                                    
  return ok;  //返回写入数据成功标志，0-不成功，1-成功 
}

/*********************************************************************************************************
* 函数名称：ReadSendBuf1
* 函数功能：读取串口发送缓冲区中的数据 
* 输入参数：p，读出来的数据存放的首地址
* 输出参数：p，读出来的数据存放的首地址
* 返 回 值：读取数据成功标志，0-不成功，1-成功 
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
static  uint8  ReadSendBuf1(uint8 *p)
{
  uint8 ok = 0;  //读取数据成功标志，0-不成功，1-成功
                                                                   
  ok = DeQueue(&s_structUART1SendCirQue, p, 1);  
                                                                   
  return ok;  //返回读取数据成功标志，0-不成功，1-成功 
}

/*********************************************************************************************************
* 函数名称：ConfigUART1
* 函数功能：配置串口相关的参数，包括GPIO、RCC、USART和NVIC  
* 输入参数：bound，波特率
* 输出参数：void
* 返 回 值：void
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
static  void  ConfigUART1(uint32 bound)
{
  GPIO_InitTypeDef  GPIO_InitStructure;   //GPIO_InitStructure用于存放GPIO的参数
  USART_InitTypeDef USART_InitStructure;  //USART_InitStructure用于存放USART的参数
  NVIC_InitTypeDef  NVIC_InitStructure;   //NVIC_InitStructure用于存放NVIC的参数
  
  //使能RCC相关时钟                                                            
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);  //使能USART1的时钟    
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   //使能GPIOA的时钟
  
  //配置TX的GPIO 
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;               //设置TX的引脚
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;          //设置TX的模式
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;         //设置TX的I/O口输出速度
  GPIO_Init(GPIOA, &GPIO_InitStructure);                    //根据参数初始化TX的GPIO
  
  //配置RX的GPIO
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;              //设置RX的引脚
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;    //设置RX的模式
  GPIO_Init(GPIOA, &GPIO_InitStructure);                    //根据参数初始化RX的GPIO
  
  //配置USART的参数
  USART_StructInit(&USART_InitStructure);                   //初始化USART_InitStructure
  USART_InitStructure.USART_BaudRate   = bound;             //设置波特率
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;   //设置数据字长度
  USART_InitStructure.USART_StopBits   = USART_StopBits_1;  //设置停止位
  USART_InitStructure.USART_Parity     = USART_Parity_No;   //设置奇偶校验位
  USART_InitStructure.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx;           //设置模式
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //设置硬件流控制模式
  USART_Init(USART1, &USART_InitStructure);                 //根据参数初始化USART1

  //配置NVIC
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;         //中断通道号
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //设置抢占优先级
  NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0; //设置子优先级
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           //使能中断
  NVIC_Init(&NVIC_InitStructure);                           //根据参数初始化NVIC

  //使能USART1及其中断
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  //使能接收缓冲区非空中断
  USART_ITConfig(USART1, USART_IT_TXE,  ENABLE);  //使能发送缓冲区空中断
  USART_Cmd(USART1, ENABLE);                      //使能USART1
                                                                     
  s_iUART1TxSts = UART_STATE_OFF;  //串口发送数据状态设置为未发送数据
}

/*********************************************************************************************************
* 函数名称：EnableUART1Tx
* 函数功能：使能串口发送，在WriteUARTx中调用，即每次发送数据之后需要调用这个函数来使能发送中断 
* 输入参数：void
* 输出参数：void
* 返 回 值：void 
* 创建日期：2021年07月11日
* 注    意：s_iUART1TxSts = UART_STATE_ON;这句话必须放在USART_ITConfig之前，否则会导致中断打开无法执行
*********************************************************************************************************/
static  void  EnableUART1Tx(void)
{
  s_iUART1TxSts = UART_STATE_ON;                     //串口发送数据状态设置为正在发送数据

  USART_ITConfig(USART1, USART_IT_TXE, ENABLE);     //使能发送中断
}

/*********************************************************************************************************
* 函数名称：SendCharUsedByFputc
* 函数功能：发送字符函数，专由fputc函数调用  
* 输入参数：ch，待发送的字符
* 输出参数：void
* 返 回 值：void 
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
static  void  SendCharUsedByFputc(uint16 ch)
{  
  USART_SendData(USART1, (uint8)ch);

  //等待发送完毕
  while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
  {
    
  }
}

/*********************************************************************************************************
* 函数名称：USART1_IRQHandler
* 函数功能：USART1中断服务函数 
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
void USART1_IRQHandler(void)                
{
  uint8  uData = 0x07;

  if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //接收缓冲区非空中断（串口接收到数据）
  {                                                         
    NVIC_ClearPendingIRQ(USART1_IRQn);  //清除USART1中断挂起
    uData = USART_ReceiveData(USART1);  //将USART1接收到的数据保存到uData
                                                          
    WriteReceiveBuf1(uData);  //将接收到的数据写入接收缓冲区                                 
  }                                                         
                                                            
  if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) == SET) //溢出错误标志为1（串口接收溢出）
  {                                                         
    USART_ClearFlag(USART1, USART_FLAG_ORE);             //清除溢出错误标志
    USART_ReceiveData(USART1);                           //读取USART_DR 
  }                                                         
                                                           
  if(USART_GetITStatus(USART1, USART_IT_TXE)!= RESET)    //发送缓冲区空中断（串口发送完成）
  {                                                        
    USART_ClearITPendingBit(USART1, USART_IT_TXE);       //清除发送中断标志
    NVIC_ClearPendingIRQ(USART1_IRQn);                   //清除USART1中断挂起
                                                           
    if(1 == ReadSendBuf1(&uData))                           //读取发送缓冲区的数据到uData
    {                                                                
      USART_SendData(USART1, uData);                       //将uData写入USART_DR
    }
    
    if(QueueEmpty(&s_structUART1SendCirQue))              //当发送缓冲区为空时
    {                                                               
      s_iUART1TxSts = UART_STATE_OFF;                     //串口发送数据状态设置为未发送数据       
      USART_ITConfig(USART1, USART_IT_TXE, DISABLE);     //关闭串口发送缓冲区空中断
    }
  }
} 

/*********************************************************************************************************
*                                              API函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：InitUART1
* 函数功能：初始化UART模块 
* 输入参数：bound,波特率
* 输出参数：void
* 返 回 值：void
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
void InitUART1(uint32 bound)
{
  InitUART1Buf();        //初始化串口缓冲区，包括发送缓冲区和接收缓冲区  
                  
  ConfigUART1(bound);    //配置串口相关的参数，包括GPIO、RCC、USART和NVIC  
}

/*********************************************************************************************************
* 函数名称：WriteUART1
* 函数功能：写串口，即写数据到的串口发送缓冲区  
* 输入参数：pBuf，要写入数据的首地址，len，期望写入数据的个数,最大1000字节
* 输出参数：void
* 返 回 值：成功写入数据的个数，不一定与形参len相等
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
uint8  WriteUART1(uint8 *pBuf, uint8 len)
{
  uint8 wLen = 0;  //实际写入数据的个数
                                                                  
  wLen = EnQueue(&s_structUART1SendCirQue, pBuf, len);

  if(wLen < UART1_BUF_SIZE)
  {
    if(s_iUART1TxSts == UART_STATE_OFF)
    {
      EnableUART1Tx();//使能中断发送
    }    
  }
  
  if(wLen < len)
  {
    printf("UART1发送缓冲区溢出,还要%d个字节",len - wLen);
  }
  
  return wLen;  //返回实际写入数据的个数
}

/*********************************************************************************************************
* 函数名称：ReadUART1
* 函数功能：读串口，即读取串口接收缓冲区中的数据  
* 输入参数：pBuf，读取的数据存放的首地址，len，期望读取数据的个数
* 输出参数：pBuf，读取的数据存放的首地址
* 返 回 值：成功读取数据的个数，不一定与形参len相等
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
uint8  ReadUART1(uint8 *pBuf, uint8 len)
{
  uint8 rLen = 0;  //实际读取数据长度
                                                    
  rLen = DeQueue(&s_structUART1RecCirQue, pBuf, len);

  return rLen;  //返回实际读取数据的长度
}
    
/*********************************************************************************************************
* 函数名称：fputc
* 函数功能：重定向函数  
* 输入参数：ch，f
* 输出参数：void
* 返 回 值：int 
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
int fputc(int ch, FILE* f)
{
  SendCharUsedByFputc((uint8) ch);  //发送字符函数，专由fputc函数调用
                               
  return ch;                     //返回ch
}
