/************************************************************************************
* @file     : numfilter.h
* @brief    : 
* @details  : 
* @author   : 
* @date     : 
* @version  : 
* @copyright: Copyright (c) 2050
***********************************************************************************/
/**
 * @file numfilter.h
 * @brief 数值处理与通用滤波算法接口 
 *
 * 该文件集中声明比例计算、插值、二维表查找、平均滤波、方差统计、
 * 气体工况换算和差分计算等常用算法接口 
 */
#ifndef _UNIT_ALGO_NUM_STATISTIC_H_
#define _UNIT_ALGO_NUM_STATISTIC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/** Return the greater of two values. */
#define NUMFILTER_MAX(a, b)    (((a) > (b)) ? (a) : (b))

/** Return the lesser of two values. */
#define NUMFILTER_MIN(a, b)    (((a) < (b)) ? (a) : (b))

/** 比例换算对象，满足 y = kx + b  */
typedef struct _PROPORT_OBJ_
{
    float m_y;
    float m_k;
    float m_b;
}ProportObj ;

/** 线性插值对象  */
typedef struct _LAGRANGE_OBJ_
{
    float m_Output ;
}LagrangeObj;

/** 一阶传递函数离散对象  */
typedef struct _FIRST_ORD_TRANSF_OBJ_
{
    float m_xMinusOne ;
    float m_yMinusOne ;
    float m_a1 ;
    float m_b1 ;
    float m_b2 ;
}FirstOrdTransfObj;

/** 二维表单个采样点  */
typedef struct _TAB2D_POINT_OBJ_
{
    float m_x ;
    float m_y ;
    float m_z ;
}Tab2DPointObj;


/** 二维表对象  */
typedef struct _TAB2D_OBJ_
{
    const Tab2DPointObj *m_Points;
    int16_t m_size;
}Tab2DObj;

/** 移动平均滤波对象  */
typedef struct _MOV_AVG_FILTER_OBJ_
{
    uint16_t m_BuffLen;
    uint16_t m_Index;
    uint8_t  m_BuffFull ;
    
    float    *m_Buffer;
    float    m_Sum;
}MovAvgFilterObj ;

/** 去极值平均滤波对象  */
typedef struct _AVG_FILTER_OBJ_
{
    uint16_t m_BuffLen ;
    uint16_t m_Index ;
    uint8_t  m_BufFull;
    
    float *m_Buff ;
    float m_Sum ;
    float m_Max ;
    float m_Min ;
}AvgFilterObj ;

/** 锁相平均滤波对象  */
typedef struct _PHASE_LOCK_FILTER_OBJ
{
    uint32_t m_BackwardTime;
    uint32_t m_Period;
    uint32_t m_Pointer;
    uint32_t m_BuffLen;
    uint8_t  m_FirstInFlag;
    
    float *m_Buffer;
    float m_DownwardVal;
    float m_FiltOut;
}PhaseLockFilterObj;

/** 滑动方差计算对象  */
typedef struct _VARIANCE_OBJ_
{
    uint8_t  m_FullFlag ;
    uint16_t m_Len ;
    uint16_t m_Ptr ;
    float    m_SumXi;
    float    m_SumXiSquare ;
    float    m_VarOutput ;
}VarianceObj ;


/** 滑动标准差计算对象  */
typedef struct _STAND_DEVIAT_OBJ_
{
    uint8_t  m_FullFlag ;
    uint16_t m_Len ;
    uint16_t m_Ptr ;
    float    m_SumXi;
    float    m_SumXiSquare ;
    float    m_StdOutput ;
}StandDeviatObj ;
    

/** 有限长度高效均值对象  */
typedef struct _EFFIC_MEAN_OBJ_
{
    uint16_t m_Cnt;
    uint16_t m_Len;
    float    m_Mean;
}EfficMeanObj;

/** 有限长度高效方差对象  */
typedef struct _EFFIC_VARIANCE_OBJ_
{
    uint16_t m_Cnt ;
    uint16_t m_Len ;
    float    m_LastMean ;
    float    m_LastSum ;
    float    m_LastVarOut ;
}EfficVarianceObj ;

