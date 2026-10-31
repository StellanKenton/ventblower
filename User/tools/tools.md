# Tools 目录说明

`tools` 目录存放项目层可复用的小型工具代码。工具模块应保持轻量，不直接承担业务流程。

| 子目录 | 说明 | 主文档 |
| --- | --- | --- |
| `aes` | AES 加解密工具 | `aes/aes.md` |
| `controller` | PID 等轻量控制算法 | `controller/controller.md` |
| `filter` | 数字滤波工具集合 | `filter/filter.md` |
| `md5` | MD5 摘要工具 | `md5/md5.md` |
| `ringbuffer` | 字节环形缓冲区工具 | `ringbuffer/ringbuffer.md` |

## 依赖规则

- `ringbuffer` 当前由日志模块使用，公共头文件为 `ringbuffer/ringbuffer.h`。
- `controller` 当前提供固定周期浮点 PID，公共头文件为 `controller/pid.h`。
- 参与 Keil 构建时，工具目录需要显式加入工程源文件和 include path。
- 新增工具子目录时，需要在本文件补充用途、主文档和依赖关系。
