/*********************************************************************************************************
* ģ�����ƣ�ProcHostCmd.c
* ժ    Ҫ��ProcHostCmdģ��
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
#include "ProcHostCmd.h"
#include "PackUnpack.h"
#include "DAC.h"
#include "Wave.h"
#include "SendDataToHost.h"
#include "ADC.h"
#include "Route.h"
#include "RADIO.h"
#include "cJSON.h"
#include "Main.h"
#include "UART2.h"
#include <string.h>

/*********************************************************************************************************
*                                              �궨��
*********************************************************************************************************/

/*********************************************************************************************************
*                                              ö�ٽṹ�嶨��
*********************************************************************************************************/

/*********************************************************************************************************
*                                              �ڲ�����
*********************************************************************************************************/
#if (defined SINK) && (SINK == TRUE)//��۽ڵ�
static uint8 IdBuff[10] = {0};//�洢�ϴ�����ID����ֹ�ظ�
#endif
/*********************************************************************************************************
*                                              �ڲ���������
*********************************************************************************************************/
static uint8  OnGenWave(uint8* pMsg);  //���ɲ��ε���Ӧ����

/*********************************************************************************************************
*                                              �ڲ�����ʵ��
*********************************************************************************************************/
/*********************************************************************************************************
* �������ƣ�OnGenWave
* �������ܣ����ɲ��ε���Ӧ����
* ���������pMsg
* ���������void
* �� �� ֵ��Ӧ����Ϣ
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
static uint8 OnGenWave(uint8* pMsg) 
{  
  StructDACWave wave;   //DAC��������
  
  if(pMsg[0] == 0x00)                      
  {
    wave.waveBufAddr  = (uint32)GetSineWave100PointAddr();  //��ȡ���Ҳ�����ĵ�ַ 
  }
  else if(pMsg[0] == 0x01)
  {
    wave.waveBufAddr  = (uint32)GetTriWave100PointAddr();   //��ȡ���ǲ�����ĵ�ַ 
  }
  else if(pMsg[0] == 0x02)
  {
    wave.waveBufAddr  = (uint32)GetRectWave100PointAddr();  //��ȡ��������ĵ�ַ 
  }
  
  wave.waveBufSize  = 100;  //����һ�����ڵ���Ϊ100

  SetDACWave(wave);         //����DAC��������
  
  return(CMD_ACK_OK);       //��������ɹ�
}

/*********************************************************************************************************
*                                              API����ʵ��
*********************************************************************************************************/
/*********************************************************************************************************
* �������ƣ�InitProcHostCmd
* �������ܣ���ʼ��ProcHostCmdģ�� 
* ���������void
* ���������void
* �� �� ֵ��void
* �������ڣ�2021��07��11��
* ע    �⣺
*********************************************************************************************************/
void  InitProcHostCmd(void)
{
  
}

/*********************************************************************************************************
* �������ƣ�ProcHostCmd
* �������ܣ��������������������� 
* ���������recData
* ���������void
* �� �� ֵ��void
* �������ڣ�2022��02��01��
* ע    �⣺
*********************************************************************************************************/
void ProcHostCmd(uint8 recData)
{ 
  StructPackType pack;    //���ṹ�����
  uint8 ack;                 //�洢Ӧ����Ϣ
  uint8 ackArr[61] = {0};
  
  while(UnPackData(recData))   //����ɹ�
  {
    pack = GetUnPackRslt();    //��ȡ������    
    
    switch(pack.packType)  //ģ��ID
    {
      case TYPE_DATA:        //���ݷ���
        ProcDatePack(pack.arrData);
        break;
      case TYPE_ROUTE:        //·�ɷ���  
        ack = UpdateRouTab2(pack.arrData);              //����·�ɱ�
        ackArr[0] = ack;
        sprintf((char*)ackArr+1, "OK=%d,TYPE_ROUTE ACK",ack);
        SendAckPack(pack.arrData[0], pack.arrData[1], 0x00, ackArr, sizeof(ackArr));
        break;
      default:          
        break;
    }
  }   
}

