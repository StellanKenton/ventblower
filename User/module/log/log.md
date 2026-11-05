---
port_files:
  - User/module/log/portlog.h
  - User/module/log/portlog.c
---

# log 模块说明

这是当前目录的权威入口文档。

## 1. 模块定位

提供日志输出（`LOG_E/W/I/D/R`）和控制台命令输入。每行日志在调用者栈上格式化一次，然后写给每条启用的链路（目前只有 RTT）；console 只从一条链路读命令。

- core：`log.*`、`console.*`，与具体链路无关，ventblower 和 ventcore 两个仓库保持一致。
- port：`portlog.*`，负责链路绑定和配置，每个工程一份。
- 项目自己的命令不放在本目录：sysdebug 在 `User/app/system/`，bspdebug 在 `User/bsp/`。

## 2. 目录内文件职责

| 文件 | 层 | 职责 |
|---|---|---|
| `log.h` / `log.c` | core | 级别宏、链路契约 `stLogPortOps`、格式化与多链路写出、丢行统计 |
| `console.h` / `console.c` | core | 行编辑、命令注册与分发、`consoleDrain` |
| `portlog.h` | port | 编译期级别、行缓冲尺寸、链路开关、console 链路与回显 |
| `portlog.c` | port | 链路表 `gLogPortLinks[]`、RTT 链路、时间戳、ISR 判断 |
| `SEGGER/` | 厂商 | RTT 原样代码，只编译 `SEGGER_RTT.c` |

## 3. 对外接口与调用上下文

| 接口 | 任务 | 调度器启动前 | ISR | 说明 |
|---|---|---|---|---|
| `logInit()` | — | ✓ | ✗ | main 开头调用一次，依次调用各链路的 `init` |
| `LOG_E/W/I/D` | ✓ | ✓ | 丢弃并计数 | 前缀 `[L][ms][tag]`，受 `LOG_COMPILED_LEVEL` 裁剪；单行上限 `LOG_LINE_MAX_SIZE`（含 CRLF） |
| `LOG_R` | ✓ | ✓ | 丢弃并计数 | 无前缀，不受等级裁剪；上限 `LOG_RAW_LINE_MAX_SIZE`；用于测试数据等原始行 |
| `logShowStats()` | ✓ | ✓ | ✗ | 打印每条链路的丢行数和 ISR 丢弃数，即 sysdebug 的 `log` 命令 |
| `consoleRegisterCommand()` | ✓ | ✓ | ✗ | 返回 `CONSOLE_STATUS_*`；命令描述符必须一直有效 |
| `consoleProcess()` | 单一任务 | — | ✗ | 读 `CONSOLE_INPUT_LINK` 并执行完整的命令行；当前由 defaultTask 每 10 ms 调用 |
| `consoleShowHelp()` | ✓ | ✓ | ✗ | |
| `consoleDrain(maxPending, timeoutMs)` | 低优先级任务 | ✗（tick 不走） | ✗ | 忙等 console 链路积压降到 `maxPending` 以下；reboot 前传 0，批量输出每行前传约 1024 |

- 栈：`logWrite` 在调用者栈上占 `LOG_LINE_MAX_SIZE`（256 B），`logRawWrite` 占 `LOG_RAW_LINE_MAX_SIZE`（384 B），另加 vsnprintf 自身的开销。`LOG_R` 只在栈充足的任务里使用（console 任务、main）。
- ISR：core 入口检查 IPSR，ISR 里的调用直接丢弃并计数，链路代码永远不会在 ISR 中运行。需要在 ISR 里做诊断时，先存到 RAM，再在任务里打印。FreeRTOS 栈溢出钩子在 PendSV 中执行，同样不能打日志。

## 4. 链路契约（`stLogPortOps` 必填字段检查表）

| 成员 | 必填 | 约定 |
|---|---|---|
| `name` | 是 | 短名，`log` 命令显示用 |
| `init` | 否 | `logInit()` 时调用一次 |
| `write` | 是 | 永不阻塞；整行全部收下返回 length，否则返回 0 且一个字节都不写；多任务并发调用时由链路自己保证原子性；只在线程模式下被调用，可以使用 `repRtosEnterCritical()` |
| `read` | 否 | 不阻塞，返回读到的字节数（不超过 length）；NULL 表示这条链路只输出 |
| `pending` | 否 | 已收下但还没发出的字节数；NULL 视为 0 |

## 5. 分发规则

- 输出：每行只格式化一次，按 linkId 从小到大写给所有启用的链路。某条链路返回 0 时，只记它自己的丢行数，不重试、不阻塞，也不影响其它链路。
- 输入：只读 `CONSOLE_INPUT_LINK`；如果它指向未启用或越界的链路，`portlog.h` 会在编译时报错。
- 未知或未启用的 linkId：`logPortGetOps()` 返回 NULL，core 直接跳过。
- 多个 linkId 可以共享同一组静态函数，但不能共享链路缓冲。ops 表一律用指定初始化器；以后新增字段时，0 值必须等于现有行为，这样已有链路不用改。
- 回显（`CONSOLE_ECHO_ENABLE=1`）只写回 console 链路；日志和命令输出写给所有链路。

## 6. 配置（`portlog.h`）

