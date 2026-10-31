/***********************************************************************************
* @file     : numfilter.c
* @brief    : 
* @details  : 
* @author   : 
* @date     : 
* @version  : 
* @copyright: Copyright (c) 2050
**********************************************************************************/
/**
 * @file numfilter.c
 * @brief 数值处理与通用滤波算法实现。
 *
 * 该文件实现比例计算、插值、二维表查找、平均滤波、统计量计算、
 * 工况换算和固定间隔差分等基础算法。
 */

#include "numfilter.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define UNIT_ALGO_FLOAT_EPSILON    (1.0e-6f)

/* 常用气体工况定义。 */
GasConditionObj ATPD = {760.0f, 298.15f, 0.0f};
GasConditionObj ATPS = {760.0f, 298.15f, 23.8f};
GasConditionObj STPD = {760.0f, 273.15f, 0.0f};
GasConditionObj BTPS = {760.0f, 310.15f, 47.0f};

/* 内部通用辅助函数。 */
static float UnitAlgoLinearInterpolate(float x0, float y0, float x1, float y1, float x)
{
    float dx;

    dx = x1 - x0;
    if (fabsf(dx) <= UNIT_ALGO_FLOAT_EPSILON)
    {
        return y0;
    }

    return y0 + (((x - x0) * (y1 - y0)) / dx);
}

static uint16_t UnitAlgoActiveLength(uint16_t length, uint8_t fullFlag, uint16_t index)
{
    if (fullFlag != 0U)
    {
        return length;
    }

    return index;
}

static void UnitAlgoAvgFilterRecalc(AvgFilterObj *pxHand, uint16_t activeLen)
{
    uint16_t index;

    if ((pxHand == NULL) || (pxHand->m_Buff == NULL) || (activeLen == 0U))
    {
        return;
    }

    pxHand->m_Max = pxHand->m_Buff[0];
    pxHand->m_Min = pxHand->m_Buff[0];

    for (index = 1U; index < activeLen; ++index)
    {
        if (pxHand->m_Buff[index] > pxHand->m_Max)
        {
            pxHand->m_Max = pxHand->m_Buff[index];
        }

        if (pxHand->m_Buff[index] < pxHand->m_Min)
        {
            pxHand->m_Min = pxHand->m_Buff[index];
        }
    }
}

static float UnitAlgoCalcVariance(float sumXi, float sumXiSquare, uint16_t count)
{
    float mean;
    float variance;

    if (count == 0U)
    {
        return 0.0f;
    }

    mean = sumXi / (float)count;
    variance = (sumXiSquare / (float)count) - (mean * mean);
    return (variance > 0.0f) ? variance : 0.0f;
}

static int16_t UnitAlgoFindExact2DPoint(const Tab2DObj *Tab, float x, float y, Tab2DPointObj *point)
{
    int16_t index;

    if ((Tab == NULL) || (Tab->m_Points == NULL) || (point == NULL))
    {
        return -1;
    }

    for (index = 0; index < Tab->m_size; ++index)
    {
        if ((fabsf(Tab->m_Points[index].m_x - x) <= UNIT_ALGO_FLOAT_EPSILON)
         && (fabsf(Tab->m_Points[index].m_y - y) <= UNIT_ALGO_FLOAT_EPSILON))
        {
            *point = Tab->m_Points[index];
            return index;
        }
    }

    return -1;
}

/* 比例计算与插值接口。 */
void UnitAlgoNumStatProportInit(ProportObj *pxHand, float k, float b)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->m_k = k;
    pxHand->m_b = b;
    pxHand->m_y = 0.0f;
}

float UnitAlgoNumStatProportCalc(ProportObj *pxHand, float x)
{
    if (pxHand == NULL)
    {
        return x;
    }

    pxHand->m_y = (pxHand->m_k * x) + pxHand->m_b;
    return pxHand->m_y;
}

void UnitAlgoLagrangeInit(LagrangeObj *pxHand)
{
    if (pxHand != NULL)
    {
        pxHand->m_Output = 0.0f;
    }
}

