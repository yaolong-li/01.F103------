/*********************************************************************************************************
* ģ�����ƣ�UART2.c
* ժ    Ҫ������ģ�飬��������ģ���ʼ�����Լ��жϷ����������Լ���д���ں���ʵ��
* ��ǰ�汾��1.0.0
* ��    �ߣ�SZLY(COPYRIGHT 2018 - 2020 SZLY. All rights reserved.)
* ������ڣ�2020��01��01�� 
* ��    �ݣ�
* ע    �⣺                                                                  
**********************************************************************************************************
* ȡ���汾��
* ��    �ߣ�
* ������ڣ�
* �޸����ݣ�
* �޸��ļ���
*********************************************************************************************************/

/*********************************************************************************************************
*                                              ����ͷ�ļ�
*********************************************************************************************************/
#include "UART2.h"
#include "Queue.h"

/*********************************************************************************************************
*                                              �궨��
*********************************************************************************************************/

/*********************************************************************************************************
*                                              ö�ٽṹ�嶨��
*********************************************************************************************************/
//���ڷ���״̬
typedef enum
{
  UART_STATE_OFF, //����δ��������
  UART_STATE_ON,  //�������ڷ�������
  UART_STATE_MAX
}EnumUARTState;             

/*********************************************************************************************************
*                                              �ڲ�����
*********************************************************************************************************/    
static  StructCirQue s_structUART2SendCirQue;  //���ʹ���ѭ������
static  StructCirQue s_structUART2RecCirQue;   //���մ���ѭ������
static  uint8  s_arrSendBuf2[UART2_BUF_SIZE];     //���ʹ���ѭ�����еĻ�����
static  uint8  s_arrRecBuf2[UART2_BUF_SIZE];      //���մ���ѭ�����еĻ�����

static  uint8  s_iUART2TxSts;                     //���ڷ�������״̬
          
/*********************************************************************************************************
*                                              �ڲ���������
*********************************************************************************************************/
static  void  InitUART2Buf(void);      //��ʼ�����ڻ��������������ͻ������ͽ��ջ����� 
static  uint8    WriteReceiveBuf2(uint8 d);  //�����յ�������д��UART2���ջ�����
static  uint8    ReadSendBuf2(uint8 *p);     //��ȡUART2���ͻ������е�����
                                            
