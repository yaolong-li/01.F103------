/*********************************************************************************************************
* 模块名称：Route.h
* 摘    要：路由协议的实现
* 当前版本：1.0.0
* 作    者：LYL(COPYRIGHT 2021 - 2021 LYL. All rights reserved.)
* 完成日期：2021年10月31日 
* 内    容：
* 注    意：                                                                  
**********************************************************************************************************
* 取代版本：
* 作    者：
* 完成日期：
* 修改内容：
* 修改文件：
*********************************************************************************************************/
#ifndef _ROUTE_H_
#define _ROUTE_H_

/*********************************************************************************************************
*                                              包含头文件
*********************************************************************************************************/
#include <stdio.h>
#include "DataType.h"

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/
#define ROUTE_TABLE_SIZE 16           //设置路由表的行数

/*********************************************************************************************************
*                                              枚举结构体定义
*********************************************************************************************************/
//路由表每行格式
typedef struct{
  uint8 addh;      //邻居节点地址高位(下一跳)
  uint8 addl;      //邻居节点地址低位(下一跳)
  uint8 no;        //接收到该条信息的序列号9
  uint8 distance;  //到汇聚节点的跳数，即接收到的跳数加1(跳数)
  uint8 qualify;   //链路质量

  uint8 SedCnt;    //发送计数
  uint8 RecCnt;    //接收计数
  int8 liveliness;//是否可用，每60S减30，收到消息置60.假设30S广播一次
}StructRoute;

/*********************************************************************************************************
*                                              API函数声明
*********************************************************************************************************/
void InitRoute(void);         //初始化线性路由表
uint8 UpdateRouTab(uint8 *pMsg);    //更新路由表   ,地址有相同，更新，无则插入
uint8 UpdateRouTab2(uint8 *pMsg);
uint16 GetParentAddr(void);//查找父结点地址
void RouteTimerTasks(void);   //路由定时任务(广播路由分组，更新生命周期)

#endif
