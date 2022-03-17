/*********************************************************************************************************
* 模块名称：Route.c
* 摘    要：
* 当前版本：1.0.0
* 作    者：LYL(COPYRIGHT 2021 - 2021 LYL. All rights reserved.)
* 完成日期：2021年10月14日 
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
#include "Route.h"
#include "SendDataToHost.h"
#include "RADIO.h"
#include "PackUnpack.h"
#include "string.h"

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              枚举结构体定义
*********************************************************************************************************/
//路由表
typedef struct
{
  uint8 len;               //路由表最大行数
  uint8 elemNum;           //当前行数
  StructRoute *pRouBuf; //路由表的缓冲区
}StructRouteTable;

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
volatile static StructRouteTable s_structRouteTable;//路由线性表
static StructRoute s_structRouteBuf[ROUTE_TABLE_SIZE];//路由表信息存放数组
volatile static uint8 IndexOfParent;  //父节点表中下标

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/
static void  InitRouTab(void);         //初始化路由表
static uint8 getRouLen(void);          //取得路由表当前长度
static int8  find(uint8 addh, uint8 addl);//找到路由表中有无该路由项
static uint8 InsertRou(StructRoute *pRou);//插入新路由项
static void  UpdateEst(void);          //更新链路质量评估
static uint8 UpdateTable(uint8 *pMsg, uint8 position);//更新已有表项
static void  evaluateCost(void);       //通信代价评估
static void  SendRouteTask(void);      // 广播路由信息给邻居
static void  RouteTimerTask(void);     //定时20S，路由表更新，父结点选择，发送路由更新通告
static void  UpdateParent(void);       //更新父节点
static void  DecreaseLiveTime(void);   //老化路由表
static void  DeleteTable(void);        //删除超时过期路由项

/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：InitRouTab
* 函数功能：初始化路由表
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2021年11月02日
* 注    意：
*********************************************************************************************************/
void InitRouTab(void)
{
  int16 i = 0;
  StructRoute structRou;//初始化默认参数全为0
  
  structRou.addh = 0;
  structRou.addl = 0;
  structRou.distance = 0;
  structRou.liveliness = 0;
  structRou.no = 0;
  structRou.qualify = 0;
  structRou.RecCnt = 0;
  structRou.SedCnt = 0;
  
  s_structRouteTable.len = ROUTE_TABLE_SIZE;//表长
  s_structRouteTable.elemNum = 0;           //当前行数
  
  for(i = 0; i < ROUTE_TABLE_SIZE; i++)
  {
    s_structRouteBuf[i]  = structRou;       //清空缓存区  
  }
  
  s_structRouteTable.pRouBuf = s_structRouteBuf;
}

/*********************************************************************************************************
* 函数名称：ConfigLRMode
* 函数功能：取得路由表当前长度
* 输入参数：void
* 输出参数：无
* 返 回 值：void
* 创建日期：2021年11月02日
* 注    意：
*********************************************************************************************************/
uint8 getRouLen(void)
{
  return s_structRouteTable.elemNum;
}

/*********************************************************************************************************
* 函数名称：find
* 函数功能：找到路由表中有无该路由项
* 输入参数：地址高位addh，地址低位addl
* 输出参数：无
* 返 回 值：该地址所在数组下标,没找到返回-1
* 创建日期：2021年10月15日
* 注    意：
*********************************************************************************************************/
int8 find(uint8 addh, uint8 addl)
{
  uint8 len = s_structRouteTable.elemNum;
  StructRoute *strupRou = s_structRouteTable.pRouBuf;
  int i;
  
  for(i = 0; i<len; i++)
  {
    if(strupRou[i].addh == addh && strupRou[i].addh == addl)
    {
      return i;
    }
  }
  
  return -1;
}
/*********************************************************************************************************
* 函数名称：InsertRou
* 函数功能：插入新路由项
* 输入参数：pRou指向待插入的路由项
* 输出参数：无
* 返 回 值：ok 0---失败
* 创建日期：2021年11月2日
* 注    意：
*********************************************************************************************************/
uint8 InsertRou(StructRoute *pRou)
{
  uint8 ok = 1;
  uint8 position = s_structRouteTable.elemNum;
  
  if(position >= s_structRouteTable.len - 1)//表中无空间，插入失败
  {
    return !ok;
  }
  s_structRouteTable.elemNum++;
  memcpy(&s_structRouteBuf[position],pRou,sizeof(StructRoute));//复制路由数据
  return ok;
}

