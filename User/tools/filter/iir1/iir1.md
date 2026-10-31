---
doc_role: tool-spec
layer: tools
module: iir1
status: active
portability: standalone
public_headers:
  - iir1.h
core_files:
  - iir1.c
port_files: []
depends_on: []
copy_minimal_set:
  - iir1.h
  - iir1.c
---

# IIR1 模块说明

`iir1` 提供一阶 IIR 差分滤波与简化的一阶低通滤波，不依赖 RTOS、BSP 或业务模块。

| 文件 | 职责 |
| --- | --- |
| `iir1.h` | 滤波对象与公共 API |
| `iir1.c` | 差分方程和状态推进 |
| `iir1.md` | 模块 contract |

## 基本用法

```c
stIir1 filter;
iir1Init(&filter, b0, b1, a1);
float output = iir1Run(&filter, input);
```

只使用 `b0` 分子项时调用 `iir1RunB0`。批量配置接口为 `iir1SetNum`、`iir1SetDen` 和 `iir1SetState`。

简化的一阶低通接口：

```c
stLpf1 filter;
lpf1Init(&filter, gain);
float output = lpf1Run(&filter, input);
```

滤波对象由调用方持有；系数按固定采样周期设计，采样周期改变后必须重新计算。