/*********************************************************************************************************
* �������ƣ�ProcDatePack
* �������ܣ������ӽ�㷢���������ݷ���
* ���������recData
* ���������void
* �� �� ֵ��void
* �������ڣ�2022��02��12��
* ע    �⣺
*********************************************************************************************************/
void ProcDatePack(uint8* pRecData)
{
#if (defined SINK) && (SINK == TRUE)//��۽ڵ�
  //getPackData   packAsJSON(char*, tempData, humidData, ...)    SendDateToE20
  //�ѽ��յ����ݰ���AlinkJSON��ʽ��ʽ�����ݣ�ͨ�����ڷ���eport-e20��eport-e20����mqttЭ����װ�󷢸�������
  /*
  AlinkJSON��ʽ���£�
  {
  "id": "123",
  "version": "1.0",
  "params": { 
    "F103ship_temperature": {
      "value": 9,
      "time": 1524448722000
    }
  },
  "method": "thing.event.property.post"
  } 
  */
  char* out;
  static uint32 MsgNo=1;
  char MsgNobuf[10];
  cJSON* item;
  cJSON* root = cJSON_CreateObject();
  cJSON* root2 = cJSON_CreateObject();
  cJSON* root3 = cJSON_CreateObject();
    
  sprintf(MsgNobuf, "%d", MsgNo);
  item = cJSON_CreateString(MsgNobuf);
    
  MsgNo++;
  
  cJSON_AddItemToObject(root, "id", item);
  cJSON_AddStringToObject(root, "version", "1.0");
    
  cJSON_AddNumberToObject(root3, "value", 9);
  //cJSON_AddNumberToObject(root3, "time", 1524448722000);//ʱ�������ѡ
    
  cJSON_AddItemToObject(root2, "F103ship_temperature", root3);
  
  cJSON_AddItemToObject(root, "params", root2);
  cJSON_AddStringToObject(root, "method", "thing.event.property.post");
  
  out = cJSON_Print(root);
  //printf("%s", out);
  WriteUART2(out, strlen(out));
  cJSON_Delete (root);
  free(out);
#else  //��ͨ�ڵ�
  SendDateToParent(pRecData, DATALEN);  //ת�����ݷ�������ڵ�
#endif
}

/*********************************************************************************************************
* �������ƣ�ProcCloudCmd
* �������ܣ������ƶ��·�������
* ���������void
* ���������void
* �� �� ֵ��void
* �������ڣ�2022��03��10��
* ע    �⣺
*********************************************************************************************************/
#if (defined SINK) && (SINK == TRUE)//��۽ڵ�
void ProcCloudCmd(void)
{
  char CmdBuf[200]={0};
  uint8 Cnt;
  cJSON *root, *method, *id, *params, *ADC_period_S, *version;
  char str[] = "{\"method\":\"thing.service.property.set\",\"id\":\"19244945\",\"params\":{\"ADC_period_S\":3},\"version\":\"1.0.0\"}";

  Cnt = ReadUART2(CmdBuf, 200);
  
  if(Cnt < 101)
  {
    return;
  }
  debug(CmdBuf);
  
/*���ճɹ������л�����JSON
{"method":"thing.service.property.set","id":"19244945","params":{"ADC_period_S":3},"version":"1.0.0"}
*/
  root = cJSON_Parse(CmdBuf);
  method = cJSON_GetObjectItem(root, "method");//û�д˶����򷵻ؿ�
  id = cJSON_GetObjectItem(root, "id");
  version = cJSON_GetObjectItem(root, "version");
  params = cJSON_GetObjectItem(root, "params");
  ADC_period_S = cJSON_GetObjectItem(params, "ADC_period_S");
  if(root == NULL || method == NULL || id == NULL  || version == NULL)
  {
   return;
  }
  if(strcmp(IdBuff, id->valuestring) == 0 || strcmp("1.0.0", version->valuestring) == 0)//ͬһ�������汾����
  {
    return;
  }
  else
  {
    memset(IdBuff, '\0', 10);
    strcpy(IdBuff, id->valuestring);
  }
  
  if(0 == strcmp("thing.service.property.set", method->valuestring))//��thing.service.property.set��ͬ
  {
    if (ADC_period_S)
    {
      ADC_period_S->valueint;
    }
  }


  cJSON_Delete(root);//����ͷ��ڴ�
}
#endif
