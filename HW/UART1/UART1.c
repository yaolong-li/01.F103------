/*********************************************************************************************************
* ģ�����ƣ�UART1.c
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
#include "UART1.h"
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
static  StructCirQue s_structUART1SendCirQue;  //���ʹ���ѭ������
static  StructCirQue s_structUART1RecCirQue;   //���մ���ѭ������
static  uint8  s_arrSendBuf1[UART1_BUF_SIZE];     //���ʹ���ѭ�����еĻ�����
static  uint8  s_arrRecBuf1[UART1_BUF_SIZE];      //���մ���ѭ�����еĻ�����

static  uint8  s_iUART1TxSts;                     //���ڷ�������״̬
          
/*********************************************************************************************************
*                                              �ڲ���������
*********************************************************************************************************/
static  void  InitUART1Buf(void);      //��ʼ�����ڻ��������������ͻ������ͽ��ջ����� 
static  uint8    WriteReceiveBuf1(uint8 d);  //�����յ�������д��UART1���ջ�����
static  uint8    ReadSendBuf1(uint8 *p);     //��ȡUART1���ͻ������е�����
                                            
static  void  ConfigUART1(uint32 bound);  //����UART1������صĲ���������GPIO��RCC��USART��NVIC 
static  void  EnableUART1Tx(void);     //ʹ��UART1���ڷ��ͣ�WriteUARTx�е��ã�ÿ�η�������֮����Ҫ����                                      
                                            
static  void  SendCharUsedByFputc(uint16 ch);  //�����ַ�������ר��fputc��������,��printf�������
  
/*********************************************************************************************************
*                                              �ڲ�����ʵ��
*********************************************************************************************************/
/*********************************************************************************************************
* �������ƣ�InitUART1Buf
* �������ܣ���ʼ�����ڻ��������������ͻ������ͽ��ջ�����  
* ���������void
* ���������void
* �� �� ֵ��void 
* �������ڣ�2021��07��11��
* ע    �⣺
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
* �������ƣ�WriteReceiveBuf1
* �������ܣ�д���ݵ����ڽ��ջ����� 
* ���������d����д�봮�ڽ��ջ�����������
* ���������void
* �� �� ֵ��д�����ݳɹ���־��0-���ɹ���1-�ɹ� 
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
static  uint8  WriteReceiveBuf1(uint8 d)
{
  uint8 ok = 0;  //д�����ݳɹ���־��0-���ɹ���1-�ɹ�
                                                                    
  ok = EnQueue(&s_structUART1RecCirQue, &d, 1);   
                                                                    
  return ok;  //����д�����ݳɹ���־��0-���ɹ���1-�ɹ� 
}

/*********************************************************************************************************
* �������ƣ�ReadSendBuf1
* �������ܣ���ȡ���ڷ��ͻ������е����� 
* ���������p�������������ݴ�ŵ��׵�ַ
* ���������p�������������ݴ�ŵ��׵�ַ
* �� �� ֵ����ȡ���ݳɹ���־��0-���ɹ���1-�ɹ� 
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
static  uint8  ReadSendBuf1(uint8 *p)
{
  uint8 ok = 0;  //��ȡ���ݳɹ���־��0-���ɹ���1-�ɹ�
                                                                   
  ok = DeQueue(&s_structUART1SendCirQue, p, 1);  
                                                                   
  return ok;  //���ض�ȡ���ݳɹ���־��0-���ɹ���1-�ɹ� 
}

/*********************************************************************************************************
* �������ƣ�ConfigUART1
* �������ܣ����ô�����صĲ���������GPIO��RCC��USART��NVIC  
* ���������bound��������
* ���������void
* �� �� ֵ��void
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
static  void  ConfigUART1(uint32 bound)
{
  GPIO_InitTypeDef  GPIO_InitStructure;   //GPIO_InitStructure���ڴ��GPIO�Ĳ���
  USART_InitTypeDef USART_InitStructure;  //USART_InitStructure���ڴ��USART�Ĳ���
  NVIC_InitTypeDef  NVIC_InitStructure;   //NVIC_InitStructure���ڴ��NVIC�Ĳ���
  
  //ʹ��RCC���ʱ��                                                            
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);  //ʹ��USART1��ʱ��    
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);   //ʹ��GPIOA��ʱ��
  
  //����TX��GPIO 
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;               //����TX������
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;          //����TX��ģʽ
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;         //����TX��I/O������ٶ�
  GPIO_Init(GPIOA, &GPIO_InitStructure);                    //���ݲ�����ʼ��TX��GPIO
  
  //����RX��GPIO
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;              //����RX������
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
  USART_Init(USART1, &USART_InitStructure);                 //���ݲ�����ʼ��USART1

  //����NVIC
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;         //�ж�ͨ����
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //������ռ���ȼ�
  NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0; //���������ȼ�
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           //ʹ���ж�
  NVIC_Init(&NVIC_InitStructure);                           //���ݲ�����ʼ��NVIC

  //ʹ��USART1�����ж�
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  //ʹ�ܽ��ջ������ǿ��ж�
  USART_ITConfig(USART1, USART_IT_TXE,  ENABLE);  //ʹ�ܷ��ͻ��������ж�
  USART_Cmd(USART1, ENABLE);                      //ʹ��USART1
                                                                     
  s_iUART1TxSts = UART_STATE_OFF;  //���ڷ�������״̬����Ϊδ��������
}

/*********************************************************************************************************
* �������ƣ�EnableUART1Tx
* �������ܣ�ʹ�ܴ��ڷ��ͣ���WriteUARTx�е��ã���ÿ�η�������֮����Ҫ�������������ʹ�ܷ����ж� 
* ���������void
* ���������void
* �� �� ֵ��void 
* �������ڣ�2021��07��11��
* ע    �⣺s_iUART1TxSts = UART_STATE_ON;��仰�������USART_ITConfig֮ǰ������ᵼ���жϴ��޷�ִ��
*********************************************************************************************************/
static  void  EnableUART1Tx(void)
{
  s_iUART1TxSts = UART_STATE_ON;                     //���ڷ�������״̬����Ϊ���ڷ�������

  USART_ITConfig(USART1, USART_IT_TXE, ENABLE);     //ʹ�ܷ����ж�
}