/** 有限长度高效标准差对象  */
typedef struct _EFFIC_STDDEV_OBJ_
{
    uint16_t m_Cnt ;
    uint16_t m_Len ;
    float    m_LastMean ;
    float    m_LastSum ;
    float    m_StdOut ;
}EfficStdDevObj ;

/** 加速度差分滤波对象  */
typedef struct _ACC_FILTER_OBJ_
{
  uint32_t m_BuffLen ;
  uint32_t m_Index ;
  uint32_t m_WdLen ;

  float *m_Buffer ;
  float m_NewAdd ;
  float m_OldAdd ;
}AccFilterObj ;

/** 固定间隔差分计算对象  */
typedef struct _DIFF_CALC_OBJ_
{
  uint16_t m_BuffLen ;
  uint16_t m_Index ;
  uint16_t m_DiffInterval ;
  uint8_t  m_FirstInFlag ;
  float *m_Buffer ;
  float m_DiffOut ;
}DiffCalcObj ;

/** 气体工况对象  */
typedef struct _GAS_CONDITION_OBJ_
{
  float m_Press_mmHg ;
  float m_Temp_K ;
  float m_Vapor_mmHg  ;
}GasConditionObj ;

/** 25°C、760mmHg、干燥工况  */
extern GasConditionObj ATPD;

/** 25°C、760mmHg、饱和工况  */
extern GasConditionObj ATPS;

/** 0°C、760mmHg、干燥工况  */
extern GasConditionObj STPD;

/** 37°C、760mmHg、饱和工况  */
extern GasConditionObj BTPS;

/* 比例与插值算法接口  */

/** 初始化比例换算对象  */
void UnitAlgoNumStatProportInit(ProportObj *pxHand, float k, float b);

/** 计算比例换算结果  */
float UnitAlgoNumStatProportCalc(ProportObj *pxHand, float x);

/** 初始化线性插值对象  */
void UnitAlgoLagrangeInit(LagrangeObj *pxHand);

/** 在升序数组中查找插值左边界索引  */
void UnitAlgoIncreaseBinarySearchLut(float *pArr, uint32_t ArrLen, float Data, uint32_t *pIndexLeft);

/** 在降序数组中查找插值左边界索引  */
void UnitAlgoDecreaseBinarySearchLut(float *pArr, uint32_t ArrLen, float Data, uint32_t *pIndexLeft);

/** 在升序表上执行线性插值  */
float UnitAlgoIncreaseLagrangeCalc(LagrangeObj *pxHand, float *xArr, 
        float *yArr, float Data, int16_t Size);

/** 在降序表上执行线性插值  */
float UnitAlgoDecreaseLagrangeCalc(LagrangeObj *pxHand, float *xArr, float *yArr, float Data, int16_t Size);

/* 二维表插值接口  */

/** 查找二维表中不大于目标值的坐标索引  */
int16_t UnitAlgoFind2DTabLowerIndex(const Tab2DObj *Tab, float Data, int16_t xyFlag);

/** 获取包围目标点的四个二维表节点  */
int16_t UnitAlgoFind2DTabSurroundPoints(const Tab2DObj *Tab, float x, float y, 
            Tab2DPointObj *Q00, Tab2DPointObj *Q10, Tab2DPointObj *Q01, Tab2DPointObj *Q11);

/** 按二维表节点执行双线性插值  */
float UnitAlgoBilinearInterpolatePoints(const Tab2DObj *Tab, float x, float y);

/* 一阶传递函数接口  */

/** 初始化一阶离散传递函数对象  */
void UnitAlgoFirstOrdTransfInit(FirstOrdTransfObj *pxHand, float Period, float a, float b);

/** 设置一阶传递函数初始状态  */
void UnitAlgoFirstOrdTransfStateSet(FirstOrdTransfObj *pxHand, float Data);

/** 更新一阶传递函数输出  */
float UnitAlgoFirstOrdTransfUpdata(FirstOrdTransfObj *pxHand, float Data);

/* 平均滤波接口  */

/** 初始化移动平均滤波器  */
void UnitAlgoMovAvgFilterInit(MovAvgFilterObj *pxHand, float *DataBuffer, uint16_t DataBuffLen);