void UnitAlgoIncreaseBinarySearchLut(float *pArr, uint32_t ArrLen, float Data, uint32_t *pIndexLeft)
{
    uint32_t left;
    uint32_t right;
    uint32_t mid;

    if ((pArr == NULL) || (pIndexLeft == NULL) || (ArrLen < 2U))
    {
        if (pIndexLeft != NULL)
        {
            *pIndexLeft = 0U;
        }
        return;
    }

    if (Data <= pArr[0])
    {
        *pIndexLeft = 0U;
        return;
    }

    if (Data >= pArr[ArrLen - 1U])
    {
        *pIndexLeft = ArrLen - 2U;
        return;
    }

    left = 0U;
    right = ArrLen - 1U;
    while ((right - left) > 1U)
    {
        mid = left + ((right - left) / 2U);
        if (pArr[mid] <= Data)
        {
            left = mid;
        }
        else
        {
            right = mid;
        }
    }

    *pIndexLeft = left;
}

void UnitAlgoDecreaseBinarySearchLut(float *pArr, uint32_t ArrLen, float Data, uint32_t *pIndexLeft)
{
    uint32_t left;
    uint32_t right;
    uint32_t mid;

    if ((pArr == NULL) || (pIndexLeft == NULL) || (ArrLen < 2U))
    {
        if (pIndexLeft != NULL)
        {
            *pIndexLeft = 0U;
        }
        return;
    }

    if (Data >= pArr[0])
    {
        *pIndexLeft = 0U;
        return;
    }

    if (Data <= pArr[ArrLen - 1U])
    {
        *pIndexLeft = ArrLen - 2U;
        return;
    }

    left = 0U;
    right = ArrLen - 1U;
    while ((right - left) > 1U)
    {
        mid = left + ((right - left) / 2U);
        if (pArr[mid] >= Data)
        {
            left = mid;
        }
        else
        {
            right = mid;
        }
    }

    *pIndexLeft = left;
}

float UnitAlgoIncreaseLagrangeCalc(LagrangeObj *pxHand, float *xArr, float *yArr, float Data, int16_t Size)
{
    uint32_t indexLeft;

    if ((pxHand == NULL) || (xArr == NULL) || (yArr == NULL) || (Size <= 0))
    {
        return 0.0f;
    }

    if (Size == 1)
    {
        pxHand->m_Output = yArr[0];
        return pxHand->m_Output;
    }

    UnitAlgoIncreaseBinarySearchLut(xArr, (uint32_t)Size, Data, &indexLeft);
    pxHand->m_Output = UnitAlgoLinearInterpolate(xArr[indexLeft], yArr[indexLeft], xArr[indexLeft + 1U], yArr[indexLeft + 1U], Data);
    return pxHand->m_Output;
}

float UnitAlgoDecreaseLagrangeCalc(LagrangeObj *pxHand, float *xArr, float *yArr, float Data, int16_t Size)
{
    uint32_t indexLeft;

    if ((pxHand == NULL) || (xArr == NULL) || (yArr == NULL) || (Size <= 0))
    {
        return 0.0f;
    }

    if (Size == 1)
    {
        pxHand->m_Output = yArr[0];
        return pxHand->m_Output;
    }

    UnitAlgoDecreaseBinarySearchLut(xArr, (uint32_t)Size, Data, &indexLeft);
    pxHand->m_Output = UnitAlgoLinearInterpolate(xArr[indexLeft], yArr[indexLeft], xArr[indexLeft + 1U], yArr[indexLeft + 1U], Data);
    return pxHand->m_Output;
}

int16_t UnitAlgoFind2DTabLowerIndex(const Tab2DObj *Tab, float Data, int16_t xyFlag)
{
    int16_t index;
    int16_t bestIndex;
    float bestValue;
    float coord;

    if ((Tab == NULL) || (Tab->m_Points == NULL) || (Tab->m_size <= 0))
    {
        return -1;
    }

    bestIndex = 0;
    bestValue = (xyFlag == 0) ? Tab->m_Points[0].m_x : Tab->m_Points[0].m_y;

    for (index = 0; index < Tab->m_size; ++index)
    {
        coord = (xyFlag == 0) ? Tab->m_Points[index].m_x : Tab->m_Points[index].m_y;
        if ((coord <= Data) && ((bestValue > Data) || (coord >= bestValue)))
        {
            bestValue = coord;
            bestIndex = index;
        }
    }

    return bestIndex;
}

