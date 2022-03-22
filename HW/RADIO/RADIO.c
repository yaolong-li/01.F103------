/*********************************************************************************************************
* ģ�����ƣ�RADIO.c
* ժ    Ҫ������ģ�飬��������ģ���ʼ�����Լ��жϷ����������Լ���д���ں���ʵ��
* ��ǰ�汾��1.0.0
* ��    �ߣ�LYL(COPYRIGHT 2021 - 2021 LYL. All rights reserved.)
* ������ڣ�2022��2��22�� 
* ��    �ݣ���������:               
            STM32f103RCT6   |   E22-400T22S1C
                PB6     ------->    M0
                PB5     ------->    M1
                PB4     ------->    AUX
                TX1     ------->    TXD  (����)
                RX1     ------->    RXD  (����)
                GND     ------->    GND
* ע    �⣺LORAģ���ѹҪ��ȵ�Ƭ���ߣ�����Ƭ�������߸�LORA���磬��ʱ��Ƶ��ѹ�����󣬷���ʧ�ܡ�                
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
#include "RADIO.h"
#include "Timer.h"
#include "stm32f10x_conf.h"
#include "SysTick.h"
#include "UART1.h"
#include <stdlib.h>

/*********************************************************************************************************
*                                              �궨��
*********************************************************************************************************/
// registers  LORAģ��Ĵ�����ַ
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
*                                              ö�ٽṹ�嶨��
*********************************************************************************************************/
typedef enum
{
  AUX_STATE_BUSSY = 0x00,
  AUX_STATE_FREE  = 0x01,
}Enum_s_Aux_State;

/*********************************************************************************************************
*                                              �ڲ�����
*********************************************************************************************************/
static uint8 s_curMode;       //��ǰģʽ
volatile static uint8  s_Aux; //Auxλ
const static uint8 s_arrSetting[9] = {0xC0, 0x00, 0x00, 0x62, 0x00, 0x17, 0x03, 0x00, 0x00 }; //��������õĲ���
static uint16 s_address;      //Loraģ���ַ
static uint16 s_RadioBuf;     //Loraģ�黺������ǰ��С

/*********************************************************************************************************
*                                              �ڲ���������
*********************************************************************************************************/
static  void  DeInitLora(void);                                 //��ʼ��ģ�����---�ָ���������
static  void  ConfigLRGPIO(void);                               //����LOAR����ģ���뵥Ƭ��������GPIO
static  uint8    ConfigLRMode(uint8 mode);                 //����LORAģ���ģʽ
static  uint8    readRegister(uint8 address);                         //���Ĵ���
static  uint8    readRegisters(uint8 addHead, uint8 len, uint8* pBufdata);  //���Ĵ�����
static  uint8    writeRegister(uint8 address, uint8 value);              //д�Ĵ���
static  uint8    writeRegisters(uint8 addHead, uint8 value);             //д�Ĵ�����
static  uint8    GetAuxState(void);                                //��ѯLORAģ��״̬,1--����, 0--��æ

