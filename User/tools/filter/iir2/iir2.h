/************************************************************************************
* @file     : iir2.h
* @brief    : 
* @details  : 
* @author   : 
* @date     : 
* @version  : 
* @copyright: Copyright (c) 2050
***********************************************************************************/
/**
 * @file iir2.h
 * @brief 二阶滤波算法对外接口。
 *
 * 该文件定义二阶离散滤波器对象，并提供系数访问、状态访问、
 * 初始化和多种差分方程执行接口。
 */
#ifndef IIR2_H
#define IIR2_H

#ifdef __cplusplus
extern "C" {
#endif

/** 二阶 IIR 滤波器对象。 */
typedef struct {
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;
    float x1;
    float x2;
    float y1;
    float y2;
} stIir2;

/** 读取分母系数 a1。 */
float iir2GetA1(stIir2 *filter);

/** 读取分母系数 a2。 */
float iir2GetA2(stIir2 *filter);

/** 读取分子系数 b0。 */
float iir2GetB0(stIir2 *filter);

/** 读取分子系数 b1。 */
float iir2GetB1(stIir2 *filter);

/** 读取分子系数 b2。 */
float iir2GetB2(stIir2 *filter);

/** 读取上一拍输入 x1。 */
float iir2GetX1(stIir2 *filter);

/** 读取上两拍输入 x2。 */
float iir2GetX2(stIir2 *filter);

/** 读取上一拍输出 y1。 */
float iir2GetY1(stIir2 *filter);

/** 读取上两拍输出 y2。 */
float iir2GetY2(stIir2 *filter);

/** 按完整二阶差分方程执行一次滤波。 */
float iir2Run(stIir2 *filter, float input);

/** 按只含 b0 的形式执行一次滤波。 */
float iir2RunB0(stIir2 *filter, float input);

/** 按含 b0、b1 的形式执行一次滤波。 */
float iir2RunB1(stIir2 *filter, float input);

/** 按含 b0、b1、b2 的完整形式执行滤波。 */
float iir2RunFull(stIir2 *filter, float input);

/** 写入分母系数 a1。 */
void iir2SetA1(stIir2 *filter, float a1);

/** 写入分母系数 a2。 */
void iir2SetA2(stIir2 *filter, float a2);

/** 写入分子系数 b0。 */
void iir2SetB0(stIir2 *filter, float b0);

/** 写入分子系数 b1。 */
void iir2SetB1(stIir2 *filter, float b1);

/** 写入分子系数 b2。 */
void iir2SetB2(stIir2 *filter, float b2);

/** 写入上一拍输入 x1。 */
void iir2SetX1(stIir2 *filter, float x1);

/** 写入上两拍输入 x2。 */
void iir2SetX2(stIir2 *filter, float x2);

/** 写入上一拍输出 y1。 */
void iir2SetY1(stIir2 *filter, float y1);

/** 写入上两拍输出 y2。 */
void iir2SetY2(stIir2 *filter, float y2);

/** 批量读取分母系数。 */
void iir2GetDen(stIir2 *filter, float *pa1, float *pa2);

/** 批量读取二阶滤波器状态。 */
void iir2GetState(stIir2 *filter, float *px1, float *px2, float *py1, float *py2);

/** 批量读取分子系数。 */
void iir2GetNum(stIir2 *filter, float *pb0, float *pb1, float *pb2);

/** 初始化二阶滤波器系数与状态。 */
void iir2Init(stIir2 *filter, float b0, float b1, float b2, float a1, float a2);

/** 批量写入分母系数。 */
void iir2SetDen(stIir2 *filter, float a1, float a2);

/** 批量写入二阶滤波器状态。 */
void iir2SetState(stIir2 *filter, float x1, float x2, float y1, float y2);

/** 批量写入分子系数。 */
void iir2SetNum(stIir2 *filter, float b0, float b1, float b2);

#ifdef __cplusplus
}
#endif

#endif
/**************************End of file********************************/