int16_t UnitAlgoFind2DTabSurroundPoints(const Tab2DObj *Tab, float x, float y, Tab2DPointObj *Q00, Tab2DPointObj *Q10, Tab2DPointObj *Q01, Tab2DPointObj *Q11)
{
    int16_t index;
    uint8_t hasXLow;
    uint8_t hasXHigh;
    uint8_t hasYLow;
    uint8_t hasYHigh;
    float xLow;
    float xHigh;
    float yLow;
    float yHigh;
    float xVal;
    float yVal;

    if ((Tab == NULL) || (Tab->m_Points == NULL) || (Q00 == NULL) || (Q10 == NULL) || (Q01 == NULL) || (Q11 == NULL) || (Tab->m_size <= 0))
    {
        return -1;
    }

    hasXLow = 0U;
    hasXHigh = 0U;
    hasYLow = 0U;
    hasYHigh = 0U;
    xLow = 0.0f;
    xHigh = 0.0f;
    yLow = 0.0f;
    yHigh = 0.0f;

    for (index = 0; index < Tab->m_size; ++index)
    {
        xVal = Tab->m_Points[index].m_x;
        yVal = Tab->m_Points[index].m_y;

        if ((xVal <= x) && ((hasXLow == 0U) || (xVal > xLow)))
        {
            xLow = xVal;
            hasXLow = 1U;
        }

        if ((xVal >= x) && ((hasXHigh == 0U) || (xVal < xHigh)))
        {
            xHigh = xVal;
            hasXHigh = 1U;
        }

        if ((yVal <= y) && ((hasYLow == 0U) || (yVal > yLow)))
        {
            yLow = yVal;
            hasYLow = 1U;
        }

        if ((yVal >= y) && ((hasYHigh == 0U) || (yVal < yHigh)))
        {
            yHigh = yVal;
            hasYHigh = 1U;
        }
    }

    if (hasXLow == 0U)
    {
        xLow = xHigh;
    }

    if (hasXHigh == 0U)
    {
        xHigh = xLow;
    }

    if (hasYLow == 0U)
    {
        yLow = yHigh;
    }

    if (hasYHigh == 0U)
    {
        yHigh = yLow;
    }

    if ((UnitAlgoFindExact2DPoint(Tab, xLow, yLow, Q00) < 0)
     || (UnitAlgoFindExact2DPoint(Tab, xHigh, yLow, Q10) < 0)
     || (UnitAlgoFindExact2DPoint(Tab, xLow, yHigh, Q01) < 0)
     || (UnitAlgoFindExact2DPoint(Tab, xHigh, yHigh, Q11) < 0))
    {
        return -1;
    }

    return 0;
}

float UnitAlgoBilinearInterpolatePoints(const Tab2DObj *Tab, float x, float y)
{
    Tab2DPointObj Q00;
    Tab2DPointObj Q10;
    Tab2DPointObj Q01;
    Tab2DPointObj Q11;
    float zLow;
    float zHigh;

    if (UnitAlgoFind2DTabSurroundPoints(Tab, x, y, &Q00, &Q10, &Q01, &Q11) != 0)
    {
        return 0.0f;
    }

    if (fabsf(Q00.m_x - Q10.m_x) <= UNIT_ALGO_FLOAT_EPSILON)
    {
        return UnitAlgoLinearInterpolate(Q00.m_y, Q00.m_z, Q01.m_y, Q01.m_z, y);
    }

    if (fabsf(Q00.m_y - Q01.m_y) <= UNIT_ALGO_FLOAT_EPSILON)
    {
        return UnitAlgoLinearInterpolate(Q00.m_x, Q00.m_z, Q10.m_x, Q10.m_z, x);
    }

    zLow = UnitAlgoLinearInterpolate(Q00.m_x, Q00.m_z, Q10.m_x, Q10.m_z, x);
    zHigh = UnitAlgoLinearInterpolate(Q01.m_x, Q01.m_z, Q11.m_x, Q11.m_z, x);
    return UnitAlgoLinearInterpolate(Q00.m_y, zLow, Q01.m_y, zHigh, y);
}

