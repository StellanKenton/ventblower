/***********************************************************************************
* @file     : iir1.c
* @brief    : 
* @details  : 
* @author   : 
* @date     : 
* @version  : 
* @copyright: Copyright (c) 2050
**********************************************************************************/
/**
 * @file iir1.c
 * @brief 一阶滤波算法实现。
 *
 * 该文件实现一阶离散滤波器的系数管理、状态管理和滤波运算，
 * 同时提供一个控制环节使用的简化一阶滤波器。
 */

#include "iir1.h"

#include <stddef.h>

/* 一阶标准滤波器接口。 */

float iir1GetA1(stIir1 *filter)
{
    return (filter != NULL) ? filter->a1 : 0.0f;
}

float iir1GetB0(stIir1 *filter)
{
    return (filter != NULL) ? filter->b0 : 0.0f;
}

float iir1GetB1(stIir1 *filter)
{
    return (filter != NULL) ? filter->b1 : 0.0f;
}

float iir1GetX1(stIir1 *filter)
{
    return (filter != NULL) ? filter->x1 : 0.0f;
}

float iir1GetY1(stIir1 *filter)
{
    return (filter != NULL) ? filter->y1 : 0.0f;
}

float iir1GetDen(stIir1 *filter)
{
    return (filter != NULL) ? filter->a1 : 0.0f;
}

void iir1GetState(stIir1 *filter, float *px1, float *py1)
{
    if (filter == NULL)
    {
        return;
    }

    if (px1 != NULL)
    {
        *px1 = filter->x1;
    }

    if (py1 != NULL)
    {
        *py1 = filter->y1;
    }
}

void iir1GetNum(stIir1 *filter, float *pb0, float *pb1)
{
    if (filter == NULL)
    {
        return;
    }

    if (pb0 != NULL)
    {
        *pb0 = filter->b0;
    }

    if (pb1 != NULL)
    {
        *pb1 = filter->b1;
    }
}

void iir1Init(stIir1 *filter, float b0, float b1, float a1)
{
    if (filter == NULL)
    {
        return;
    }

    filter->a1 = a1;
    filter->b0 = b0;
    filter->b1 = b1;
    filter->x1 = 0.0f;
    filter->y1 = 0.0f;
}

float iir1Run(stIir1 *filter, float input)
{
    float lOutput;

    if (filter == NULL)
    {
        return input;
    }

    lOutput = (filter->b0 * input) + (filter->b1 * filter->x1) - (filter->a1 * filter->y1);
    filter->x1 = input;
    filter->y1 = lOutput;

    return lOutput;
}

float iir1RunB0(stIir1 *filter, float input)
{
    float lOutput;

    if (filter == NULL)
    {
        return input;
    }

    lOutput = (filter->b0 * input) - (filter->a1 * filter->y1);
    filter->x1 = input;
    filter->y1 = lOutput;

    return lOutput;
}

void iir1SetA1(stIir1 *filter, float a1)
{
    if (filter != NULL)
    {
        filter->a1 = a1;
    }
}

void iir1SetB0(stIir1 *filter, float b0)
{
    if (filter != NULL)
    {
        filter->b0 = b0;
    }
}

void iir1SetB1(stIir1 *filter, float b1)
{
    if (filter != NULL)
    {
        filter->b1 = b1;
    }
}

void iir1SetX1(stIir1 *filter, float x1)
{
    if (filter != NULL)
    {
        filter->x1 = x1;
    }
}

void iir1SetY1(stIir1 *filter, float y1)
{
    if (filter != NULL)
    {
        filter->y1 = y1;
    }
}

void iir1SetDen(stIir1 *filter, float a1)
{
    if (filter != NULL)
    {
        filter->a1 = a1;
    }
}

void iir1SetState(stIir1 *filter, float x1, float y1)
{
    if (filter == NULL)
    {
        return;
    }

    filter->x1 = x1;
    filter->y1 = y1;
}

void iir1SetNum(stIir1 *filter, float b0, float b1)
{
    if (filter == NULL)
    {
        return;
    }

    filter->b0 = b0;
    filter->b1 = b1;
}

/* 控制环节用简化滤波器接口。 */

void lpf1Init(stLpf1 *filter, float gain)
{
    if (filter == NULL)
    {
        return;
    }

    filter->gain = gain;
    filter->x1 = 0.0f;
    filter->y = 0.0f;
    filter->y1 = 0.0f;
}

void lpf1SetGain(stLpf1 *filter, float gain)
{
    if (filter != NULL)
    {
        filter->gain = gain;
    }
}

void lpf1Reset(stLpf1 *filter)
{
    if (filter == NULL)
    {
        return;
    }

    filter->x1 = 0.0f;
    filter->y = 0.0f;
    filter->y1 = 0.0f;
}

float lpf1Run(stLpf1 *filter, float input)
{
    float lOutput;

    if (filter == NULL)
    {
        return input;
    }

    lOutput = (filter->gain * input) + ((1.0f - filter->gain) * filter->y1);
    filter->x1 = input;
    filter->y = lOutput;
    filter->y1 = lOutput;

    return lOutput;
}

/**************************End of file********************************/