/*********************************************************************************************************
* 函数名称：UpdateEst
* 函数功能：更新链路质量评估
* 输入参数：void
* 输出参数：无
* 返 回 值：void
* 创建日期：2021年11月2日
* 注    意：也许可以用RSSI指示链路质量？
*********************************************************************************************************/
void UpdateEst(void)
{
  
}

/*********************************************************************************************************
* 函数名称：UpdateTable
* 函数功能：更新已有表项
* 输入参数：pMsg广播消息，position路由表对应项地址下标
* 输出参数：无
* 返 回 值：void
* 创建日期：2021年11月2日
* 注    意：利用广播消息更新路由表 |addh |addl |dis |
*********************************************************************************************************/
uint8 UpdateTable(uint8 *pMsg, uint8 position)
{
  StructRoute *strupRou = s_structRouteTable.pRouBuf;
  StructRoute temp;
  uint8 ok  = 1;
  uint8 dis = pMsg[2];
  temp = strupRou[position];
  
  if(dis <= 0xff )//最大255跳
  {
    temp.distance   = (dis == 0xff)? 0xff:dis+1;//记录跳数
    temp.liveliness = 100;
  }
  
  strupRou[position] = temp;//存入路由表
  return ok;
}

/*********************************************************************************************************
* 函数名称：RouteTimerTask
* 函数功能：定时20S，路由表更新，父结点选择，发送路由更新通告
* 输入参数：void
* 输出参数：无
* 返 回 值：void
* 创建日期：2021年11月2日
* 注    意：
*********************************************************************************************************/
void RouteTimerTask()
{
  DecreaseLiveTime(); //老化路由表
  DeleteTable();      //删除超时过期路由项
  UpdateParent();     //父结点更新
  SendRouteTask();    //广播路由信息给邻居
}

/*********************************************************************************************************
* 函数名称：
* 函数功能：更新父节点
* 输入参数：void
* 输出参数：无
* 返 回 值：void
* 创建日期：2021年11月2日
* 注    意：
*********************************************************************************************************/
void  UpdateParent()
{
  uint8 len = s_structRouteTable.elemNum;
  StructRoute *strupRou = s_structRouteTable.pRouBuf;
  uint8 i;
  uint8 MinHop = 0xff;
  uint8 Index = 0;//Index=0是默认路由，跳数0xff
  
  for(i = 0; i<len; i++)
  {
    uint8 dis = strupRou[i].distance;
    if(dis < MinHop)
    {
      MinHop = dis;
      Index = i;
    }
  }
  
  IndexOfParent = Index;
}

/*********************************************************************************************************
* 函数名称：
* 函数功能：老化路由表
* 输入参数：void
* 输出参数：无
* 返 回 值：void
* 创建日期：2021年11月2日
* 注    意：
*********************************************************************************************************/
void DecreaseLiveTime()
{
  uint8 len = s_structRouteTable.elemNum;                //当前表长
  StructRoute *strupRou = s_structRouteTable.pRouBuf; //指向表头的指针
  int8 i;//计数
  
  for(i = 1; i<len; i++)//i=0是默认路由
  {
    strupRou[i].liveliness -= 1;
  }
}

/*********************************************************************************************************
* 函数名称：
* 函数功能：删除超时过期路由项
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月2日
* 注    意：
*********************************************************************************************************/
void DeleteTable()        
{
  uint8 len = s_structRouteTable.elemNum;                //当前表长
  StructRoute *strupRou = s_structRouteTable.pRouBuf; //指向表头的指针
  int8 i,j;//计数
  
  for(i = 0; i<len; i++)
  {
    if(strupRou[i].liveliness <= 0)
    {
      for(j=i; j<len; j++)//后面的路由项覆盖前面的
      {
        if(j+1+1<=s_structRouteTable.len)
        {
          strupRou[j] = strupRou[j + 1];
        }
        else//最后一项
        {
          memset(&strupRou[j],0,sizeof(StructRoute));
        }
      }
      len--;
      s_structRouteTable.elemNum--;
    }
  }
}