/* 一阶传递函数接口。 */
void UnitAlgoFirstOrdTransfInit(FirstOrdTransfObj *pxHand, float Period, float a, float b)
{
    float den;

    if (pxHand == NULL)
    {
        return;
    }

    if (fabsf(a) <= UNIT_ALGO_FLOAT_EPSILON)
    {
        pxHand->m_a1 = 0.0f;
        pxHand->m_b1 = 1.0f;
        pxHand->m_b2 = 0.0f;
        pxHand->m_xMinusOne = 0.0f;
        pxHand->m_yMinusOne = 0.0f;
        return;
    }

    den = Period + (2.0f * a);
    if (fabsf(den) <= UNIT_ALGO_FLOAT_EPSILON)
    {
        den = UNIT_ALGO_FLOAT_EPSILON;
    }

    pxHand->m_a1 = (Period - (2.0f * a)) / den;
    pxHand->m_b1 = (Period + (2.0f * b)) / den;
    pxHand->m_b2 = (Period - (2.0f * b)) / den;
    pxHand->m_xMinusOne = 0.0f;
    pxHand->m_yMinusOne = 0.0f;
}

void UnitAlgoFirstOrdTransfStateSet(FirstOrdTransfObj *pxHand, float Data)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->m_xMinusOne = Data;
    pxHand->m_yMinusOne = Data;
}

float UnitAlgoFirstOrdTransfUpdata(FirstOrdTransfObj *pxHand, float Data)
{
    float output;

    if (pxHand == NULL)
    {
        return Data;
    }

    output = (pxHand->m_b1 * Data) + (pxHand->m_b2 * pxHand->m_xMinusOne) - (pxHand->m_a1 * pxHand->m_yMinusOne);
    pxHand->m_xMinusOne = Data;
    pxHand->m_yMinusOne = output;
    return output;
}

/* 平均滤波接口。 */
void UnitAlgoMovAvgFilterInit(MovAvgFilterObj *pxHand, float *DataBuffer, uint16_t DataBuffLen)
{
    if ((pxHand == NULL) || (DataBuffer == NULL) || (DataBuffLen == 0U))
    {
        return;
    }

    pxHand->m_BuffLen = DataBuffLen;
    pxHand->m_Index = 0U;
    pxHand->m_BuffFull = 0U;
    pxHand->m_Buffer = DataBuffer;
    pxHand->m_Sum = 0.0f;
    UnitAlgoMovAvgFilterReset(pxHand);
}

void UnitAlgoMovAvgFilterReset(MovAvgFilterObj *pxHand)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->m_Index = 0U;
    pxHand->m_BuffFull = 0U;
    pxHand->m_Sum = 0.0f;

    if ((pxHand->m_Buffer != NULL) && (pxHand->m_BuffLen > 0U))
    {
        memset(pxHand->m_Buffer, 0, sizeof(float) * pxHand->m_BuffLen);
    }
}

float UnitAlgoMovAvgFilterUpdata(MovAvgFilterObj *pxHand, float NewData)
{
    uint16_t activeLen;

    if ((pxHand == NULL) || (pxHand->m_Buffer == NULL) || (pxHand->m_BuffLen == 0U))
    {
        return NewData;
    }

    if (pxHand->m_BuffFull != 0U)
    {
        pxHand->m_Sum -= pxHand->m_Buffer[pxHand->m_Index];
    }

    pxHand->m_Buffer[pxHand->m_Index] = NewData;
    pxHand->m_Sum += NewData;
    pxHand->m_Index++;

    if (pxHand->m_Index >= pxHand->m_BuffLen)
    {
        pxHand->m_Index = 0U;
        pxHand->m_BuffFull = 1U;
    }

    activeLen = UnitAlgoActiveLength(pxHand->m_BuffLen, pxHand->m_BuffFull, pxHand->m_Index);
    return (activeLen > 0U) ? (pxHand->m_Sum / (float)activeLen) : NewData;
}

void UnitAlgoAvgFilterInit(AvgFilterObj *pxHand, float *DataBuffer, uint16_t DataBuffLen)
{
    if ((pxHand == NULL) || (DataBuffer == NULL) || (DataBuffLen == 0U))
    {
        return;
    }

    pxHand->m_BuffLen = DataBuffLen;
    pxHand->m_Index = 0U;
    pxHand->m_BufFull = 0U;
    pxHand->m_Buff = DataBuffer;
    pxHand->m_Sum = 0.0f;
    pxHand->m_Max = 0.0f;
    pxHand->m_Min = 0.0f;
    UnitAlgoAvgFilterReset(pxHand);
}

