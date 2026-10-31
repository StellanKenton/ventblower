/************************************************************************************
* @file     : butterworthfilter.h
* @brief    : 
* @details  : 
* @author   : 
* @date     : 
* @version  : 
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef UNIT_ALGO_BUTTERWORTH_FILTER_H
#define UNIT_ALGO_BUTTERWORTH_FILTER_H

/**
 * @file butterworthfilter.h
 * @brief 二阶 Butterworth 低通滤波器接口。
 *
 * 该文件定义 Direct Form 结构的 Butterworth 滤波对象，并提供
 * 初始化、复位和单步更新接口。
 */

#include <string.h>

/** Butterworth 滤波器对象。 */
typedef struct
{
    const float *num;
    const float *den;
    float x1;
    float x2;
    float y1;
    float y2;
} ButterworthFilterObj;

/** 默认电流环路分子系数。 */
extern const float CURRENT_LOOP_FILTER_NUM[3];

/** 默认电流环路分母系数。 */
extern const float CURRENT_LOOP_FILTER_DEN[3];

/** 初始化 Butterworth 滤波器对象。 */
void UnitAlgoButterworthFilterInit(ButterworthFilterObj *pxHand, const float *num, const float *den);

/** 清零 Butterworth 滤波器状态。 */
void UnitAlgoButterworthFilterReset(ButterworthFilterObj *pxHand);

/** 输入新样本并返回滤波输出。 */
float UnitAlgoButterworthFilterUpdate(float NewData, ButterworthFilterObj *pxHand);

#endif
/**************************End of file********************************/