/** 复位移动平均滤波器  */
void UnitAlgoMovAvgFilterReset(MovAvgFilterObj *pxHand);

/** 更新移动平均滤波器输出  */
float UnitAlgoMovAvgFilterUpdata(MovAvgFilterObj *pxHand, float NewData);

/** 初始化去极值平均滤波器  */
void UnitAlgoAvgFilterInit(AvgFilterObj *pxHand, float *DataBuffer, uint16_t DataBuffLen);

/** 复位去极值平均滤波器  */
void UnitAlgoAvgFilterReset(AvgFilterObj *pxHand);

/** 更新去极值平均滤波器输出  */
float UnitAlgoAvgFilterUpdata(AvgFilterObj *pxHand, float NewData);

/** 初始化锁相平均滤波器  */
void UnitAlgoPhaseLockFilterInit(PhaseLockFilterObj *pxHand, float *DataBuffer, uint16_t DataBuffLen, 
    float Value, uint32_t Time, uint32_t Period);

/** 复位锁相平均滤波器  */
void UnitAlgoPhaseLockFilterReset(PhaseLockFilterObj *pxHand);

/** 更新锁相平均滤波器输出  */
float UnitAlgoPhaseLockFilterUpdata(PhaseLockFilterObj *pxHand, float NewData);

/* 统计与差分算法接口  */

/** 初始化加速度差分滤波器  */
void UnitAlgoAccFilterInit(AccFilterObj *pxHand, float *Buffer, uint16_t BuffLen);

/** 更新加速度差分结果  */
float UnitAlgoAccFilterUpdate(AccFilterObj *pxHand, float NewData) ;

/** 初始化滑动方差对象  */
void UnitAlgoVarianceInit(VarianceObj *pxHand, uint16_t BuffLen);

/** 更新滑动方差输出  */
float UnitAlgoVarianceUpdate(VarianceObj *pxHand, float BuffData[], float NewValue);

/** 初始化滑动标准差对象  */
void UnitAlgoStandDeviationInit(StandDeviatObj *pxHand, uint16_t BuffLen);

/** 更新滑动标准差输出  */
float UnitAlgoStandDeviationUpdate(StandDeviatObj *pxHand, float BuffData[], float NewValue);

/** 初始化高效均值对象  */
void UnitAlgoEfficMeanInit(EfficMeanObj *pxHand, uint16_t MeanLen);

/** 更新高效均值输出  */
float UnitAlgoEfficMeanUpdata(EfficMeanObj *pxHand, uint16_t NewValue);

/** 初始化高效方差对象  */
void UnitAlgoEfficVarianceInit(EfficVarianceObj *pxHand, uint16_t VarLen);

/** 更新高效方差输出  */
float UnitAlgoEfficVarianceUpdata(EfficVarianceObj *pxHand, uint16_t NewValue);

/** 初始化高效标准差对象  */
void UnitAlgoEfficStdDeviatInit(EfficStdDevObj *pxHand, uint16_t VarLen);

/** 更新高效标准差输出  */
float UnitAlgoStdDeviatUpdata(EfficStdDevObj *pxHand, uint16_t NewValue);

/* 工况换算与归一化接口  */

/** 按目标气体工况换算体积  */
float UnitAlgoConvertGasVolume(float Volume, GasConditionObj *Sourece, GasConditionObj *Target);

/** 将物理量归一化到 0 到 1  */
float UnitAlgoPhysicNormalz(float RefPhysicVal, float PhysicMin, float PhysicMax) ;

/** 将归一化量反算为物理量  */
float UnitAlgoPhysicInversNormalz(float fRefNormVal, float PhysicMin, float PhysicMax) ;

/** 初始化固定间隔差分对象  */
void UnitAlgoDiffCalcInit(DiffCalcObj *pxHand, float *DataBuffer, uint16_t DataBuffLen, uint16_t DiffInterval);

/** 复位固定间隔差分对象  */
void UnitAlgoDiffCalcReset(DiffCalcObj *pxHand);

/** 更新固定间隔差分结果  */
float UnitAlgoDiffCalcUpdate(DiffCalcObj *pxHand, float NewData);

#ifdef __cplusplus
}
#endif // extern "C"

#endif
/**************************End of file********************************/