void UnitAlgoAvgFilterReset(AvgFilterObj *pxHand)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->m_Index = 0U;
    pxHand->m_BufFull = 0U;
    pxHand->m_Sum = 0.0f;
    pxHand->m_Max = 0.0f;
    pxHand->m_Min = 0.0f;

    if ((pxHand->m_Buff != NULL) && (pxHand->m_BuffLen > 0U))
    {
        memset(pxHand->m_Buff, 0, sizeof(float) * pxHand->m_BuffLen);
    }
}

float UnitAlgoAvgFilterUpdata(AvgFilterObj *pxHand, float NewData)
{
    uint16_t activeLen;
    float output;

    if ((pxHand == NULL) || (pxHand->m_Buff == NULL) || (pxHand->m_BuffLen == 0U))
    {
        return NewData;
    }

    if (pxHand->m_BufFull != 0U)
    {
        pxHand->m_Sum -= pxHand->m_Buff[pxHand->m_Index];
    }

    pxHand->m_Buff[pxHand->m_Index] = NewData;
    pxHand->m_Sum += NewData;
    pxHand->m_Index++;

    if (pxHand->m_Index >= pxHand->m_BuffLen)
    {
        pxHand->m_Index = 0U;
        pxHand->m_BufFull = 1U;
    }

    activeLen = UnitAlgoActiveLength(pxHand->m_BuffLen, pxHand->m_BufFull, pxHand->m_Index);
    UnitAlgoAvgFilterRecalc(pxHand, activeLen);
    if (activeLen <= 2U)
    {
        output = (activeLen > 0U) ? (pxHand->m_Sum / (float)activeLen) : NewData;
    }
    else
    {
        output = (pxHand->m_Sum - pxHand->m_Max - pxHand->m_Min) / (float)(activeLen - 2U);
    }

    return output;
}

void UnitAlgoPhaseLockFilterInit(PhaseLockFilterObj *pxHand, float *DataBuffer, uint16_t DataBuffLen, float Value, uint32_t Time, uint32_t Period)
{
    uint16_t index;

    if ((pxHand == NULL) || (DataBuffer == NULL) || (DataBuffLen == 0U))
    {
        return;
    }

    pxHand->m_BackwardTime = Time;
    pxHand->m_Period = (Period == 0U) ? 1U : Period;
    pxHand->m_Pointer = 0U;
    pxHand->m_BuffLen = DataBuffLen;
    pxHand->m_FirstInFlag = 1U;
    pxHand->m_Buffer = DataBuffer;
    pxHand->m_DownwardVal = Value;
    pxHand->m_FiltOut = Value;

    for (index = 0U; index < DataBuffLen; ++index)
    {
        pxHand->m_Buffer[index] = Value;
    }
}

void UnitAlgoPhaseLockFilterReset(PhaseLockFilterObj *pxHand)
{
    uint32_t index;

    if ((pxHand == NULL) || (pxHand->m_Buffer == NULL) || (pxHand->m_BuffLen == 0U))
    {
        return;
    }

    for (index = 0U; index < pxHand->m_BuffLen; ++index)
    {
        pxHand->m_Buffer[index] = 0.0f;
    }

    pxHand->m_Pointer = 0U;
    pxHand->m_FirstInFlag = 1U;
    pxHand->m_DownwardVal = 0.0f;
    pxHand->m_FiltOut = 0.0f;
}

float UnitAlgoPhaseLockFilterUpdata(PhaseLockFilterObj *pxHand, float NewData)
{
    uint32_t index;
    float sum;
    float minValue;

    if ((pxHand == NULL) || (pxHand->m_Buffer == NULL) || (pxHand->m_BuffLen == 0U))
    {
        return NewData;
    }

    pxHand->m_Buffer[pxHand->m_Pointer] = NewData;
    pxHand->m_Pointer = (pxHand->m_Pointer + 1U) % pxHand->m_BuffLen;
    pxHand->m_BackwardTime += pxHand->m_Period;

    sum = 0.0f;
    minValue = pxHand->m_Buffer[0];
    for (index = 0U; index < pxHand->m_BuffLen; ++index)
    {
        sum += pxHand->m_Buffer[index];
        if (pxHand->m_Buffer[index] < minValue)
        {
            minValue = pxHand->m_Buffer[index];
        }
    }

    pxHand->m_DownwardVal = minValue;
    pxHand->m_FiltOut = sum / (float)pxHand->m_BuffLen;
    pxHand->m_FirstInFlag = 0U;

    return pxHand->m_FiltOut;
}