static  void  ConfigUART2(uint32 bound);  //����UART2������صĲ���������GPIO��RCC��USART��NVIC 
static  void  EnableUART2Tx(void);     //ʹ��UART2���ڷ��ͣ�WriteUARTx�е��ã�ÿ�η�������֮����Ҫ����                                      
                                            
  
/*********************************************************************************************************
*                                              �ڲ�����ʵ��
*********************************************************************************************************/
/*********************************************************************************************************
* �������ƣ�InitUART2Buf
* �������ܣ���ʼ�����ڻ��������������ͻ������ͽ��ջ�����  
* ���������void
* ���������void
* �� �� ֵ��void 
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
static  void  InitUART2Buf(void)
{
  int16 i;

  for(i = 0; i < UART2_BUF_SIZE; i++)
  {
    s_arrSendBuf2[i] = 0;
    s_arrRecBuf2[i]  = 0;  
  }

  InitQueue(&s_structUART2SendCirQue, s_arrSendBuf2, UART2_BUF_SIZE);
  InitQueue(&s_structUART2RecCirQue,  s_arrRecBuf2,  UART2_BUF_SIZE);
}

/*********************************************************************************************************
* �������ƣ�WriteReceiveBuf2
* �������ܣ�д���ݵ����ڽ��ջ����� 
* ���������d����д�봮�ڽ��ջ�����������
* ���������void
* �� �� ֵ��д�����ݳɹ���־��0-���ɹ���1-�ɹ� 
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
static  uint8  WriteReceiveBuf2(uint8 d)
{
  uint8 ok = 0;  //д�����ݳɹ���־��0-���ɹ���1-�ɹ�
                                                                    
  ok = EnQueue(&s_structUART2RecCirQue, &d, 1);   
                                                                    
  return ok;  //����д�����ݳɹ���־��0-���ɹ���1-�ɹ� 
}

/*********************************************************************************************************
* �������ƣ�ReadSendBuf2
* �������ܣ���ȡ���ڷ��ͻ������е����� 
* ���������p�������������ݴ�ŵ��׵�ַ
* ���������p�������������ݴ�ŵ��׵�ַ
* �� �� ֵ����ȡ���ݳɹ���־��0-���ɹ���1-�ɹ� 
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
static  uint8  ReadSendBuf2(uint8 *p)
{
  uint8 ok = 0;  //��ȡ���ݳɹ���־��0-���ɹ���1-�ɹ�
                                                                   
  ok = DeQueue(&s_structUART2SendCirQue, p, 1);  
                                                                   
  return ok;  //���ض�ȡ���ݳɹ���־��0-���ɹ���1-�ɹ� 
}

/*********************************************************************************************************
* �������ƣ�ConfigUART2
* �������ܣ����ô�����صĲ���������GPIO��RCC��USART��NVIC  
* ���������bound��������
* ���������void
* �� �� ֵ��void
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
static  void  ConfigUART2(uint32 bound)
{
  GPIO_InitTypeDef  GPIO_InitStructure;   //GPIO_InitStructure���ڴ��GPIO�Ĳ���
  USART_InitTypeDef USART_InitStructure;  //USART_InitStructure���ڴ��USART�Ĳ���
  NVIC_InitTypeDef  NVIC_InitStructure;   //NVIC_InitStructure���ڴ��NVIC�Ĳ���
  
  //ʹ��RCC���ʱ��                                                            
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);  //ʹ��USART2��ʱ��    
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   //ʹ��GPIOA��ʱ��
  
  //����TX��GPIO 
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;               //����TX������
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;          //����TX��ģʽ
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;         //����TX��I/O������ٶ�
  GPIO_Init(GPIOA, &GPIO_InitStructure);                    //���ݲ�����ʼ��TX��GPIO
  
  //����RX��GPIO
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;              //����RX������
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;    //����RX��ģʽ
  GPIO_Init(GPIOA, &GPIO_InitStructure);                    //���ݲ�����ʼ��RX��GPIO
  
  //����USART�Ĳ���
  USART_StructInit(&USART_InitStructure);                   //��ʼ��USART_InitStructure
  USART_InitStructure.USART_BaudRate   = bound;             //���ò�����
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;   //���������ֳ���
  USART_InitStructure.USART_StopBits   = USART_StopBits_1;  //����ֹͣλ
  USART_InitStructure.USART_Parity     = USART_Parity_No;   //������żУ��λ
  USART_InitStructure.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx;           //����ģʽ
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //����Ӳ��������ģʽ
  USART_Init(USART2, &USART_InitStructure);                 //���ݲ�����ʼ��USART2

  //����NVIC
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;         //�ж�ͨ����
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //������ռ���ȼ�
  NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1; //���������ȼ�
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           //ʹ���ж�
  NVIC_Init(&NVIC_InitStructure);                           //���ݲ�����ʼ��NVIC

  //ʹ��USART2�����ж�
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);  //ʹ�ܽ��ջ������ǿ��ж�
  USART_ITConfig(USART2, USART_IT_TXE,  ENABLE);  //ʹ�ܷ��ͻ��������ж�
  USART_Cmd(USART2, ENABLE);                      //ʹ��USART2
                                                                     
  s_iUART2TxSts = UART_STATE_OFF;  //���ڷ�������״̬����Ϊδ��������
}

/*********************************************************************************************************
* �������ƣ�EnableUART2Tx
* �������ܣ�ʹ�ܴ��ڷ��ͣ���WriteUARTx�е��ã���ÿ�η�������֮����Ҫ�������������ʹ�ܷ����ж� 
* ���������void
* ���������void
* �� �� ֵ��void 
* �������ڣ�2021��07��11��
* ע    �⣺s_iUART2TxSts = UART_STATE_ON;��仰�������USART_ITConfig֮ǰ������ᵼ���жϴ��޷�ִ��
*********************************************************************************************************/
static  void  EnableUART2Tx(void)
{
  s_iUART2TxSts = UART_STATE_ON;                     //���ڷ�������״̬����Ϊ���ڷ�������

  USART_ITConfig(USART2, USART_IT_TXE, ENABLE);     //ʹ�ܷ����ж�
}

