---
doc_role: tool-spec
layer: tools
module: jsonparser
status: active
portability: standalone
public_headers:
  - jsonparser.h
core_files:
  - jsonparser.c
port_files: []
debug_files: []
depends_on: []
forbidden_depends_on:
  - 文件系统对象
  - RTOS 或 BSP
  - 动态内存
required_hooks: []
optional_hooks: []
common_utils: []
copy_minimal_set:
  - jsonparser.h
  - jsonparser.c
read_next: []
---

# JSON Parser 模块说明

这是当前目录的权威入口文档。

## 1. 模块定位

`jsonparser` 是面向 MCU 的轻量 JSON 字段提取工具，用于从 HTTP 返回文本中按 key 获取字符串或整数值。

该模块不构建 DOM，不申请动态内存，不保存解析状态；调用方提供完整 JSON 文本和输出缓冲区。

## 2. 目录内文件职责

| 文件 | 职责 |
| --- | --- |
| `jsonparser.h` | 状态码与公共解析接口 |
| `jsonparser.c` | JSON 字符串、数字与 key 查找实现 |
| `jsonparser.md` | 当前目录 contract |

## 3. 对外公共接口

稳定公共头文件：`jsonparser.h`

稳定 API：

- `jsonParserFindString()`
- `jsonParserFindInt()`

## 4. 配置、状态与生命周期

- 所有 API 都是一次性解析，不持有外部指针。
- `json` 可以不是 `\0` 结尾，长度由 `jsonLen` 指定。
- 字符串输出会处理常见反斜杠转义并补 `\0`。

## 5. 依赖白名单与黑名单

- 仅依赖标准头 `stdbool.h`、`stdint.h`、`string.h`。
- 禁止依赖文件系统、RTOS、日志、业务层或动态内存。

## 6. 函数指针 / port / assembly 契约表

| 项 | 约束 |
| --- | --- |
| platform hook | 无 |
| port 文件 | 无 |
| assembly 默认绑定 | 无 |

## 7. 公共函数使用契约表

| 函数 | 调用要求 | 失败语义 |
| --- | --- | --- |
| `jsonParserFindString()` | `json/key/output` 不能为空，`outputSize > 0` | 未找到返回 `JSON_PARSER_STATUS_NOT_FOUND`，格式错误返回 `JSON_PARSER_STATUS_ERROR_FORMAT` |
| `jsonParserFindInt()` | `json/key/value` 不能为空 | 未找到返回 `JSON_PARSER_STATUS_NOT_FOUND`，格式错误返回 `JSON_PARSER_STATUS_ERROR_FORMAT` |

## 8. 改动落点矩阵

| 需求 | 应改文件 | 不该改的文件 |
| --- | --- | --- |
| 增加新值类型 | `jsonparser.h`、`jsonparser.c`、`jsonparser.md` | 业务 manager |
| 增加平台字段名 | 调用方业务代码 | `jsonparser.c` |

## 9. 复制到其他工程的最小步骤

最小依赖集：`jsonparser.h`、`jsonparser.c`。

## 10. 使用示例

```c
char token[64];

if (jsonParserFindString(response, responseLen, "token", token, sizeof(token), NULL) == JSON_PARSER_STATUS_OK) {
    /* use token */
}
```