/*********************************************************************************************************
*                                              �ڲ�����ʵ��
*********************************************************************************************************/
/*********************************************************************************************************
* �������ƣ�ConfigLRGPIO
* �������ܣ�����LORA����ģ����ص�GPIO,��ʼ������ģ��M1,M0,Auxλ��GPIO
* ���������void
* ���������void
* �� �� ֵ��void
* �������ڣ�2021��10��14��
* ע    �⣺
*********************************************************************************************************/
static  void  ConfigLRGPIO(void)  
{
  GPIO_InitTypeDef GPIO_InitStructure;      //GPIO_InitStructure���ڴ��GPIO�Ĳ���
  EXTI_InitTypeDef EXTI_InitStructure;      //EXTI_InitStructure���ڴ���ⲿ�жϵĲ���
  NVIC_InitTypeDef NVIC_InitStructure;      //NVIC_InitStructure���ڴ��NVIC(�����ж����ȼ�)�Ĳ���
  
  //ʹ��RCC���ʱ��
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); //ʹ��GPIOB��ʱ�ӣ�GPIOB�����ƽʹ��
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);  //ʹ��AFIO��ʱ�ӣ����벶��ʹ��
  
  //����Loraģ���M1λ
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;           //��������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;     //����I/O����ٶ�
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;     //����ģʽ
  GPIO_Init(GPIOB, &GPIO_InitStructure);                //���ݲ�����ʼ��M1��GPIO

  GPIO_WriteBit(GPIOB, GPIO_Pin_6, Bit_RESET);          //��M1Ĭ��״̬����Ϊ0
  
  //����Loraģ���M0λ
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;           //��������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;     //����I/O����ٶ�
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;     //����ģʽ
  GPIO_Init(GPIOB, &GPIO_InitStructure);                //���ݲ�����ʼ��M0��GPIO

  GPIO_WriteBit(GPIOB, GPIO_Pin_5, Bit_RESET);           //��M0Ĭ��״̬����Ϊ0
  
  //����Loraģ���Auxλ,�� AUX ����͵�ƽʱ����ʾģ�鷱æ
  GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;            //����Aux������
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;         //����Aux��ģʽ,������������ʱΪ1.��ʾģ�����
  GPIO_Init(GPIOB, &GPIO_InitStructure);                 //���ݲ�����ʼ��Aux��GPIO
   
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource4);//ΪPB4�����ⲿ�ж�
  //�����ⲿ�жϣ����Auxλ
  EXTI_InitStructure.EXTI_Line    = EXTI_Line4;           //ѡ��EXTI4ͨ��
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;               //ʹ���ж�
  EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;  //ģʽѡ��
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  //�����غ��½����ж�
  EXTI_Init(&EXTI_InitStructure);                         //���ݲ�����ʼ��EXTI
  
  //����NVIC
  NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;              //���õ��ж�ͨ����
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;     //������ռ���ȼ�
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;            //���������ȼ�
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;               //ʹ���ж�
  NVIC_Init(&NVIC_InitStructure);                               //���ݲ�����ʼ��NVIC 

}

/*********************************************************************************************************
* �������ƣ�EXTI4_IRQHandler
* �������ܣ�EXTI4�жϷ����� 
* ���������void
* ���������void
* �� �� ֵ��void
* �������ڣ�2022��02��10��
* ע    �⣺PB4(Auxλ)�������жϴ�������LORAģ����Ӧ�ź�
*********************************************************************************************************/
void EXTI4_IRQHandler(void)  
{  
  if(EXTI_GetITStatus(EXTI_Line4) == SET)   //�ж��ⲿ�ж��Ƿ���
  {
    EXTI_ClearITPendingBit(EXTI_Line4);     //����ⲿ�жϱ�־
    
    if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == 1)
    {
      DelayNus(100);//����
      if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == 1)
      {
        s_Aux = AUX_STATE_FREE;                 //�������жϱ�ʾLORAģ�����
      }
    }
    else
    {
      DelayNus(100);//����
      if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == 0)
      {
        s_Aux = AUX_STATE_BUSSY;                 //�½����жϱ�ʾLORAģ�����
      }
    }
    
  }
}

/*********************************************************************************************************
* �������ƣ�ConfigLRMode
* �������ܣ�ģ��ģʽ����
* ���������mode,Ҫ���õ�ģʽ
* �����������
* �� �� ֵ��1---�ɹ�
* �������ڣ�2021��10��14��
* ע    �⣺0-����ģʽ(֧������ָ���������), 1-WORģʽ(֧�ֿ��л���), 2-����ģʽ, 3-�������
           ���ı� M1��M0 ����ģ����У�1ms �󣬼��ɰ����µ�ģʽ��ʼ������
*********************************************************************************************************/
static  uint8  ConfigLRMode(uint8 mode)
{
  uint8  ok = 1;
  uint32 lastMillis;
  
//  lastMillis = millis();    //��ǰ������
//  while((ok = !isLoRaReady()) && (millis() - lastMillis) <= 100);  //�ȴ�ģ����л�ʱ(ms)
//  DelayNms(5);
//  s_Aux = AUX_STATE_BUSSY;  //����æ��־λ
  s_curMode = mode;         //���µ�ǰģʽ
  GPIO_WriteBit(GPIOB, GPIO_Pin_6, ((uint8)mode)&0x01);       //����Loraģ���M0λ
  GPIO_WriteBit(GPIOB, GPIO_Pin_5, (((uint8)mode)>>1)&0x01);  //����Loraģ���M1λ
  lastMillis = millis();    //��ǰ������
  //��������ȷ��ģ���ȿ���(�ߵ�ƽ)���ٷ�æ���͵�ƽ�����ٿ���(�ߵ�ƽ)
  while((ok = !isLoRaReady()) && (millis() - lastMillis) <= 100);  //�ȴ�������ɻ�ʱ(ms)

  return !ok;
}

