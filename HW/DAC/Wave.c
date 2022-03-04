/*********************************************************************************************************
* 模块名称：Wave.c
* 摘    要：Wave模块
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
#include "Wave.h"
#include "DataType.h"

/*********************************************************************************************************
*                                              宏定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              枚举结构体定义
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部变量
*********************************************************************************************************/
//100个点的正弦波，matlab代码如下：
//>> t=0:2*pi/100:2*pi;
//>> y=sin(t);
//>> y=y+1;
//>> y=y/2*4095;
static uint16 s_arrSineWave100Point[100] = {
2048,
2176,
2304,
2431,
2557,
2680,
2801,
2919,
3034,
3145,
3251,
3353,
3449,
3540,
3625,
3704,
3776,
3842,
3900,
3951,
3995,
4031,
4059,
4079,
4091,
4095,
4091,
4079,
4059,
4031,
3995,
3951,
3900,
3842,
3776,
3704,
3625,
3540,
3449,
3353,
3251,
3145,
3034,
2919,
2801,
2680,
2557,
2431,
2304,
2176,
2048,
1919,
1791,
1664,
1538,
1415,
1294,
1176,
1061,
950,
844,
742,
646,
555,
470,
391,
319,
253,
195,
144,
100,
64,
36,
16,
4,
0,
4,
16,
36,
64,
100,
144,
195,
253,
319,
391,
470,
555,
646,
742,
844,
950,
1061,
1176,
1294,
1415,
1538,
1664,
1791,
1919
};

//100个点的方波
static uint16 s_arrRectWave100Point[100] = {
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095,
4095
};

//100个点的三角波
static uint16 s_arrTriWave100Point[100] = {
0,
82,
164,
246,
328,
409,
491,
573,
655,
737,
819,
901,
983,
1065,
1147,
1229,
1310,
1392,
1474,
1556,
1638,
1720,
1802,
1884,
1966,
2048,
2129,
2211,
2293,
2375,
2457,
2539,
2621,
2703,
2785,
2867,
2948,
3030,
3112,
3194,
3276,
3358,
3440,
3522,
3604,
3686,
3767,
3849,
3931,
4013,
4095,
4013,
3931,
3849,
3767,
3686,
3604,
3522,
3440,
3358,
3276,
3194,
3112,
3030,
2948,
2867,
2785,
2703,
2621,
2539,
2457,
2375,
2293,
2211,
2129,
2048,
1966,
1884,
1802,
1720,
1638,
1556,
1474,
1392,
1310,
1229,
1147,
1065,
983,
901,
819,
737,
655,
573,
491,
409,
328,
246,
164,
82
};

/*********************************************************************************************************
*                                              内部函数声明
*********************************************************************************************************/

/*********************************************************************************************************
*                                              内部函数实现
*********************************************************************************************************/

/*********************************************************************************************************
*                                              API函数实现
*********************************************************************************************************/
/*********************************************************************************************************
* 函数名称：InitWave
* 函数功能：初始化Wave模块 
* 输入参数：void
* 输出参数：void
* 返 回 值：void
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
void  InitWave(void)
{
  
}

/*********************************************************************************************************
* 函数名称：GetSineWave100PointAddr
* 函数功能：获取100点正弦波数组的地址 
* 输入参数：void
* 输出参数：void
* 返 回 值：100点正弦波数组的地址
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
uint16* GetSineWave100PointAddr(void)
{
  return(s_arrSineWave100Point);
}

/*********************************************************************************************************
* 函数名称：GetRectWave100PointAddr
* 函数功能：获取100点方波数组的地址 
* 输入参数：void
* 输出参数：void
* 返 回 值：100点方波数组的地址
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
uint16* GetRectWave100PointAddr(void)
{
  return(s_arrRectWave100Point);
}

/*********************************************************************************************************
* 函数名称：GetTriWave100PointAddr
* 函数功能：获取100点三角波数组的地址 
* 输入参数：void
* 输出参数：void
* 返 回 值：100点三角波数组的地址
* 创建日期：2021年07月11日
* 注    意：
*********************************************************************************************************/
uint16* GetTriWave100PointAddr(void)
{
  return(s_arrTriWave100Point);
}