/*********************************************************************************************************
* �������ƣ�USART2_IRQHandler
* �������ܣ�USART2�жϷ����� 
* ���������void
* ���������void
* �� �� ֵ��void
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
void USART2_IRQHandler(void)                
{
  uint8  uData = 0x07;

  if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //���ջ������ǿ��жϣ����ڽ��յ����ݣ�
  {                                                         
    NVIC_ClearPendingIRQ(USART2_IRQn);  //���USART2�жϹ���
    uData = USART_ReceiveData(USART2);  //��USART2���յ������ݱ��浽uData
                                                          
    WriteReceiveBuf2(uData);  //�����յ�������д����ջ�����                                 
  }                                                         
                                                            
  if(USART_GetFlagStatus(USART2, USART_FLAG_ORE) == SET) //��������־Ϊ1�����ڽ��������
  {                                                         
    USART_ClearFlag(USART2, USART_FLAG_ORE);             //�����������־
    USART_ReceiveData(USART2);                           //��ȡUSART_DR 
  }                                                         
                                                           
  if(USART_GetITStatus(USART2, USART_IT_TXE)!= RESET)    //���ͻ��������жϣ����ڷ�����ɣ�
  {                                                        
    USART_ClearITPendingBit(USART2, USART_IT_TXE);       //��������жϱ�־
    NVIC_ClearPendingIRQ(USART2_IRQn);                   //���USART2�жϹ���
                                                           
    if(1 == ReadSendBuf2(&uData))                           //��ȡ���ͻ����������ݵ�uData
    {                                                                
      USART_SendData(USART2, uData);                       //��uDataд��USART_DR
    }
    
    if(QueueEmpty(&s_structUART2SendCirQue))              //�����ͻ�����Ϊ��ʱ
    {                                                               
      s_iUART2TxSts = UART_STATE_OFF;                     //���ڷ�������״̬����Ϊδ��������       
      USART_ITConfig(USART2, USART_IT_TXE, DISABLE);     //�رմ��ڷ��ͻ��������ж�
    }
  }
} 

/*********************************************************************************************************
*                                              API����ʵ��
*********************************************************************************************************/
/*********************************************************************************************************
* �������ƣ�InitUART2
* �������ܣ���ʼ��UARTģ�� 
* ���������bound,������
* ���������void
* �� �� ֵ��void
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
void InitUART2(uint32 bound)
{
  InitUART2Buf();        //��ʼ�����ڻ��������������ͻ������ͽ��ջ�����  
                  
  ConfigUART2(bound);    //���ô�����صĲ���������GPIO��RCC��USART��NVIC  
}

/*********************************************************************************************************
* �������ƣ�WriteUART2
* �������ܣ�д���ڣ���д���ݵ��Ĵ��ڷ��ͻ�����  
* ���������pBuf��Ҫд�����ݵ��׵�ַ��len������д�����ݵĸ���,���1000�ֽ�
* ���������void
* �� �� ֵ���ɹ�д�����ݵĸ�������һ�����β�len���
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
uint8  WriteUART2(uint8 *pBuf, uint8 len)
{
  uint8 wLen = 0;  //ʵ��д�����ݵĸ���
                                                                  
  wLen = EnQueue(&s_structUART2SendCirQue, pBuf, len);

  if(wLen < UART2_BUF_SIZE)
  {
    if(s_iUART2TxSts == UART_STATE_OFF)
    {
      EnableUART2Tx();//ʹ���жϷ���
    }    
  }
  
  if(wLen < len)
  {
    printf("UART2���ͻ��������,��Ҫ%d���ֽ�",len - wLen);
  }
  
  return wLen;  //����ʵ��д�����ݵĸ���
}

/*********************************************************************************************************
* �������ƣ�ReadUART2
* �������ܣ������ڣ�����ȡ���ڽ��ջ������е�����  
* ���������pBuf����ȡ�����ݴ�ŵ��׵�ַ��len��������ȡ���ݵĸ���
* ���������pBuf����ȡ�����ݴ�ŵ��׵�ַ
* �� �� ֵ���ɹ���ȡ���ݵĸ�������һ�����β�len���
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
uint8  ReadUART2(uint8 *pBuf, uint8 len)
{
  uint8 rLen = 0;  //ʵ�ʶ�ȡ���ݳ���
                                                    
  rLen = DeQueue(&s_structUART2RecCirQue, pBuf, len);

  return rLen;  //����ʵ�ʶ�ȡ���ݵĳ���
}
    