/*********************************************************************************************************
* �������ƣ�readRegister
* �������ܣ����Ĵ���
* ���������address--�����Ĵ�����ַ
* ���������
* �� �� ֵ��
* �������ڣ�2021��11��6��
* ע    �⣺
*********************************************************************************************************/
static uint8 readRegister(uint8 address)
{
  //               ָ��, ��ַ , ����
  uint8 pBufCMD[3] = {0xc1, 0x00, 0x01 };
  uint8 pBufRes[4];
  uint8 i,response;
  uint32 lastMillis;
  
  pBufCMD[1] = address;
  
  ConfigLRMode(MODEM_CONFIG); //��������ģʽ
  while(!isLoRaReady());      //�ȴ��������
  
  WriteUART1(pBufCMD, 3);     //����ָ��
  
  i=0;
  lastMillis = millis();      //��ǰ������
  while(i < 4 || millis() - lastMillis <= 100)//�յ�4�ֽڻ�ȴ�����0.1S�˳�ѭ��
  {
    i += ReadUART1(&pBufRes[i] , 4);
  }
  ConfigLRMode(MODEM_TRANSFER);   //�����˳�����ģʽ���ص��ϴ�ģʽ
  
  for(i=0; i<3; i++)
  {
    if(pBufRes[i] != pBufCMD[i])
    {
      return 0; //��ȡʧ��
    }
  }
  response = pBufRes[3];
  
  return response;
}

/*********************************************************************************************************
* �������ƣ�readRegisters
* �������ܣ����Ĵ�����
* ���������address--�����Ĵ�����ʼ��ַ��len-Ҫ���Ĵ���������pBufdata-��Ŷ���ֵ��ַ
* �����������
* �� �� ֵ��1--�ɹ���0--ʧ��
* �������ڣ�2022��2��2��
* ע    �⣺
*********************************************************************************************************/
static uint8 readRegisters(uint8 addHead, uint8 len, uint8* pBufdata)
{
  uint8 i;
  uint8 Cnt;
  uint8 pBufRes[3] = {0};
  //               ָ��,��ʼ��ַ, ����
  uint8 pBufCMD[3] = {0xC1, 0x00, 0x01 };

  pBufCMD[1] = addHead;
  pBufCMD[2] = len;
  
  ConfigLRMode(MODEM_CONFIG); //��������ģʽ
  DelayNms(12);               //��ʱ���Է���Ӧʱ��
  WriteUART1(pBufCMD, 3);     //����ָ��
  DelayNms(12);
  Cnt = ReadUART1(&pBufRes[0], 3);//������Ӧ
  
  for(i=0; i<3; i++)
  {
    if(pBufRes[i] != pBufCMD[i])//������ȷ���ظ�����
    {
      return 0; //��Ӧ����
    }
  }
  
  return ReadUART1(pBufdata, len)==len? 1:0;
}  

/*********************************************************************************************************
* �������ƣ�writeRegister
* �������ܣ�д�Ĵ���
* ���������address->�Ĵ�����ַ, value->��д���ֵ
* �����������
* �� �� ֵ��1-д��ɹ�, 0-д��ʧ��
* �������ڣ�2021��11��6��
* ע    �⣺
*********************************************************************************************************/
static uint8 writeRegister(uint8 address, uint8 value)
{
  //               ָ��, ��ַ , ���� ,����
  uint8 pBufCMD[4] = {0xc0, 0x00, 0x01 ,0x00};
  uint8 pBufRes[4];
  uint8 i;
  uint32 lastMillis;
  
  pBufCMD[1] = address;
  pBufCMD[3] = value;
  
  ConfigLRMode(MODEM_CONFIG); //��������ģʽ
  
  WriteUART1(pBufCMD, 4);     //����ָ��
  
  i=0;
  lastMillis = millis();      //��ǰ������
  while(i < 4 || millis() - lastMillis <= 100)//�յ�4�ֽڻ�ȴ�����0.1S�˳�ѭ��
  {
    i += ReadUART1(&pBufRes[i] , 4);
    
  }
  
  ConfigLRMode(MODEM_TRANSFER);   //�����˳�����ģʽ���ص��ϴ�ģʽ

  for(i=1; i<4; i++)
  {
    if(pBufRes[i] != pBufCMD[i])
    {
      return 0; //д��ʧ��
    }
  }
  
  return 1;
}

