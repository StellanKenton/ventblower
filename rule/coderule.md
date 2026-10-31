# 规则

## 1. 环境与文本约束

- 环境：Visual Studio Code + Windows 11。
- 代码注释使用英文；新增 md 默认使用中文。
- 文本编码统一为 UTF-8，缩进统一为 4 个空格，换行统一为 LF。
- 每个文本文件以且仅以一个换行符结尾。
- 文件名和目录名保持小写。

## 2. 通用工程原则

- 在引入新模式前，先遵循现有局部主流实现。
- 除非任务明确要求，否则不要把重构与功能变更混在一起。
- 非平凡修改都要考虑错误处理、日志和可测试性。
- 硬规则：新增 `typedef` 结构体、`typedef enum`、函数指针 `typedef`、类型别名和宏定义必须放在 `.h` 中，不能放在 `.c` 中。
- 上一条没有例外：即使该类型只在单个 `.c` 文件内部使用，也不能写在 `.c` 中，必须放在对应 `.h` 中。
- 新增文件时先阅读 `example/` 下的同类示例；如果新增的是项目绑定内容，默认放进 `example/`，不要再在 `rep/ 顶层新增 `manager/`、`system/` 一类目录。
- 新增 `.c`、`.h` 文件时，默认按 `newfile/` 下同类示例补齐统一文件头和文件尾，不能省略。
- 新增函数时，默认按同目录主流实现补齐统一注释风格，不能省略；如果当前目录没有同类函数，则回退到 `example/` 下的同类函数示例。
- 提交前自检：如果本次改动新增了宏、`typedef`、枚举别名、函数指针类型，必须逐项确认它们位于 `.h` 文件；出现在 `.c` 文件即视为违规。
- 日志的输出统一使用 `LOG_I`、`LOG_W`、`LOG_E` 等宏，不能直接调用底层日志函数或自己实现日志输出。
- 代码中禁止直接使用 `printf`、`puts`、`putchar` 等标准库函数输出日志或调试信息。
- 当新增功能时，除非明确要求，否则不要更改rep文件夹下的内容，尽量更改其他文件来适配rep文件夹下的内容。

## 2.1 新增 C/H 文件模板约束

- 默认模板来源：`newfile/example.c`、`newfile/example.h`。
- 新增 `.c` 文件时，必须包含统一注释文件头、正文实现和 `End of file` 文件尾注释。
- 新增 `.h` 文件时，必须包含统一注释文件头、include guard、`extern "C"` 骨架（如适用）和 `End of file` 文件尾注释。
- 文件头中的 `@file` 必须与当前文件名一致；其余字段按实际信息填写，未知项可暂留空，但头部结构不能删。
- 文件尾统一保留示例中的结束注释风格，不要改成其他样式。
- 若当前目录已有更具体的同类模板，则优先复用该目录主流模板；若没有，则回退到 `newfile/`。

## 3. 命名规则

- C 标识符默认使用 camelCase。
- 全局变量和文件作用域静态变量使用 `g` 前缀。
- 临时局部变量使用 `l` 前缀。
- 结构体类型使用 `st` 前缀，枚举类型使用 `e` 前缀。
- 函数名尽量以前缀模块名开头，例如 `drvSpiInit`、`mpu6050Init`、`frmProcProcess`。
- 函数指针类型使用 `pf` 前缀，例如 `pfDrvSpiTransfer`、`pfFrmProcProcess`。
- 缩写可以使用，但必须保持可读性与唯一性。

## 4. 文档命名与术语规则

- 目录主文档尽量与目录同名，例如 `drvuart.md`、`frameprocess.md`。
- 补充文档可以使用 `architecture.md`、`migration.md`、`plan.md` 等名称，但不能与主文档竞争权威性。
- 文档中统一使用下面术语：
	- `core`：稳定语义层。
	- `port` 或 `platform hook`：项目绑定层或注入点。
	- `assembly`：装配期配置、默认 linkId / bus / transport 绑定。
	- `debug`：可裁剪的调试与 console 能力。
推荐命名：
- adapter：`xxxHardIicReadRegAdpt`、`xxxSpiTransferAdpt`
- hook：`xxxPlatformDelayMs`、`xxxLoadPlatformDefaultCfg`
- assembly 配置：`stXxxAssembleCfg`
- provider：`xxxGetPlatformInterface`、`xxxLoadPlatformDefaultCfg`
不推荐命名：
- 含义模糊的 `doXxx`、`commonFunc`、`tmpHook`
- 把具体 MCU 或引脚名直接塞进 core 公共 API

## 5. 代码结构规则

- 每个文件保持单一职责。
- 头文件只暴露最小必要接口，内部辅助函数保持为 `static`。
- 仅包含实际使用的头文件。
- 修改驱动或模块前先读对应父目录总文档。

## 6. C 规则

### 6.1 风格与语言使用

- 大括号使用同行风格。
- 优先写可移植 C 代码，避免无必要的编译器扩展。
- 尽量使用 `const`、定宽整数类型和显式初始化。
- `NULL` 仅用于空指针。

### 6.2 函数与参数

- 函数保持简短、单一职责。
- 模块边界必须校验指针、长度和枚举值。
- 失败路径返回明确状态码，不允许静默吞错。
- 错误码返回值统一直接使用 `int8_t`，不要新增或保留 `eXxxStatus`、`typedef enum XxxStatus`、`typedef uint8_t XxxStatus` 这类状态类型别名。
- `int8_t` 状态码约定：`1` 表示成功，负数表示故障；各模块只在自己的 `.h` 中用宏声明错误原因，调用链需要透传时直接返回底层状态码，不做无意义转换。
- 函数声明、定义和函数指针类型的参数列表默认单行书写。

### 6.3 并发与中断

- ISR 只做短小、有界、非阻塞动作。
- 跨上下文共享数据必须明确 ownership、更新顺序和临界区规则。
- 文档中要写明哪些 API 允许在任务、ISR 或两者中调用。
- 项目层和复用层代码禁止直接调用原生 RTOS API；`OSTime*`、`OSTask*`、`OSSem*`、`OSMutex*`、`OSQ*`、`OSFlag*`、`OSStatInit` 等接口只允许出现在 `net/user/module/rtos/portRtos.*` 这类端口绑定层。
- 需要延时、tick、任务创建、互斥、队列、统计初始化等能力时，统一通过 `rep/sys/rtos/rtos.h` 暴露的 `repRtos*` 接口访问，不能在 `User/system`、`User/manager`、`User/bsp` 或 `rep/` 业务代码中直接 include vendor RTOS 头并调用原生函数。

## 7. 文档写作风格

- 主文档优先写 contract，不写散文式计划。
- 表格优先于大段散文，尤其是 hook 和公共函数使用规则。
- “改哪里”要直接写成矩阵，不要让维护者自己猜。
- 文档中的文件职责描述必须与当前目录真实结构一致，不能引用不存在的 `_port.*` 路径。
- 项目示例文档必须显式写清自己位于 `example/` 下，避免把示例层误写成仓库顶层公共层。
- 只要目录真实存在 `port` / provider / platform binding 文件，主文档必须在 front matter 的 `port_files` 中列出真实路径，不能留空或只在正文顺手提及。
- 只要目录需要维护者自己补 `port`，主文档必须给出最小 `port.c` 骨架，至少覆盖静态 ops/provider 表、公开获取入口、未知配置失败语义。
- 只要目录存在配置装配函数，例如 `loadDefaultCfg`、`loadDefaultProtoCfg`、`getOps`、`getPlatformInterface`，主文档必须给出一张“必填字段 / 必填成员”检查表，避免维护者靠源码反推。
- 只要目录支持多协议、多设备、多 linkId 或多映射，主文档必须给出分发规则，明确未知 id 的处理方式、是否允许共享 ops/provider、哪些分支可以复用同一组静态函数。

## 8. Brace Style: Same line
```c
void func() {
}
if (x) {
}
```