| 宏 | 默认 | 说明 |
|---|---|---|
| `LOG_COMPILED_LEVEL` | 4 | 0 全关，1 E，2 W，3 I，4 D；被裁掉的调用连参数都不会求值 |
| `LOG_LINE_MAX_SIZE` | 256U | 带前缀的行，含 CRLF |
| `LOG_RAW_LINE_MAX_SIZE` | 384U | `LOG_R` 行，含 CRLF；需要容纳 ventcore 300 字符的 CSV 表头 |
| `LOG_LINK_RTT_ENABLE` | 1 | |
| `LOG_LINK_UART_ENABLE` | 0 | 开启前先看第 8 节 |
| `LOG_CONSOLE_ENABLE` | 1 | 为 0 时 main 不注册命令，defaultTask 不调用 `consoleProcess` |
| `CONSOLE_INPUT_LINK` | `LOG_LINK_RTT` | |
| `CONSOLE_ECHO_ENABLE` | 0 | 人工在串口终端上输入时设为 1 |

Device Tool 构建时只传固定的几个 `-D`，所以工程之间的差异直接改 `portlog.h`。这些宏带 `#ifndef`，只是为了让主机端测试可以覆盖。

## 7. 改动落点矩阵

| 需求 | 应改文件 | 不该改的文件 |
|---|---|---|
| 调日志等级、行长 | `portlog.h` | core |
| 切换链路、改 console 链路或回显 | `portlog.h` | core |
| 新增 UART、USB CDC、CAN 等链路 | `portlog.h`（`LOG_LINK_xxx` 编号、`LOG_LINK_COUNT`、开关、`#error` 检查）、`portlog.c`（ops 和链路表）、对应 BSP | core |
| 改时间戳来源（裸机工程） | `portlog.c` 的 `logPortGetTimeMs()` | core |
| 新增项目命令 | 项目自己的文件，用 `consoleRegisterCommand()` 注册 | 本目录 |
| 改行格式、丢弃策略、命令解析 | `log.c` / `console.c`，两个仓库同步修改 | — |

## 8. 新增 UART 链路的最小骨架

前置条件：

- 专用 USART：不能和二进制协议共用（ventcore 的 USART0 跑的是 MCM 协议）。
- BSP 要提供非阻塞的流式写，一行要么全部收下、要么一个字节都不写：TX 环形缓冲由 TBE 中断接着发，建议不小于 2 KB。现有的 `uartSendData` 是一次性发 256 B，忙时返回 BUSY，而且只支持 instance 0，不能直接用。
- 带宽：115200 约 11.5 KB/s，ventcore 一次 `vt status`（约 29 KB）要 2.5 s。测试数据要走串口时用 921600，并在每行输出前调用 `consoleDrain(1024U, 200U)`。
- 调度器启动前写入的日志会留在 TX 缓冲里：FreeRTOS 在启动前进过临界区后，会一直屏蔽优先级数值 ≥ 5 的中断（USART0 是 6），调度器启动后才会发出。启动阶段的问题仍然看 RTT。
- 主机端：ventcore 的 test 脚本共用 `Rtt` 类，GUI 用 `RttSession`；切到串口时各加一个接口相同的串口版本。

下面的 `uartStream*` 是 BSP 待新增接口的示意名称：

```c
#if LOG_LINK_UART_ENABLE
static void logPortUartInit(void)
{
    uartStreamInit(LOG_UART_INSTANCE);
}

static uint32_t logPortUartWrite(const uint8_t *data, uint32_t length)
{
    return (uartStreamWrite(LOG_UART_INSTANCE, data, length) == UART_STATUS_OK) ? length : 0U;
}

static uint32_t logPortUartRead(uint8_t *data, uint32_t length)
{
    return uartStreamRead(LOG_UART_INSTANCE, data, length);
}

static uint32_t logPortUartPending(void)
{
    return uartStreamPending(LOG_UART_INSTANCE);
}

static const stLogPortOps gLogPortUartOps = {
    .name = "uart",
    .init = logPortUartInit,
    .write = logPortUartWrite,
    .read = logPortUartRead,
    .pending = logPortUartPending,
};
#endif
```

`LOG_UART_INSTANCE` 定义在 `portlog.h`。链路表里已经预留了 `[LOG_LINK_UART] = &gLogPortUartOps`。

## 9. 复制到其他工程的最小步骤

1. 拷贝整个目录，core 文件不改。
2. 按目标工程修改 `portlog.h`（开关、尺寸）和 `portlog.c`（链路、时间戳、ISR 判断）。
3. CMake 加入 `log.c`、`console.c`、`portlog.c`、`SEGGER/SEGGER_RTT.c`，include 目录加 `User/module/log` 和 `User/module/log/SEGGER`。
4. main 开头调用 `logInit()`；由一个低优先级任务周期调用 `consoleProcess()`。

## 10. 验证清单

- 主机：`python3 User/develop/test_log.py`，覆盖行格式、截断、整行丢弃、ISR 拦截、console 解析、等级裁剪和回显。
- 上板：`help`、`time`、`log`、`bsp adc`、`reboot` 都正常，`reboot` 前的最后一行能看到；粘贴 256 个 `a` 加 ` reboot`，只提示 too long，不会复位；`log` 显示的丢行数为 0。
- 改了行缓冲或链路之后，用 Ozone 的 FreeRTOS 视图确认各任务的栈余量。
