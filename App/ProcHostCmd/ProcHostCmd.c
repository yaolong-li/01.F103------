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
#include "UART1.h"

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
static uint8 lastRecCmdID = 0;

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static uint8  OnGenWave(uint8* pMsg);  //生成波形的响应函数
static uint8  SetSamplePeriod(uint8 CmdVlaue);  //设置采样周期的响应函数

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
* 函数名称：SetSamplePeriod
* 函数功能：设置采样周期的响应函数
* 输入参数：CmdVlaue
* 输出参数：void
* 返 回 值：应答消息
* 创建日期：2022年3月18日21:56:34
* 注    意：
*********************************************************************************************************/
static uint8  SetSamplePeriod(uint8 CmdVlaue)
{
  SetSmpPrd(CmdVlaue);
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
        debug("DATA\r\n");
        ProcDatePack(pack.arrData);
        break;
      case TYPE_ROUTE:        //路由分组  
        debug("\r\nROUTE\r\n");
        ack = UpdateRouTab2(pack.arrData);              //更新路由表
//        ackArr[0] = ack;
//        sprintf((char*)ackArr+1, "OK=%d,TYPE_ROUTE ACK",ack);
//        SendAckPack(pack.arrData[0], pack.arrData[1], 0x00, ackArr, sizeof(ackArr));
        break;
      case TYPE_SYS:        //命令分组 
        debug("\r\nSYS\r\n");
        ProcCmdPack(pack.arrData);
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
  cJSON* id;
  cJSON* root = cJSON_CreateObject();
  cJSON* root2 = cJSON_CreateObject();
  cJSON* root3 = cJSON_CreateObject();
  cJSON* root3_1 = cJSON_CreateObject();
    
  sprintf(MsgNobuf, "%d", MsgNo);
  id = cJSON_CreateString(MsgNobuf);
    
  MsgNo++;
  
  cJSON_AddItemToObject(root, "id", id);
  cJSON_AddStringToObject(root, "version", "1.0");
    
  cJSON_AddNumberToObject(root3, "value", pRecData[0]);
  cJSON_AddNumberToObject(root3_1, "value", pRecData[1]);
  //cJSON_AddNumberToObject(root3, "time", 1524448722000);//时间戳，可选
    
  cJSON_AddItemToObject(root2, "F103ship_temperature", root3);
  cJSON_AddItemToObject(root2, "Smp_Period", root3_1);
  
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
  char CmdBuf[256]={0};
  uint8 Cnt;
  cJSON *root, *method, *id, *params, *ADC_period_S, *version, *Period_ms, *CmdObj;
  //char str[] = "{\"method\":\"thing.service.property.set\",\"id\":\"19244945\",\"params\":{\"ADC_period_S\":3},\"version\":\"1.0.0\"}";
  char pdebug[100] = {0};

  Cnt = ReadUART2((uint8*)CmdBuf, 0xff);
  
  if(Cnt < 101)
  {
    return;
  }
  debug((uint8*)CmdBuf);
  
/*接收成功则反序列化以下JSON
{"method":"thing.service.property.set","id":"19244945","params":{"ADC_period_S":3},"version":"1.0.0"}
*/
  root = cJSON_Parse(CmdBuf);
  //必须有的参数
  method = cJSON_GetObjectItem(root, "method");//没有此对象则返回空
  id = cJSON_GetObjectItem(root, "id");
  version = cJSON_GetObjectItem(root, "version");
  params = cJSON_GetObjectItem(root, "params");
  //可能有的参数
  ADC_period_S = cJSON_GetObjectItem(params, "ADC_period_S");
  Period_ms = cJSON_GetObjectItem(params, "Period_ms");
  CmdObj = cJSON_GetObjectItem(params, "CmdObj");
  if(root == NULL || method == NULL || id == NULL  || version == NULL)
  {
    cJSON_Delete(root);
    return;
  }
  if(strcmp((char*)IdBuff, id->valuestring) == 0 || strcmp("1.0.0", version->valuestring) != 0)//同一条命令或版本不同
  {
//    cJSON_Delete(root);
//    return;
  }
  else
  {
    memset(IdBuff, '\0', 10);
    strcpy((char*)IdBuff, id->valuestring);
  }
  
  if(0 == strcmp("thing.service.property.set", method->valuestring))//与thing.service.property.set相同,即设置属性
  {
    if (ADC_period_S)
    {
      sprintf(pdebug, "%d", ADC_period_S->valueint);
      debug((uint8*)pdebug);

    }
  }
  if(0 == strcmp("thing.service.Smp_Period", method->valuestring))//与thing.service.Smp_Period相同,即设置采样周期
  {
    if (Period_ms && CmdObj)
    {
      sprintf(pdebug, "\r\nPeriod_ms:%d, CmdObj:%d\r\n", Period_ms->valueint, CmdObj->valueint);
      debug((uint8*)pdebug);
      SendCmdPack((uint8)*(id->valuestring), CMD_SET_SMP_PRD, Period_ms->valueint, CmdObj->valueint, 0);
    }
  }

  cJSON_Delete(root);//最后释放内存
}
#endif

/*********************************************************************************************************
* 函数名称：ProcCmdPack
* 函数功能：处理命令分组
* 输入参数：recData
* 输出参数：void
* 返 回 值：void
* 创建日期：2022年3月18日21:19:52
* 注    意：接收处理SendCmdPack()函数的消息
*********************************************************************************************************/
void ProcCmdPack(uint8* pRecData)
{  
#if (defined SINK) && (SINK == TRUE)//汇聚节点
#else
  if(lastRecCmdID == pRecData[0])//已经接收过这条命令
  {
//    return;
  }
  lastRecCmdID = pRecData[0];
  if(getAddress() == ((uint16)pRecData[3]<<8 | (uint16)pRecData[4]))//命令对象是该节点
  {
    switch (pRecData[1])
    {
    	case CMD_SET_SMP_PRD: 
        SetSamplePeriod(pRecData[2]);
    		break;
    	default:
    		break;
    }
  }
  else
  {
    SendCmdPack(pRecData[0], pRecData[1], pRecData[2], (uint16)pRecData[3]<<8 | (uint16)pRecData[4], pRecData[5]);//转发该命令
  }
  
#endif
}