/* 统计与差分接口。 */
void UnitAlgoAccFilterInit(AccFilterObj *pxHand, float *Buffer, uint16_t BuffLen)
{
    if ((pxHand == NULL) || (Buffer == NULL) || (BuffLen == 0U))
    {
        return;
    }

    pxHand->m_BuffLen = BuffLen;
    pxHand->m_Index = 0U;
    pxHand->m_WdLen = 0U;
    pxHand->m_Buffer = Buffer;
    pxHand->m_NewAdd = 0.0f;
    pxHand->m_OldAdd = 0.0f;
    memset(Buffer, 0, sizeof(float) * BuffLen);
}

float UnitAlgoAccFilterUpdate(AccFilterObj *pxHand, float NewData)
{
    float oldData;

    if ((pxHand == NULL) || (pxHand->m_Buffer == NULL) || (pxHand->m_BuffLen == 0U))
    {
        return 0.0f;
    }

    oldData = pxHand->m_Buffer[pxHand->m_Index];
    pxHand->m_Buffer[pxHand->m_Index] = NewData;
    pxHand->m_Index = (pxHand->m_Index + 1U) % pxHand->m_BuffLen;

    if (pxHand->m_WdLen < pxHand->m_BuffLen)
    {
        pxHand->m_WdLen++;
    }

    pxHand->m_OldAdd = oldData;
    pxHand->m_NewAdd = NewData - oldData;
    return pxHand->m_NewAdd;
}

void UnitAlgoVarianceInit(VarianceObj *pxHand, uint16_t BuffLen)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->m_FullFlag = 0U;
    pxHand->m_Len = BuffLen;
    pxHand->m_Ptr = 0U;
    pxHand->m_SumXi = 0.0f;
    pxHand->m_SumXiSquare = 0.0f;
    pxHand->m_VarOutput = 0.0f;
}

float UnitAlgoVarianceUpdate(VarianceObj *pxHand, float BuffData[], float NewValue)
{
    float oldValue;
    uint16_t count;

    if ((pxHand == NULL) || (BuffData == NULL) || (pxHand->m_Len == 0U))
    {
        return 0.0f;
    }

    if (pxHand->m_FullFlag != 0U)
    {
        oldValue = BuffData[pxHand->m_Ptr];
        pxHand->m_SumXi -= oldValue;
        pxHand->m_SumXiSquare -= oldValue * oldValue;
    }

    BuffData[pxHand->m_Ptr] = NewValue;
    pxHand->m_SumXi += NewValue;
    pxHand->m_SumXiSquare += NewValue * NewValue;
    pxHand->m_Ptr++;

    if (pxHand->m_Ptr >= pxHand->m_Len)
    {
        pxHand->m_Ptr = 0U;
        pxHand->m_FullFlag = 1U;
    }

    count = (pxHand->m_FullFlag != 0U) ? pxHand->m_Len : pxHand->m_Ptr;
    pxHand->m_VarOutput = UnitAlgoCalcVariance(pxHand->m_SumXi, pxHand->m_SumXiSquare, count);
    return pxHand->m_VarOutput;
}

void UnitAlgoStandDeviationInit(StandDeviatObj *pxHand, uint16_t BuffLen)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->m_FullFlag = 0U;
    pxHand->m_Len = BuffLen;
    pxHand->m_Ptr = 0U;
    pxHand->m_SumXi = 0.0f;
    pxHand->m_SumXiSquare = 0.0f;
    pxHand->m_StdOutput = 0.0f;
}