/*********************************************************************************************************
* �������ƣ�SendCharUsedByFputc
* �������ܣ������ַ�������ר��fputc��������  
* ���������ch�������͵��ַ�
* ���������void
* �� �� ֵ��void 
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
static  void  SendCharUsedByFputc(uint16 ch)
{  
  USART_SendData(USART1, (uint8)ch);

  //�ȴ��������
  while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
  {
    
  }
}

/*********************************************************************************************************
* �������ƣ�USART1_IRQHandler
* �������ܣ�USART1�жϷ����� 
* ���������void
* ���������void
* �� �� ֵ��void
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
void USART1_IRQHandler(void)                
{
  uint8  uData = 0x07;

  if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //���ջ������ǿ��жϣ����ڽ��յ����ݣ�
  {                                                         
    NVIC_ClearPendingIRQ(USART1_IRQn);  //���USART1�жϹ���
    uData = USART_ReceiveData(USART1);  //��USART1���յ������ݱ��浽uData
                                                          
    WriteReceiveBuf1(uData);  //�����յ�������д����ջ�����                                 
  }                                                         
                                                            
  if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) == SET) //��������־Ϊ1�����ڽ��������
  {                                                         
    USART_ClearFlag(USART1, USART_FLAG_ORE);             //�����������־
    USART_ReceiveData(USART1);                           //��ȡUSART_DR 
  }                                                         
                                                           
  if(USART_GetITStatus(USART1, USART_IT_TXE)!= RESET)    //���ͻ��������жϣ����ڷ�����ɣ�
  {                                                        
    USART_ClearITPendingBit(USART1, USART_IT_TXE);       //��������жϱ�־
    NVIC_ClearPendingIRQ(USART1_IRQn);                   //���USART1�жϹ���
                                                           
    if(1 == ReadSendBuf1(&uData))                           //��ȡ���ͻ����������ݵ�uData
    {                                                                
      USART_SendData(USART1, uData);                       //��uDataд��USART_DR
    }
    
    if(QueueEmpty(&s_structUART1SendCirQue))              //�����ͻ�����Ϊ��ʱ
    {                                                               
      s_iUART1TxSts = UART_STATE_OFF;                     //���ڷ�������״̬����Ϊδ��������       
      USART_ITConfig(USART1, USART_IT_TXE, DISABLE);     //�رմ��ڷ��ͻ��������ж�
    }
  }
} 

/*********************************************************************************************************
*                                              API����ʵ��
*********************************************************************************************************/
/*********************************************************************************************************
* �������ƣ�InitUART1
* �������ܣ���ʼ��UARTģ�� 
* ���������bound,������
* ���������void
* �� �� ֵ��void
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
void InitUART1(uint32 bound)
{
  InitUART1Buf();        //��ʼ�����ڻ��������������ͻ������ͽ��ջ�����  
                  
  ConfigUART1(bound);    //���ô�����صĲ���������GPIO��RCC��USART��NVIC  
}

/*********************************************************************************************************
* �������ƣ�WriteUART1
* �������ܣ�д���ڣ���д���ݵ��Ĵ��ڷ��ͻ�����  
* ���������pBuf��Ҫд�����ݵ��׵�ַ��len������д�����ݵĸ���,���1000�ֽ�
* ���������void
* �� �� ֵ���ɹ�д�����ݵĸ�������һ�����β�len���
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
uint8  WriteUART1(uint8 *pBuf, uint8 len)
{
  uint8 wLen = 0;  //ʵ��д�����ݵĸ���
                                                                  
  wLen = EnQueue(&s_structUART1SendCirQue, pBuf, len);

  if(wLen < UART1_BUF_SIZE)
  {
    if(s_iUART1TxSts == UART_STATE_OFF)
    {
      EnableUART1Tx();//ʹ���жϷ���
    }    
  }
  
  if(wLen < len)
  {
    printf("UART1���ͻ��������,��Ҫ%d���ֽ�",len - wLen);
  }
  
  return wLen;  //����ʵ��д�����ݵĸ���
}

/*********************************************************************************************************
* �������ƣ�ReadUART1
* �������ܣ������ڣ�����ȡ���ڽ��ջ������е�����  
* ���������pBuf����ȡ�����ݴ�ŵ��׵�ַ��len��������ȡ���ݵĸ���
* ���������pBuf����ȡ�����ݴ�ŵ��׵�ַ
* �� �� ֵ���ɹ���ȡ���ݵĸ�������һ�����β�len���
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
uint8  ReadUART1(uint8 *pBuf, uint8 len)
{
  uint8 rLen = 0;  //ʵ�ʶ�ȡ���ݳ���
                                                    
  rLen = DeQueue(&s_structUART1RecCirQue, pBuf, len);

  return rLen;  //����ʵ�ʶ�ȡ���ݵĳ���
}
    
/*********************************************************************************************************
* �������ƣ�fputc
* �������ܣ��ض�����  
* ���������ch��f
* ���������void
* �� �� ֵ��int 
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
int fputc(int ch, FILE* f)
{
  SendCharUsedByFputc((uint8) ch);  //�����ַ�������ר��fputc��������
                               
  return ch;                     //����ch
}
