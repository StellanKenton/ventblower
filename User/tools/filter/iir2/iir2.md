---
doc_role: tool-spec
layer: tools
module: iir2
status: active
portability: standalone
public_headers:
  - iir2.h
core_files:
  - iir2.c
port_files: []
depends_on: []
copy_minimal_set:
  - iir2.h
  - iir2.c
---

# IIR2 模块说明

`iir2` 提供二阶 IIR 差分滤波，不依赖 RTOS、BSP 或业务模块。

| 文件 | 职责 |
| --- | --- |
| `iir2.h` | 滤波对象与公共 API |
| `iir2.c` | 差分方程和状态推进 |
| `iir2.md` | 模块 contract |

## 基本用法

```c
stIir2 filter;
iir2Init(&filter, b0, b1, b2, a1, a2);
float output = iir2Run(&filter, input);
```

`iir2Run` 使用完整二阶方程；裁剪分子项时可使用 `iir2RunB0` 或 `iir2RunB1`。批量配置接口为 `iir2SetNum`、`iir2SetDen` 和 `iir2SetState`。

滤波对象由调用方持有；系数按固定采样周期设计，采样周期改变后必须重新计算。
