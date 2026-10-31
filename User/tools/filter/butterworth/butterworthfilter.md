# butterworthfilter 说明

## 文件概览
`butterworthfilter.h` 与 `butterworthfilter.c` 共同实现二阶 Butterworth 低通滤波器模块。

模块包含：
- `ButterworthFilterObj` 滤波对象定义
- 默认分子系数数组 `CURRENT_LOOP_FILTER_NUM`
- 默认分母系数数组 `CURRENT_LOOP_FILTER_DEN`
- 初始化、复位和更新接口

## 对外接口与实现职责
头文件提供对象定义和公开接口，实现文件提供默认滤波器系数、状态复位和单步更新计算。

实现重点包括：
- 默认滤波器系数定义
- 滤波器对象初始化
- 滤波器状态复位
- 新样本输入后的单步更新计算

## 如何使用
### 1. 定义对象并初始化

```c
ButterworthFilterObj filter;
UnitAlgoButterworthFilterInit(&filter, CURRENT_LOOP_FILTER_NUM, CURRENT_LOOP_FILTER_DEN);
```

### 2. 周期调用更新函数

```c
float y = UnitAlgoButterworthFilterUpdate(input, &filter);
```

### 3. 清零状态
如果切换模式或重新开始采样，可以先复位：

```c
UnitAlgoButterworthFilterReset(&filter);
```

## 适用场景
- 电流环采样值平滑
- 压力、流量等模拟量低通滤波
- 对高频抖动敏感的控制输入预处理

## 注意事项
- `num` 和 `den` 需要指向长度为 3 的系数数组
- 分母系数中默认按 `a0 = 1` 使用
- 如果需要不同截止频率，应替换默认系数数组

## 实现特点
- 内部保存两拍输入和两拍输出
- 适合固定周期任务反复调用
- 复位后状态会回到全零