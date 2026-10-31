# Filter 目录说明

`filter` 集中存放轻量、无平台依赖的数字滤波模块。

| 子目录 | 说明 | 主文档 |
| --- | --- | --- |
| `butterworth` | Butterworth 低通滤波 | `butterworth/butterworthfilter.md` |
| `iir1` | 一阶 IIR 与简化一阶低通滤波 | `iir1/iir1.md` |
| `iir2` | 二阶 IIR 滤波 | `iir2/iir2.md` |
| `numfilter` | 通用数值滤波与统计工具 | `numfilter/numfilter.md` |

各模块独立持有运行状态，不依赖 RTOS、BSP 或业务模块。参与构建时，应显式加入所需源文件及其 include path。
