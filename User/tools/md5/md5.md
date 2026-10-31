# MD5 模块说明

这是当前目录的权威入口文档。

## 1. 模块定位

`md5` 提供标准 MD5 摘要计算能力，包含：

- 增量式 `init/update/final`
- 一次性字符串和二进制摘要计算
- 16 字节摘要与 16/32 位十六进制字符串之间的转换

该模块只负责摘要算法与格式转换，不负责文件读取、存储或日志输出。

## 2. 目录内文件职责

| 文件 | 职责 |
| --- | --- |
| `md5.h` | 上下文对象、状态码、公共 API |
| `md5.c` | MD5 核心轮函数、增量更新、字符串转换实现 |
| `md5.md` | 权威主文档 |

## 3. 对外公共接口

稳定公共头文件：`md5.h`

稳定 API：

- `md5Init()` / `md5Update()` / `md5Final()`
- `md5CalcString()` / `md5CalcData()`
- `md5DigestToHex32()` / `md5DigestToHex16()`
- `md5StringToHex32()` / `md5StringToHex16()`
- `md5HexToDigest()`

## 4. 配置、状态与生命周期

- `stMd5Context` 由调用方持有。
- 增量流程必须按 `init -> update -> final` 顺序调用。
- `md5CalcData()` 支持 `length == 0` 的空数据摘要。

## 5. 依赖白名单与黑名单

- 仅依赖标准头 `stdint.h`、`string.h`。
- 不依赖文件系统、动态内存、RTOS 或业务层。
- 禁止在公共实现里引入 `sprintf`、`printf` 等格式化输出依赖。

## 6. 函数指针 / port / assembly 契约表

| 项 | 约束 |
| --- | --- |
| platform hook | 无 |
| port 文件 | 无 |
| assembly 默认绑定 | 无 |

## 7. 公共函数使用契约表

| 函数 | 调用要求 | 失败语义 |
| --- | --- | --- |
| `md5Init()` | `context` 不能为空 | 空操作返回 |
| `md5Update()` | `input == NULL` 只允许在 `inputLen == 0` 时出现 | 空操作返回 |
| `md5Final()` | `digest` 不能为空 | 空操作返回 |
| `md5CalcString()` | 输入必须是以 `\0` 结尾的字符串 | 返回 `MD5_STATUS_ERROR_PARAM` |
| `md5CalcData()` | `digest` 必须有效 | 返回 `MD5_STATUS_ERROR_PARAM` |
| `md5HexToDigest()` | 输入必须是 32 位十六进制字符串 | 返回 `MD5_STATUS_ERROR_FORMAT` |

## 8. 改动落点矩阵

| 需求 | 应改文件 | 不该改的文件 |
| --- | --- | --- |
| 增加摘要输出格式 | `md5.h`、`md5.c`、`md5.md` | 上层业务逻辑 |
| 新增文件摘要流程 | 上层封装文件读取逻辑 | `md5.c` |
| 增加调试打印 | 测试代码 | `md5.c` |

## 9. 复制到其他工程的最小步骤

最小依赖集：`md5.h`、`md5.c`。

如果目标工程只需要一次性摘要，可以只使用 `md5CalcString()` 或 `md5CalcData()`，不需要额外 port 文件。

## 10. 使用示例

```c
uint8_t digest[MD5_DIGEST_SIZE];
char hex32[MD5_HEX32_SIZE];

md5CalcString("hello", digest);
md5DigestToHex32(digest, hex32, 1U);
```