float UnitAlgoStandDeviationUpdate(StandDeviatObj *pxHand, float BuffData[], float NewValue)
{
    float oldValue;
    uint16_t count;
    float variance;

    if ((pxHand == NULL) || (BuffData == NULL) || (pxHand->m_Len == 0U))
    {
        return 0.0f;
    }

    if (pxHand->m_FullFlag != 0U)
    {
        oldValue = BuffData[pxHand->m_Ptr];
        pxHand->m_SumXi -= oldValue;
        pxHand->m_SumXiSquare -= oldValue * oldValue;
    }

    BuffData[pxHand->m_Ptr] = NewValue;
    pxHand->m_SumXi += NewValue;
    pxHand->m_SumXiSquare += NewValue * NewValue;
    pxHand->m_Ptr++;

    if (pxHand->m_Ptr >= pxHand->m_Len)
    {
        pxHand->m_Ptr = 0U;
        pxHand->m_FullFlag = 1U;
    }

    count = (pxHand->m_FullFlag != 0U) ? pxHand->m_Len : pxHand->m_Ptr;
    variance = UnitAlgoCalcVariance(pxHand->m_SumXi, pxHand->m_SumXiSquare, count);
    pxHand->m_StdOutput = sqrtf(variance);
    return pxHand->m_StdOutput;
}

void UnitAlgoEfficMeanInit(EfficMeanObj *pxHand, uint16_t MeanLen)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->m_Cnt = 0U;
    pxHand->m_Len = MeanLen;
    pxHand->m_Mean = 0.0f;
}

float UnitAlgoEfficMeanUpdata(EfficMeanObj *pxHand, uint16_t NewValue)
{
    float count;

    if ((pxHand == NULL) || (pxHand->m_Len == 0U))
    {
        return 0.0f;
    }

    if (pxHand->m_Cnt >= pxHand->m_Len)
    {
        return pxHand->m_Mean;
    }

    pxHand->m_Cnt++;
    count = (float)pxHand->m_Cnt;
    pxHand->m_Mean += (((float)NewValue) - pxHand->m_Mean) / count;
    return pxHand->m_Mean;
}

void UnitAlgoEfficVarianceInit(EfficVarianceObj *pxHand, uint16_t VarLen)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->m_Cnt = 0U;
    pxHand->m_Len = VarLen;
    pxHand->m_LastMean = 0.0f;
    pxHand->m_LastSum = 0.0f;
    pxHand->m_LastVarOut = 0.0f;
}

float UnitAlgoEfficVarianceUpdata(EfficVarianceObj *pxHand, uint16_t NewValue)
{
    float delta;
    float newMean;
    float count;

    if ((pxHand == NULL) || (pxHand->m_Len == 0U))
    {
        return 0.0f;
    }

    if (pxHand->m_Cnt >= pxHand->m_Len)
    {
        return pxHand->m_LastVarOut;
    }

    pxHand->m_Cnt++;
    count = (float)pxHand->m_Cnt;
    delta = (float)NewValue - pxHand->m_LastMean;
    newMean = pxHand->m_LastMean + (delta / count);
    pxHand->m_LastSum += delta * ((float)NewValue - newMean);
    pxHand->m_LastMean = newMean;
    pxHand->m_LastVarOut = pxHand->m_LastSum / count;

    return pxHand->m_LastVarOut;
}

void UnitAlgoEfficStdDeviatInit(EfficStdDevObj *pxHand, uint16_t VarLen)
{
    if (pxHand == NULL)
    {
        return;
    }

    pxHand->m_Cnt = 0U;
    pxHand->m_Len = VarLen;
    pxHand->m_LastMean = 0.0f;
    pxHand->m_LastSum = 0.0f;
    pxHand->m_StdOut = 0.0f;
}

float UnitAlgoStdDeviatUpdata(EfficStdDevObj *pxHand, uint16_t NewValue)
{
    float delta;
    float newMean;
    float count;
    float variance;

    if ((pxHand == NULL) || (pxHand->m_Len == 0U))
    {
        return 0.0f;
    }

    if (pxHand->m_Cnt >= pxHand->m_Len)
    {
        return pxHand->m_StdOut;
    }

    pxHand->m_Cnt++;
    count = (float)pxHand->m_Cnt;
    delta = (float)NewValue - pxHand->m_LastMean;
    newMean = pxHand->m_LastMean + (delta / count);
    pxHand->m_LastSum += delta * ((float)NewValue - newMean);
    pxHand->m_LastMean = newMean;
    variance = pxHand->m_LastSum / count;
    pxHand->m_StdOut = sqrtf((variance > 0.0f) ? variance : 0.0f);

    return pxHand->m_StdOut;
}

