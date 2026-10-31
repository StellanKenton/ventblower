# User 层

ventblower 是从 ventcore 拆出的风机（blower）控制工程，保留 GD32F470 MCU 支持、板级驱动和通用模块，不包含通气逻辑、报警、协议和校准等呼吸机业务代码。

| 路径 | 职责 |
|---|---|
| `main.c` | 初始化日志和控制台、初始化 ADC/阀门、注册 worker tasks 并启动调度器 |
| `gd32f4xx_libopt.h` | GD32F4xx 标准外设库头文件选择 |
| `app/system/taskmanager.*` | 通过 `WorkerTasksRegister()` 创建 defaultTask、SensorTask、SysTask；全部任务使用绝对周期 `DelayUntil`，defaultTask 每 10 ms 处理日志，SensorTask 每 3 ms 采集 SFM3119 并处理风机反馈与控制保活 |
| `bsp/uart/uart.*` | USART0 PA9 TX / PA10 RX，115200、8N1；中断收发，2048 字节 RX 缓存；公开 API 仅供任务调用 |
| `bsp/adc/adc.*` | 使用 ADC1 规则组扫描、连续转换和 DMA1 循环模式持续采集 14 路板级模拟量 |
| `bsp/blower_vcm/blower_vcm.*` | 使用 UART4（板级 VCM UART5，PC12/PD2）和 DMA0 异步发送双控制帧、循环接收反馈；控制变化时立即发送并每 10 ms 保活重发，提供连接超时与通信统计 |
| `bsp/bspdebug.*` | 注册 `bsp` RTT 调试命令；支持 ADC、阀门、风机控制，以及 `bsp blower stats` 通信诊断 |
| `bsp/dvalve/dvalve.*` | 以枚举选择氧气阀、泄压阀或呼气阀，提供统一的 20 kHz、0～100% PWM 占空比控制接口 |
| `bsp/eeprom/m24512r.*` | 使用 PD10/PD11 软件 I2C 访问 M24512-R，提供跨页写入和任意字节读取 |
| `bsp/sf06sdk/sfm3119.*` | 管理两只 SFM3119；空气通道使用 PB6/PB7、200 kHz 硬件 I2C0 和 DMA 异步接收，氧气通道使用 PA12/PA11 优化模拟 I2C；启动后等待首个测量结果，每 2 ms 更新流量，每 100 个周期（200 ms）更新温度与状态，并缓存产品 ID 与序列号 |
| `bsp/valve/valve.*` | 初始化 4 路零点阀控制输出和状态反馈输入，并提供按阀门枚举访问的接口 |
| `module/rtos/rtos.*` | 项目层任务、调度、固定周期 `DelayUntil`、tick 和临界区接口 |
| `module/rtos/portrtos.*` | FreeRTOS 原生接口绑定 |
| `tools/controller/` | 轻量控制算法；当前提供带输出限幅和积分抗饱和的固定周期浮点 PID |
| `tools/ringbuffer/` | 日志输出使用的轻量级字节环形缓冲区 |
| `FreeRTOSConfig.h` | FreeRTOS 工程配置 |
| `driver/aiic/` | 软件 I2C 驱动（当前未参与编译） |
| `module/log/` | RTT 日志、ringbuffer 输出队列和控制台命令；`consoleRegisted/sysdebug.*` 提供系统调试命令 |
| `develop/` | VS Code Device Tool 的 CMake 构建、烧录、复位与 RTT 工具，见 `develop/README.md` |

`bsp blower <speed|pwm> <value>` 直接调用 `blowerVcmSendControl()` 下发控制，SensorTask 中的 `blowerVcmProcess()` 负责保活重发；`bsp dvalve exp <0-100>` 直接设置呼气阀占空比。

项目代码只能通过 `rtos.h` 使用任务、调度、tick 和临界区能力；FreeRTOS 原生 API 仅允许出现在 `portrtos.c`。日志统一使用 `LOG_I`、`LOG_W`、`LOG_E` 等宏，不能直接使用标准库输出函数。

`module/rtos/portrtos.c` 的毫秒转 tick 在乘法前提升到 64 位，避免默认 `pdMS_TO_TICKS` 在 1000 Hz 下运行约 71 分 35 秒后溢出。