/*********************************************************************************************************
* �������ƣ�
* �������ܣ���ʼ��ģ�����---�ָ���������
* ���������
* ���������
* �� �� ֵ��
* �������ڣ�2021��11��6��
* ע    �⣺
*********************************************************************************************************/
static void  DeInitLora()
{
  //����Ĭ�ϲ���
  uint8 pBufTemp[9] = {0x00, 0x00, 0x00, 0x62, 0x00, 0x17, 0x03, 0x00, 0x00 };
  
  if(!SetLRMode(MODEM_CONFIG))//��������ģʽ�ɹ�
  {
    WriteUART1(pBufTemp, 9);  //д�봮�ڻ�������������д�����ݵĸ���
  }
} 
/*********************************************************************************************************
* �������ƣ�GetAuxState
* �������ܣ���ѯģ��״̬
* ���������
* ���������
* �� �� ֵ��1-������ 0-��æ
* �������ڣ�2021��10��15��
* ע    �⣺
*********************************************************************************************************/
static uint8  GetAuxState(void)
{
  uint8 ok = 0;
  
  //ok = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4);
  ok = s_Aux;//�ⲿ�жϸ���״̬
  
  return ok;
}

/*********************************************************************************************************
*                                              API����ʵ��
*********************************************************************************************************/
/*********************************************************************************************************
* �������ƣ�
* �������ܣ���ʼ��LORAģ��,����LORA����ģ����ص�GPIO
* ���������
* ���������
* �� �� ֵ��
* �������ڣ�2021��10��14��
* ע    �⣺
*********************************************************************************************************/
void  InitLORA(void)         
{
  ConfigLRGPIO();               //��ʼ������Loraģ���GPIO
  ConfigLRMode(MODEM_TRANSFER); //��ʼ��Loraģ���ģʽ
  s_Aux      = AUX_STATE_FREE;  //��ʼ��Aux��־λΪ����
  s_RadioBuf = LoRaBufMax;      //��ʼ��Lora���ջ�����
  s_address  = 0xffff;          //��ʼ��ģ��Ĭ�ϵ�ַ
  s_address  = getAddress();    //��ȡģ���ַ
}

/*********************************************************************************************************
* �������ƣ�
* �������ܣ�����LORA����ģʽ
* ���������mode
* ���������
* �� �� ֵ��0---�ɹ�����ģʽ��1---����ģʽʧ��
* �������ڣ�2021��10��14��
* ע    �⣺modeֻ��Ϊ 0 ,1 ,2 ,3
*********************************************************************************************************/
uint8  SetLRMode(uint8 mode)
{
  uint8 ok = 1;
  ok = ConfigLRMode(mode);//����LORA����ģʽ
  return ok;
}

/*********************************************************************************************************
* �������ƣ�DeInitLORA
* �������ܣ���ʼ��ģ������
* ���������
* ���������
* �� �� ֵ��
* �������ڣ�2021��10��16��
* ע    �⣺����ģʽ��LoRaģ�鴮�ڲ����ʹ̶�Ϊ9600,8N1��
*********************************************************************************************************/
void  DeInitLORA(void)
{
  DeInitLora();
  
}

/*********************************************************************************************************
* �������ƣ�isLoRaReady
* �������ܣ���ѯģ���Ƿ�׼����
* ���������void
* �����������
* �� �� ֵ��1--��׼���ã� 0--δ׼����
* �������ڣ�2021��11��6��
* ע    �⣺
*********************************************************************************************************/
uint8 isLoRaReady()
{
  return GetAuxState();
}

