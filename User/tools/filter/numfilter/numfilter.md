# numfilter 说明

## 文件概览
`numfilter.h` 与 `numfilter.c` 共同构成一个综合数值算法模块，提供基础换算、插值、滤波、统计和差分计算能力。

模块主要包含：
- 比例换算
- 一维线性插值
- 二维表查找与双线性插值
- 一阶传递函数离散化对象
- 移动平均、去极值平均、锁相平均等滤波器
- 方差、标准差、高效均值和高效方差计算
- 气体工况体积换算
- 物理量归一化和反归一化
- 固定间隔差分计算

## 对外接口与实现职责
头文件负责声明各类算法对象和对外接口，实现文件负责完成具体计算逻辑。

实现重点包括：
- 线性插值辅助函数
- 二维表点查找与双线性插值
- 一阶离散传递函数更新
- 移动平均与去极值平均滤波
- 锁相平均滤波
- 方差与标准差计算
- 高效均值、方差、标准差计算
- 气体工况体积换算
- 归一化与反归一化
- 固定间隔差分计算

## 如何使用
### 1. 按功能选择对象和接口
例如比例换算：

```c
ProportObj prop;
UnitAlgoNumStatProportInit(&prop, k, b);
float y = UnitAlgoNumStatProportCalc(&prop, x);
```

例如移动平均滤波：

```c
float buffer[16];
MovAvgFilterObj filter;
UnitAlgoMovAvgFilterInit(&filter, buffer, 16);
float y = UnitAlgoMovAvgFilterUpdata(&filter, input);
```

例如二维表插值：

```c
float z = UnitAlgoBilinearInterpolatePoints(&table, x, y);
```

例如方差计算：

```c
float dataBuff[16];
VarianceObj var;
UnitAlgoVarianceInit(&var, 16);
float value = UnitAlgoVarianceUpdate(&var, dataBuff, input);
```

例如固定间隔差分：

```c
float diffBuff[10];
DiffCalcObj diff;
UnitAlgoDiffCalcInit(&diff, diffBuff, 10, 3);
float delta = UnitAlgoDiffCalcUpdate(&diff, input);
```

### 2. 使用前先完成对象初始化
本模块多数接口依赖对象内部状态，因此正式计算前必须先调用对应 `Init` 函数。

### 3. 周期性更新
滤波和统计类接口通常应在固定采样周期内重复调用，这样结果才稳定且有意义。

## 使用建议
- 缓冲区长度不要为 0
- 周期类算法尽量在固定采样周期中调用
- 对于体积换算，确保温度和压力参数使用一致单位

## 建议阅读顺序
如果第一次接触本模块，建议按以下顺序查看：
1. 比例换算与插值
2. 平均滤波接口
3. 方差和标准差接口
4. 气体工况换算与差分接口