/************************************************************************************
* @file     : iir1.h
* @brief    : 
* @details  : 
* @author   : 
* @date     : 
* @version  : 
* @copyright: Copyright (c) 2050
***********************************************************************************/
/**
 * @file iir1.h
 * @brief 一阶滤波算法对外接口。
 *
 * 该文件定义了一阶离散滤波器对象、控制用简化滤波器对象，以及
 * 对应的初始化、系数访问、状态访问和运行接口。
 */
#ifndef IIR1_H
#define IIR1_H

#ifdef __cplusplus
extern "C" {
#endif

/** 一阶 IIR 滤波器对象。 */
typedef struct {
    float a1;
    float b0;
    float b1;
    float x1;
    float y1;
} stIir1;

/** 控制环节使用的一阶简化滤波器对象。 */
typedef struct {
    float gain;
    float x1;
    float y;
    float y1;
} stLpf1;

/** 读取分母系数 a1。 */
float iir1GetA1(stIir1 *filter);


/** 读取分子系数 b0。 */
float iir1GetB0(stIir1 *filter);

/** 读取分子系数 b1。 */
float iir1GetB1(stIir1 *filter);

/** 读取上一拍输入 x1。 */
float iir1GetX1(stIir1 *filter);


/** 读取上一拍输出 y1。 */
float iir1GetY1(stIir1 *filter);

/** 读取分母系数集合。 */
float iir1GetDen(stIir1 *filter);

/** 读取一阶滤波器状态量。 */
void iir1GetState(stIir1 *filter, float *px1, float *py1);

/** 读取分子系数集合。 */
void iir1GetNum(stIir1 *filter, float *pb0, float *pb1);
 
/** 初始化一阶滤波器系数与状态。 */
void iir1Init(stIir1 *filter, float b0, float b1, float a1);

/** 按完整一阶差分方程执行一次滤波。 */
float iir1Run(stIir1 *filter, float input);

/** 按简化一阶差分方程执行一次滤波。 */
float iir1RunB0(stIir1 *filter, float input);

/** 写入分母系数 a1。 */
void iir1SetA1(stIir1 *filter, float a1);

/** 写入分子系数 b0。 */
void iir1SetB0(stIir1 *filter, float b0);

/** 写入分子系数 b1。 */
void iir1SetB1(stIir1 *filter, float b1);

/** 写入上一拍输入 x1。 */
void iir1SetX1(stIir1 *filter, float x1);

/** 写入上一拍输出 y1。 */
void iir1SetY1(stIir1 *filter, float y1);

/** 批量写入分母系数。 */
void iir1SetDen(stIir1 *filter, float a1);

/** 批量写入一阶滤波器状态。 */
void iir1SetState(stIir1 *filter, float x1, float y1);

/** 批量写入分子系数。 */
void iir1SetNum(stIir1 *filter, float b0, float b1);

/** 初始化控制用一阶滤波器。 */
void lpf1Init(stLpf1 *filter, float gain);

/** 更新控制用一阶滤波器增益。 */
void lpf1SetGain(stLpf1 *filter, float gain);

/** 清零控制用一阶滤波器状态。 */
void lpf1Reset(stLpf1 *filter);

/** 执行控制用一阶滤波器更新。 */
float lpf1Run(stLpf1 *filter, float input);


#ifdef __cplusplus
}
#endif

#endif
/**************************End of file********************************/