float UnitAlgoConvertGasVolume(float Volume, GasConditionObj *Sourece, GasConditionObj *Target)
{
    float sourceDryPressure;
    float targetDryPressure;

    if ((Sourece == NULL) || (Target == NULL) || (Sourece->m_Temp_K <= 0.0f))
    {
        return Volume;
    }

    sourceDryPressure = Sourece->m_Press_mmHg - Sourece->m_Vapor_mmHg;
    targetDryPressure = Target->m_Press_mmHg - Target->m_Vapor_mmHg;

    if (targetDryPressure <= UNIT_ALGO_FLOAT_EPSILON)
    {
        return Volume;
    }

    return Volume * (sourceDryPressure / targetDryPressure) * (Target->m_Temp_K / Sourece->m_Temp_K);
}

float UnitAlgoPhysicNormalz(float RefPhysicVal, float PhysicMin, float PhysicMax)
{
    float den;

    den = PhysicMax - PhysicMin;
    if (fabsf(den) <= UNIT_ALGO_FLOAT_EPSILON)
    {
        return 0.0f;
    }

    return (RefPhysicVal - PhysicMin) / den;
}

float UnitAlgoPhysicInversNormalz(float fRefNormVal, float PhysicMin, float PhysicMax)
{
    return PhysicMin + (fRefNormVal * (PhysicMax - PhysicMin));
}

/* 固定间隔差分接口。 */
void UnitAlgoDiffCalcInit(DiffCalcObj *pxHand, float *DataBuffer, uint16_t DataBuffLen, uint16_t DiffInterval)
{
    if ((pxHand == NULL) || (DataBuffer == NULL) || (DataBuffLen == 0U))
    {
        return;
    }

    pxHand->m_BuffLen = DataBuffLen;
    pxHand->m_Index = 0U;
    pxHand->m_DiffInterval = (DiffInterval >= DataBuffLen) ? (DataBuffLen - 1U) : DiffInterval;
    pxHand->m_FirstInFlag = 1U;
    pxHand->m_Buffer = DataBuffer;
    pxHand->m_DiffOut = 0.0f;

    memset(DataBuffer, 0, sizeof(float) * DataBuffLen);
}

void UnitAlgoDiffCalcReset(DiffCalcObj *pxHand)
{
    if ((pxHand == NULL) || (pxHand->m_Buffer == NULL) || (pxHand->m_BuffLen == 0U))
    {
        return;
    }

    pxHand->m_Index = 0U;
    pxHand->m_FirstInFlag = 1U;
    pxHand->m_DiffOut = 0.0f;
    memset(pxHand->m_Buffer, 0, sizeof(float) * pxHand->m_BuffLen);
}

float UnitAlgoDiffCalcUpdate(DiffCalcObj *pxHand, float NewData)
{
    uint16_t oldIndex;
    float oldData;

    if ((pxHand == NULL) || (pxHand->m_Buffer == NULL) || (pxHand->m_BuffLen == 0U))
    {
        return 0.0f;
    }

    if ((pxHand->m_FirstInFlag != 0U) && (pxHand->m_Index < pxHand->m_DiffInterval))
    {
        pxHand->m_Buffer[pxHand->m_Index] = NewData;
        pxHand->m_Index++;
        pxHand->m_DiffOut = 0.0f;
        return pxHand->m_DiffOut;
    }

    if (pxHand->m_DiffInterval == 0U)
    {
        pxHand->m_DiffOut = 0.0f;
    }
    else
    {
        oldIndex = (uint16_t)((pxHand->m_Index + pxHand->m_BuffLen - pxHand->m_DiffInterval) % pxHand->m_BuffLen);
        oldData = pxHand->m_Buffer[oldIndex];
        pxHand->m_DiffOut = NewData - oldData;
    }

    pxHand->m_Buffer[pxHand->m_Index] = NewData;
    pxHand->m_Index++;

    if (pxHand->m_Index >= pxHand->m_BuffLen)
    {
        pxHand->m_Index = 0U;
        pxHand->m_FirstInFlag = 0U;
    }

    return pxHand->m_DiffOut;
}

/**************************End of file********************************/