/*********************************************************************************************************
* 函数名称：SendRouteTask
* 函数功能：广播路由信息给邻居
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月6日
* 注    意：广播发送路由消息|SenCnt |addh |addl |dis |no |recCnt |
*********************************************************************************************************/
void SendRouteTask(void)    
{
  uint8 arrRouteData[DATALEN] = {0}; //初始化数据分组的数组
  
  uint16 add = getAddress();//取得模块地址,失败返回0xffff,占用串口！！！
  
  arrRouteData[0] = add>>8; //结点自己的地址高位
  arrRouteData[1] = add;    //结点自己的地址低位
  arrRouteData[2] = s_structRouteBuf[IndexOfParent].distance;//跳数
  
  SendRouteToNeighbor(arrRouteData, DATALEN);
}

/*********************************************************************************************************
*                                              API函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：InitRoute
* 函数功能：初始化路由模块
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2021年11月3日
* 注    意：
*********************************************************************************************************/
void InitRoute(void)
{
  uint16 add;
  StructRoute structRou;
  
  InitRouTab();//初始化线性路由表
                
  add = 0xffff;//默认目的地址为广播地址
  structRou.addh = add >> 8;
  structRou.addl = add;
#if (defined SINK && SINK)//汇聚节点
  structRou.distance = 0x00;  //直接可达
#else  //普通节点
  structRou.distance = 0xff;  //初始不可达
#endif
  structRou.liveliness = 120;
  
  InsertRou(&structRou);//插入自己到汇聚节点的路由项
}

/*********************************************************************************************************
* 函数名称：UpdateRouTab
* 函数功能：更新路由表   ,地址有相同，更新，无则插入
* 输入参数：pMsg,收到的数据包中的消息
* 输出参数：void
* 返 回 值：1---成功
* 创建日期：2021年11月3日
* 注    意：pMsg---->|type |addh |addl |dis |----------- |crc |
*********************************************************************************************************/
uint8 UpdateRouTab(uint8 *pMsg)
{
  uint8 ok = 0;
  int8 index = find(pMsg[1], pMsg[2]);//查找路由表中有无该地址
  StructRoute StRou;
  
  if(index > -1)//有
  {
    ok = UpdateTable(pMsg, index);//更新路由表
  }
  else          //无则插入新路由项
  {
    StRou.addh = pMsg[1];         //addh
    StRou.addl = pMsg[2];         //addl
    StRou.distance = pMsg[3] + 1; //dis
    StRou.liveliness = 100;       //刷新存活时间
    
    ok = InsertRou(&StRou);            //插入新的表项
  }
  return ok;
}

/*********************************************************************************************************
* 函数名称：UpdateRouTab2
* 函数功能：更新路由表   ,地址有相同，更新，无则插入
* 输入参数：pMsg,收到的数据包中的消息
* 输出参数：void
* 返 回 值：1---成功
* 创建日期：2022年2月2日
* 注    意：pMsg---->|SrcAddh |SrcAddL |MinDis |
*********************************************************************************************************/
uint8 UpdateRouTab2(uint8 *pMsg)
{
  uint8 ok = 0;
  int8 index = find(pMsg[0], pMsg[1]);//查找路由表中有无该地址
  StructRoute StRou;
  
  if(index > -1)//有
  {
    ok = UpdateTable(pMsg, index);//更新路由表
  }
  else          //无则插入新路由项
  {
    StRou.addh       = pMsg[0];         //addh
    StRou.addl       = pMsg[1];         //addl
    StRou.distance   = pMsg[2] < 0xFF? pMsg[2] + 1 : 0xFF; //dis
    StRou.liveliness = 100;       //刷新存活时间
    
    ok = InsertRou(&StRou);            //插入新的表项
  }
  return ok;
}

/*********************************************************************************************************
* 函数名称：GetParentAddr
* 函数功能：查找父结点地址,选择distance最小的作为父结点
* 输入参数：void
* 输出参数：无
* 返 回 值：父结点地址
* 创建日期：2021年11月3日
* 注    意：
*********************************************************************************************************/
uint16 GetParentAddr(void)
{
  uint8 Index = IndexOfParent;
  uint16 add;
  
  add = s_structRouteBuf[IndexOfParent].addh;
  add <<= 8;
  add = add | s_structRouteBuf[IndexOfParent].addl;

  return add;
}

/*********************************************************************************************************
* 函数名称：
* 函数功能：路由定时任务
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月2日
* 注    意：
*********************************************************************************************************/
void RouteTimerTasks(void)
{
  RouteTimerTask();
}

/*********************************************************************************************************
* 函数名称：
* 函数功能：
* 输入参数：
* 输出参数：
* 返 回 值：
* 创建日期：2021年11月2日
* 注    意：
*********************************************************************************************************/
