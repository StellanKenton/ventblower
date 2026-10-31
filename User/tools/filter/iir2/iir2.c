/***********************************************************************************
* @file     : iir2.c
* @brief    : 
* @details  : 
* @author   : 
* @date     : 
* @version  : 
* @copyright: Copyright (c) 2050
**********************************************************************************/
/**
 * @file iir2.c
 * @brief 二阶滤波算法实现。
 *
 * 该文件实现二阶离散滤波器的系数访问、状态管理和多种差分方程更新逻辑。
 */

#include "iir2.h"

#include <stddef.h>

/* 二阶滤波器参数读取接口。 */

float iir2GetA1(stIir2 *filter)
{
    return (filter != NULL) ? filter->a1 : 0.0f;
}

float iir2GetA2(stIir2 *filter)
{
    return (filter != NULL) ? filter->a2 : 0.0f;
}

float iir2GetB0(stIir2 *filter)
{
    return (filter != NULL) ? filter->b0 : 0.0f;
}

float iir2GetB1(stIir2 *filter)
{
    return (filter != NULL) ? filter->b1 : 0.0f;
}

float iir2GetB2(stIir2 *filter)
{
    return (filter != NULL) ? filter->b2 : 0.0f;
}

float iir2GetX1(stIir2 *filter)
{
    return (filter != NULL) ? filter->x1 : 0.0f;
}

float iir2GetX2(stIir2 *filter)
{
    return (filter != NULL) ? filter->x2 : 0.0f;
}

float iir2GetY1(stIir2 *filter)
{
    return (filter != NULL) ? filter->y1 : 0.0f;
}

float iir2GetY2(stIir2 *filter)
{
    return (filter != NULL) ? filter->y2 : 0.0f;
}

/* 二阶滤波器运行接口。 */
float iir2Run(stIir2 *filter, float input)
{
    return iir2RunFull(filter, input);
}

float iir2RunB0(stIir2 *filter, float input)
{
    float lOutput;

    if (filter == NULL)
    {
        return input;
    }

    lOutput = (filter->b0 * input) - (filter->a1 * filter->y1) - (filter->a2 * filter->y2);
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = lOutput;

    return lOutput;
}

float iir2RunB1(stIir2 *filter, float input)
{
    float lOutput;

    if (filter == NULL)
    {
        return input;
    }

    lOutput = (filter->b0 * input) + (filter->b1 * filter->x1) - (filter->a1 * filter->y1) - (filter->a2 * filter->y2);
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = lOutput;

    return lOutput;
}

float iir2RunFull(stIir2 *filter, float input)
{
    float lOutput;

    if (filter == NULL)
    {
        return input;
    }

    lOutput = (filter->b0 * input) + (filter->b1 * filter->x1) + (filter->b2 * filter->x2)
           - (filter->a1 * filter->y1) - (filter->a2 * filter->y2);

    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = lOutput;

    return lOutput;
}

/* 二阶滤波器参数写入接口。 */
void iir2SetA1(stIir2 *filter, float a1)
{
    if (filter != NULL)
    {
        filter->a1 = a1;
    }
}

void iir2SetA2(stIir2 *filter, float a2)
{
    if (filter != NULL)
    {
        filter->a2 = a2;
    }
}

void iir2SetB0(stIir2 *filter, float b0)
{
    if (filter != NULL)
    {
        filter->b0 = b0;
    }
}

void iir2SetB1(stIir2 *filter, float b1)
{
    if (filter != NULL)
    {
        filter->b1 = b1;
    }
}

void iir2SetB2(stIir2 *filter, float b2)
{
    if (filter != NULL)
    {
        filter->b2 = b2;
    }
}

void iir2SetX1(stIir2 *filter, float x1)
{
    if (filter != NULL)
    {
        filter->x1 = x1;
    }
}

void iir2SetX2(stIir2 *filter, float x2)
{
    if (filter != NULL)
    {
        filter->x2 = x2;
    }
}

void iir2SetY1(stIir2 *filter, float y1)
{
    if (filter != NULL)
    {
        filter->y1 = y1;
    }
}

void iir2SetY2(stIir2 *filter, float y2)
{
    if (filter != NULL)
    {
        filter->y2 = y2;
    }
}

/* 二阶滤波器批量配置接口。 */
void iir2GetDen(stIir2 *filter, float *pa1, float *pa2)
{
    if (filter == NULL)
    {
        return;
    }

    if (pa1 != NULL)
    {
        *pa1 = filter->a1;
    }

    if (pa2 != NULL)
    {
        *pa2 = filter->a2;
    }
}

void iir2GetState(stIir2 *filter, float *px1, float *px2, float *py1, float *py2)
{
    if (filter == NULL)
    {
        return;
    }

    if (px1 != NULL)
    {
        *px1 = filter->x1;
    }

    if (px2 != NULL)
    {
        *px2 = filter->x2;
    }

    if (py1 != NULL)
    {
        *py1 = filter->y1;
    }

    if (py2 != NULL)
    {
        *py2 = filter->y2;
    }
}

void iir2GetNum(stIir2 *filter, float *pb0, float *pb1, float *pb2)
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

    if (pb2 != NULL)
    {
        *pb2 = filter->b2;
    }
}

void iir2Init(stIir2 *filter, float b0, float b1, float b2, float a1, float a2)
{
    if (filter == NULL)
    {
        return;
    }

    filter->a1 = a1;
    filter->a2 = a2;
    filter->b0 = b0;
    filter->b1 = b1;
    filter->b2 = b2;
    filter->x1 = 0.0f;
    filter->x2 = 0.0f;
    filter->y1 = 0.0f;
    filter->y2 = 0.0f;
}

void iir2SetDen(stIir2 *filter, float a1, float a2)
{
    if (filter == NULL)
    {
        return;
    }

    filter->a1 = a1;
    filter->a2 = a2;
}

void iir2SetState(stIir2 *filter, float x1, float x2, float y1, float y2)
{
    if (filter == NULL)
    {
        return;
    }

    filter->x1 = x1;
    filter->x2 = x2;
    filter->y1 = y1;
    filter->y2 = y2;
}

void iir2SetNum(stIir2 *filter, float b0, float b1, float b2)
{
    if (filter == NULL)
    {
        return;
    }

    filter->b0 = b0;
    filter->b1 = b1;
    filter->b2 = b2;
}

/**************************End of file********************************/