/*********************************************************************************************************
* �������ƣ�RadioSendData
* �������ܣ����߷�������
* ���������pBufData,size
* �����������
* �� �� ֵ��1--���ͳɹ��� 0--����ʧ��
* �������ڣ�2021��11��7��
* ע    �⣺
*********************************************************************************************************/
uint8  RadioSendData(uint8 *pBufData, uint8 size)
{
  uint8 ok_config = 0;//�л�ģʽ��־��0---�л�ʧ��
  uint32 lastMillis;
  
  if(!GetUART1TxSts() && GetAuxState())// ��һ�����ݰ��ѷ��꣬���ڻ�����Ϊ ��
  {
    DelayNms(5);
    if(GetAuxState())//loraģ����У�������Ϊ ��
    {
      s_RadioBuf = LoRaBufMax;
    }
    else//Loraģ�黹�ڷ���һ�����ݰ�
    {
      lastMillis = millis();      //��ǰ������
      while(!GetAuxState() && millis() - lastMillis <= 500)//���ȴ�x mS
      {
      
      }
      DelayNms(3 + rand()%4);
    }
  }
  else//Loraģ�黹�ڷ���һ�����ݰ�
  {
    lastMillis = millis();      //��ǰ������
    while(GetUART1TxSts() && millis() - lastMillis <= 500)//���ȴ�x mS
    {
      
    }
    DelayNms(10 + rand()%4);//�ȴ�ʱ��̫�������ģ������Ѿ���ʱ
  }
  
  if(s_curMode == MODEM_TRANSFER)
  {
    ok_config = 1;
  }
  else//�л�������ģʽ
  {
    ok_config = ConfigLRMode(MODEM_TRANSFER);
    s_RadioBuf = LoRaBufMax;
  }
  
  if(ok_config && s_RadioBuf >= size)
  {
    if(size > WriteUART1(pBufData, size))
    {
      debug("RadioSendData�д��ڻ��������\r\n");
      return 0;
    }
    s_RadioBuf -= size;
    return 1;
  }
  else if(!ok_config)
  {
    debug("�л�����ģʽʧ��462\r\n");
  }
  else if(s_RadioBuf < size)
  {
    debug("����̫��lora���ջ��������\r\n");
  }
  return 0;
}

/*********************************************************************************************************
* �������ƣ�
* �������ܣ�
* ���������
* ���������
* �� �� ֵ��
* �������ڣ�2021��11��7��
* ע    �⣺
*********************************************************************************************************/
void  RadioSendCMD(void)
{
  RadioModems_t lastStatu = s_curMode;
  
  if(s_curMode != MODEM_CONFIG)
  {
    ConfigLRMode(MODEM_CONFIG);
    while(!GetAuxState());    //�ȴ��л�ģʽ�ɹ�
  }
  
  //RadioSendData();
  
  
  
  ConfigLRMode(lastStatu);
}

/*********************************************************************************************************
* �������ƣ�getAddress
* �������ܣ�����ģ���ַ
* ���������
* ���������
* �� �� ֵ��uint16��ַ
* �������ڣ�2021��11��7��
* ע    �⣺��ȡʧ�ܣ�����0xffff,
*********************************************************************************************************/
uint16 getAddress(void)
{
  uint16 add = 0xFFFF;
  uint8 pAdd[3] = {0x00};
  
  if(s_address != 0xFFFF)
  {
    return s_address;
  }
  
  if(readRegisters(REG_ADDH, 2, pAdd))//��ȡ�ɹ�
  {
    add = pAdd[0];
    add <<= 8;
    add = pAdd[1];
    s_address = add;
  }
  else{
    debug("��ȡLoraģ��Ĵ�����ַʧ�ܣ���ǰ��ַ%d\r\n",add);
  }
  debug("s_address:%d\r\n",s_address);
  return add;
}

/*********************************************************************************************************
* �������ƣ�
* �������ܣ�
* ���������
* ���������
* �� �� ֵ��
* �������ڣ�2021��11��6��
* ע    �⣺
*********************************************************************************************************/

/*********************************************************************************************************
* �������ƣ�
* �������ܣ�
* ���������
* ���������
* �� �� ֵ��
* �������ڣ�2021��11��6��
* ע    �⣺
*********************************************************************************************************/

/*********************************************************************************************************
* �������ƣ�
* �������ܣ�
* ���������
* ���������
* �� �� ֵ��
* �������ڣ�2021��11��6��
* ע    �⣺
*********************************************************************************************************/

/*********************************************************************************************************
* �������ƣ�
* �������ܣ�
* ���������
* ���������
* �� �� ֵ��
* �������ڣ�2021��11��6��
* ע    �⣺
*********************************************************************************************************/
