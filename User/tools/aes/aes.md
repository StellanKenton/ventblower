# AES 模块说明

这是当前目录的权威入口文档。

## 1. 模块定位

`aes` 提供可独立迁移的 AES-128、AES-192、AES-256 分组加解密能力，支持 `ECB` 与 `CBC` 两种模式，并提供 PKCS7 填充辅助函数。

它只负责算法本身，不负责：

- 密钥持久化
- 随机 IV 生成
- 文件、串口、网络或日志输出

## 2. 目录内文件职责

| 文件 | 职责 |
| --- | --- |
| `aes.h` | 状态码、上下文对象、公共 API |
| `aes.c` | 轮密钥展开、分组加解密、PKCS7 辅助实现 |
| `aes.md` | 权威主文档 |

## 3. 对外公共接口

稳定公共头文件：`aes.h`

稳定 API：

- `aesInit()`
- `aesEncrypt()`
- `aesDecrypt()`
- `aesPkcs7Pad()`
- `aesPkcs7Unpad()`

## 4. 配置、状态与生命周期

- `stAesContext` 由调用方持有。
- `aesInit()` 完成模式、轮数、轮密钥和默认 IV 的装配。
- `aesEncrypt()` / `aesDecrypt()` 允许输入输出同缓冲区。
- `CBC` 模式每次调用都从 `context->iv` 重新开始，不会在函数内部回写链式状态。

## 5. 依赖白名单与黑名单

- 仅依赖标准头 `stdint.h`、`string.h`。
- 不依赖 `User/`、`driver/`、`module/`、`sys/`。
- 禁止在本模块内部保存全局密钥、全局 IV 或调试打印。

## 6. 函数指针 / port / assembly 契约表

| 项 | 约束 |
| --- | --- |
| platform hook | 无 |
| port 文件 | 无 |
| assembly 默认绑定 | 无 |

## 7. 公共函数使用契约表

| 函数 | 调用要求 | 失败语义 |
| --- | --- | --- |
| `aesInit()` | `key` 不能为空；`CBC` 模式下 `iv` 不能为空 | 返回 `AES_STATUS_ERROR_PARAM` 或 `AES_STATUS_ERROR_TYPE` |
| `aesEncrypt()` | `dataLen` 必须是 16 的整数倍；支持原地加密 | 返回 `AES_STATUS_ERROR_LENGTH` |
| `aesDecrypt()` | `dataLen` 必须是 16 的整数倍；支持原地解密 | 返回 `AES_STATUS_ERROR_LENGTH` |
| `aesPkcs7Pad()` | 调用方需保证缓冲区有足够尾部空间 | 返回 0 表示失败 |
| `aesPkcs7Unpad()` | 输入必须是 16 字节对齐且尾部填充值合法 | 返回 0 表示失败 |

## 8. 改动落点矩阵

| 需求 | 应改文件 | 不该改的文件 |
| --- | --- | --- |
| 扩充算法模式 | `aes.h`、`aes.c`、`aes.md` | 上层业务代码 |
| 变更密钥来源或 IV 生成策略 | 上层调用方 | `aes.c` |
| 增加日志或调试展示 | 上层测试代码 | `aes.c` |

## 9. 复制到其他工程的最小步骤

最小依赖集：`aes.h`、`aes.c`。

若外部工程需要自动填充流程，应由调用方决定先调用 `aesPkcs7Pad()`，再调用 `aesEncrypt()`。

## 10. 使用示例

```c
stAesContext aes;
uint8_t key[16] = {0};
uint8_t iv[16] = {0};
uint8_t buffer[32] = {0};

aesInit(&aes, AES_TYPE_128, AES_MODE_CBC, key, iv);
aesEncrypt(&aes, buffer, buffer, sizeof(buffer));
aesDecrypt(&aes, buffer, buffer, sizeof(buffer));
```