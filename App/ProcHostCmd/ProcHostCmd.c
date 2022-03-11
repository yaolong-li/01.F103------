/*********************************************************************************************************
* 模块名称：ProcHostCmd.c
* 摘    要：ProcHostCmd模块
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
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              枚举结构体定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
#if (defined SINK) && (SINK == TRUE)//汇聚节点
static uint8 IdBuff[10] = {0};//存储上次命令ID，防止重复
#endif
/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static uint8  OnGenWave(uint8* pMsg);  //生成波形的响应函数

/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：OnGenWave
* 函数功能：生成波形的响应函数
* 输入参数：pMsg
* 输出参数：void
* 返 回 值：应答消息
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
static uint8 OnGenWave(uint8* pMsg) 
{  
  StructDACWave wave;   //DAC波形属性
  
  if(pMsg[0] == 0x00)                      
  {
    wave.waveBufAddr  = (uint32)GetSineWave100PointAddr();  //获取正弦波数组的地址 
  }
  else if(pMsg[0] == 0x01)
  {
    wave.waveBufAddr  = (uint32)GetTriWave100PointAddr();   //获取三角波数组的地址 
  }
  else if(pMsg[0] == 0x02)
  {
    wave.waveBufAddr  = (uint32)GetRectWave100PointAddr();  //获取方波数组的地址 
  }
  
  wave.waveBufSize  = 100;  //波形一个周期点数为100

  SetDACWave(wave);         //设置DAC波形属性
  
  return(CMD_ACK_OK);       //返回命令成功
}

/*********************************************************************************************************
*                                              API函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：InitProcHostCmd
* 函数功能：初始化ProcHostCmd模块 
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
void  InitProcHostCmd(void)
{
  
}

/*********************************************************************************************************
* 函数名称：ProcHostCmd
* 函数功能：处理主机发送来的命令 
* 输入参数：recData
* 输出参数：void
* 返 回 值：void
* 创建日期：2022年02月01日
* 注    意：
*********************************************************************************************************/
void ProcHostCmd(uint8 recData)
{ 
  StructPackType pack;    //包结构体变量
  uint8 ack;                 //存储应答消息
  uint8 ackArr[61] = {0};
  
  while(UnPackData(recData))   //解包成功
  {
    pack = GetUnPackRslt();    //获取解包结果    
    
    switch(pack.packType)  //模块ID
    {
      case TYPE_DATA:        //数据分组
        ProcDatePack(pack.arrData);
        break;
      case TYPE_ROUTE:        //路由分组  
        ack = UpdateRouTab2(pack.arrData);              //更新路由表
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
* 函数名称：ProcDatePack
* 函数功能：处理子结点发送来的数据分组
* 输入参数：recData
* 输出参数：void
* 返 回 值：void
* 创建日期：2022年02月12日
* 注    意：
*********************************************************************************************************/
void ProcDatePack(uint8* pRecData)
{
#if (defined SINK) && (SINK == TRUE)//汇聚节点
  //getPackData   packAsJSON(char*, tempData, humidData, ...)    SendDateToE20
  //把接收的数据按照AlinkJSON格式格式化数据，通过串口发给eport-e20，eport-e20按照mqtt协议组装后发给阿里云
  /*
  AlinkJSON格式如下：
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
  //cJSON_AddNumberToObject(root3, "time", 1524448722000);//时间戳，可选
    
  cJSON_AddItemToObject(root2, "F103ship_temperature", root3);
  
  cJSON_AddItemToObject(root, "params", root2);
  cJSON_AddStringToObject(root, "method", "thing.event.property.post");
  
  out = cJSON_Print(root);
  //printf("%s", out);
  WriteUART2(out, strlen(out));
  cJSON_Delete (root);
  free(out);
#else  //普通节点
  SendDateToParent(pRecData, DATALEN);  //转发数据分组给父节点
#endif
}

/*********************************************************************************************************
* 函数名称：ProcCloudCmd
* 函数功能：处理云端下发的命令
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2022年03月10日
* 注    意：
*********************************************************************************************************/
#if (defined SINK) && (SINK == TRUE)//汇聚节点
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
  
/*接收成功则反序列化以下JSON
{"method":"thing.service.property.set","id":"19244945","params":{"ADC_period_S":3},"version":"1.0.0"}
*/
  root = cJSON_Parse(CmdBuf);
  method = cJSON_GetObjectItem(root, "method");//没有此对象则返回空
  id = cJSON_GetObjectItem(root, "id");
  version = cJSON_GetObjectItem(root, "version");
  params = cJSON_GetObjectItem(root, "params");
  ADC_period_S = cJSON_GetObjectItem(params, "ADC_period_S");
  if(root == NULL || method == NULL || id == NULL  || version == NULL)
  {
   return;
  }
  if(strcmp(IdBuff, id->valuestring) == 0 || strcmp("1.0.0", version->valuestring) == 0)//同一条命令或版本不对
  {
    return;
  }
  else
  {
    memset(IdBuff, '\0', 10);
    strcpy(IdBuff, id->valuestring);
  }
  
  if(0 == strcmp("thing.service.property.set", method->valuestring))//与thing.service.property.set相同
  {
    if (ADC_period_S)
    {
      ADC_period_S->valueint;
    }
  }


  cJSON_Delete(root);//最后释放内存
}
#endif
