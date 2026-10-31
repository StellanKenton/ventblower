/***********************************************************************************
* @file     : butterworthfilter.c
* @brief    : 
* @details  : 
* @author   : 
* @date     : 
* @version  : 
* @copyright: Copyright (c) 2050
**********************************************************************************/
/**
 * @file butterworthfilter.c
 * @brief 二阶 Butterworth 低通滤波器实现。
 *
 * 该文件实现二阶 Butterworth 滤波器的初始化、状态复位和单步更新逻辑。
 */

#include "butterworthfilter.h"

#include <stddef.h>

/* 默认示例系数，可在使用时替换为实际设计参数。 */
const float CURRENT_LOOP_FILTER_NUM[3] = {1.0f, 0.0f, 0.0f};
const float CURRENT_LOOP_FILTER_DEN[3] = {1.0f, 0.0f, 0.0f};

void UnitAlgoButterworthFilterInit(ButterworthFilterObj *pxHand, const float *num, const float *den)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->num = num;
    pxHand->den = den;
    UnitAlgoButterworthFilterReset(pxHand);
}

void UnitAlgoButterworthFilterReset(ButterworthFilterObj *pxHand)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->x1 = 0.0f;
    pxHand->x2 = 0.0f;
    pxHand->y1 = 0.0f;
    pxHand->y2 = 0.0f;
}

float UnitAlgoButterworthFilterUpdate(float NewData, ButterworthFilterObj *pxHand)
{
    const float *num;
    const float *den;
    float a0;
    float output;

    if ((pxHand == NULL) || (pxHand->num == NULL) || (pxHand->den == NULL))
    {
        return NewData;
    }

    num = pxHand->num;
    den = pxHand->den;
    a0 = (den[0] != 0.0f) ? den[0] : 1.0f;

    output = ((num[0] * NewData) + (num[1] * pxHand->x1) + (num[2] * pxHand->x2)
            - (den[1] * pxHand->y1) - (den[2] * pxHand->y2)) / a0;

    pxHand->x2 = pxHand->x1;
    pxHand->x1 = NewData;
    pxHand->y2 = pxHand->y1;
    pxHand->y1 = output;

    return output;
}

/**************************End of file********************************/
