/*********************************************************************************************************
* ģ�����ƣ�PackUnpack.h
* ժ    Ҫ��PackUnpackģ��
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
#ifndef _PACK_UNPACK_H_
#define _PACK_UNPACK_H_

/*********************************************************************************************************
*                                              ����ͷ�ļ�
*********************************************************************************************************/
#include "DataType.h"
#include "UART1.h"

/*********************************************************************************************************
*                                              �궨��
*********************************************************************************************************/
#define DATALEN 61                          //���ݰ����ݲ��ֵĳ���
#define PACKLEN sizeof(StructPackType)      //���ݰ��ܳ���
  
/*********************************************************************************************************
*                                              ö�ٽṹ�嶨��
*********************************************************************************************************/
//ö�ٶ���
typedef enum
{
  TYPE_DATA    = 0x01,  //���ݷ���
  TYPE_ROUTE   = 0x02,  //·�ɷ���
  TYPE_SYS     = 0x03,  //ϵͳ��Ϣ
}EnumPackType; 

typedef enum 
{
  CMD_SET_SMP_PRD = 0x01,//���ò�������
  
}EnumCmdType;

//���߰����ͽṹ��
typedef struct
{
  uint8  packType;    //����1
  uint8  arrData[DATALEN];     //������61
  short checkSum;    //У���2
}StructPackType;//uint8---4Byte;uint8*---4Byte;short---2Byte

//���ʹ��ڰ����ͽṹ��
typedef struct
{
  uint8  reverse;//�����������ֽڶ���
  uint8  addh;
  uint8  addl;
  uint8  channel;
  StructPackType spt2;
}SentStructPackType;

//�������ID��0x00��0xFF����Ϊ�Ƿ����ڲ�ͬ��ģ��ID����˲�ͬģ��ID�Ķ���ID�����ظ�
//ϵͳģ��Ķ���ID
typedef enum 
{
  DAT_RST         = 0x01,         //ϵͳ��λ��Ϣ
  DAT_SYS_STS     = 0x02,         //ϵͳ״̬
  DAT_SELF_CHECK  = 0x03,         //ϵͳ�Լ���
  DAT_CMD_ACK     = 0x04,         //����Ӧ��
  DAT_SYS_TEMP    = 0x05,         //��ǰϵͳ�¶�
  
  CMD_RST_ACK     = 0x80,         //ģ�鸴λ��ϢӦ��
  CMD_GET_POST_RSLT = 0x81,       //��ȡ�Լ���
  CMD_PAT_TYPE    = 0x90,         //������������  
}EnumSysSecondID;

//ECGģ��Ķ���ID
typedef enum 
{
  DAT_ECG_WAVE    = 0x02,         //�ĵ粨������
  DAT_ECG_LEAD    = 0x03,         //�ĵ絼����Ϣ
  DAT_ECG_HR      = 0x04,         //����
  DAT_ST          = 0x05,         //STֵ
  DAT_ST_PAT      = 0x06,         //STģ�岨��
  
  CMD_LEAD_SYS    = 0x80,         //3/5��������
  CMD_LEAD_TYPE   = 0x81,         //������ʽ����
  CMD_FILTER_MODE = 0x82,         //�ĵ��˲���ʽ����
  CMD_ECG_GAIN    = 0x83,         //ECG��������
  CMD_ECG_CAL     = 0x84,         //�ĵ�У׼
  CMD_ECG_TRA     = 0x85,         //��Ƶ�������ƿ���
  CMD_ECG_PACE    = 0x86,         //�𲫷�������
  CMD_ECG_ST_ISO  = 0x87,         //ST����ISO��ST��
  CMD_ECG_CHANNEL = 0x88,         //���ʼ���ͨ��
  CMD_ECG_LEADRN  = 0x89,         //�������¼���  
}EnumECGSecondID;

//Respģ��Ķ���ID
typedef enum 
{
  DAT_RESP_WAVE   = 0x02,         //������������
  DAT_RESP_RR     = 0x03,         //������
  DAT_RESP_APNEA  = 0x04,         //��Ϣ����
  DAT_RESP_CVA    = 0x05,         //����CVA������Ϣ
  
  CMD_RESP_GAIN   = 0x80,         //������������
  CMD_RESP_APNEA  = 0x81,         //������Ϣ����ʱ������  
}EnumRespSecondID;
  
//Tempģ��Ķ���ID
typedef enum 
{
  DAT_TEMP_DATA   = 0x02,         //��������
  
  CMD_TEMP        = 0x80,         //���²�������
}EnumTempSecondID;

//SPO2ģ��Ķ���ID
typedef enum 
{
  DAT_SPO2_WAVE   = 0x02,         //Ѫ������
  DAT_SPO2_DATA   = 0x03,         //Ѫ������
  
  CMD_SPO2        = 0x80,         //Ѫ����������
}EnumSPO2SecondID;

//NBPģ��Ķ���ID
typedef enum 
{
  DAT_NBP_CUFPRE  = 0x02,         //�޴�Ѫѹʵʱ����
  DAT_NBP_END     = 0x03,         //�޴�Ѫѹ��������
  DAT_NBP_RSLT1   = 0x04,         //�޴�Ѫѹ�������1 
  DAT_NBP_RSLT2   = 0x05,         //�޴�Ѫѹ�������2
  DAT_NBP_STS     = 0x06,         //�޴�Ѫѹ״̬
  
  CMD_NBP_START   = 0x80,         //NBP��������
  CMD_NBP_END     = 0x81,         //NBP��ֹ����
  CMD_NBP_PERIOD  = 0x82,         //NBP������������
  CMD_NBP_CALIB   = 0x83,         //NBPУ׼
  CMD_NBP_RST     = 0x84,         //NBPģ�鸴λ
  CMD_NBP_CHECK_LEAK  = 0x85,     //NBP©�����
  CMD_NBP_QUERY_STS   = 0x86,     //NBP��ѯ״̬
  CMD_NBP_FIRST_PRE   = 0x87,     //NBP�״γ���ѹ������
  CMD_NBP_CONT    = 0x88,         //��ʼ5���ӵ�STATѪѹ����
  CMD_NBP_RSLT    = 0x89,         //NBP��ѯ�ϴβ������  
}EnumNBPSecondID;

/*********************************************************************************************************
*                                              API��������
*********************************************************************************************************/
void  InitPackUnpack(void);           //��ʼ��PackUnpackģ��
uint8    PackData(StructPackType* pPT);  //�����ݽ��д����1-����ɹ���0-���ʧ��             
uint8    UnPackData(uint8 data);            //�����ݽ��н����1-����ɹ���0-���ʧ��
StructPackType  GetUnPackRslt(void);  //��ȡ��������ݰ�
#endif
