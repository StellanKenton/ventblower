# User 层

ventblower 是从 ventcore 拆出的风机（blower）控制工程，保留 GD32F470 MCU 支持、板级驱动和通用模块，不包含通气逻辑、报警、协议和校准等呼吸机业务代码。

| 路径 | 职责 |
|---|---|
| `main.c` | 初始化日志、注册控制台命令、初始化 ADC/PWM/霍尔、注册 worker tasks 并启动调度器 |
| `gd32f4xx_libopt.h` | GD32F4xx 标准外设库头文件选择 |
| `app/system/taskmanager.*` | 通过 `WorkerTasksRegister()` 创建 defaultTask、CommTask、SysTask、MotorTask、ProtectTask；全部任务使用绝对周期 `DelayUntil`，defaultTask 每 10 ms 调用 `consoleProcess()` 处理控制台输入，CommTask 每 5 ms 接收 Core 控制帧并回传转速反馈；MotorTask（优先级 22，1 ms）预留给速度环与电机状态机，ProtectTask（优先级 23，2 ms）预留给 VBUS/NTC/堵转/通信超时保护，目前均为空循环；FOC 电流环仍在 ADC 中断中执行；`taskManagerShowStats()` 打印各任务栈和堆的最低余量 |
| `app/system/sysdebug.*` | 注册 `reboot`、`time`、`log`、`task`、`help` 控制台命令；`task` 打印各任务栈最小余量和堆最小余量；`reboot` 先用 `consoleDrain()` 等最后几行发完再复位，`log` 打印各日志链路的丢行数 |
| `bsp/uart/uart.*` | USART0 PA9 TX / PA10 RX，115200、8N1；中断收发，2048 字节 RX 缓存；公开 API 仅供任务调用 |
| `bsp/adc/adc.*` | 原始值统一写入 `gAdcRaw[eAdcChannel]`：三相电流 `ADC_CH_IU/IV/IW` = PC5/PB0/PB1（IN15/IN8/IN9，只接 ADC0/ADC1），每个 PWM 周期由 ADC0、ADC1 注入组在 TIMER0 CH3 触发下同时采其中两相，未采相的槽位保留旧值；母线电压 `ADC_CH_VBUS`（PA5，IN5）和 NTC `ADC_CH_NTC`（PC3，IN13）走 ADC1 规则组连续扫描 + DMA1 循环搬运；在 `ADC_IRQHandler`（优先级 1，高于 RTOS 可屏蔽阈值，禁止调用 RTOS API）中调用 `adcCurrentCallbackSet()` 注册的电流环钩子，钩子入参是本周期未采的相，返回值是下一周期要跳过的相 |
| `bsp/pwm/pwm.*` | TIMER0 三相互补 PWM：上桥 PE9/PE11/PE13、下桥 PE8/PE10/PE12（AF1），20 kHz 中心对齐、283 ns 死区、高有效、空闲全低；CH3 在计数峰值触发电流采样；`pwmInit()` 后计数器运行但主输出关闭，`pwmOutputSet()` 开关六路门极，`pwmDutySet()` 写比较值（0～`PWM_PERIOD`）；调试暂停时 TIMER0 停止；PE15 由 `pwmPowerSet()` 控制风机 28 V 使能，上电默认关闭 |
| `bsp/hall/hall.*` | TIMER3 霍尔接口：A/B/C = PD12/PD13/PD14（AF2，上拉，约 2 µs 滤波）；每个霍尔边沿捕获边沿间隔并重启计数（1 µs/tick），65.5 ms 无边沿判定堵转；`hallStateGet()` 读三位状态，`hallPeriodGet()` 读边沿间隔（0 = 堵转/未测得），`hallElapsedGet()` 读距上次边沿时间供角度插值；TIMER3 中断优先级 2 |
| `bsp/blower_vcm/blower_vcm.*` | 风机侧（从机）：UART4（板级 VCM UART5，PC12/PD2）230400 8N1，DMA0 循环接收 Core 的控制帧（`0x01`：模式、目标值、VCM 饱和），每 10 ms 经 DMA 发送一帧转速反馈（`0x02`：0.1 rps 转速、饱和标志）；100 ms 未收到有效控制帧即判定断连，由上层停机；提供通信统计。与 ventcore 的主机侧同名模块角色相反，两边各自维护 |
| `bsp/bspdebug.*` | 注册 `bsp` 控制台调试命令；支持 ADC 原始值（三相电流/VBUS/NTC），`bsp blower stats` 查看 Core 控制与通信诊断，`bsp blower speed` 手动设置回传转速 |
| `bsp/eeprom/m24512r.*` | 使用 PD10/PD11 软件 I2C 访问 M24512-R，提供跨页写入和任意字节读取 |
| `module/rtos/rtos.*` | 项目层任务、调度、固定周期 `DelayUntil`、tick、临界区接口，以及栈/堆最低余量查询 |
| `module/rtos/portrtos.*` | FreeRTOS 原生接口绑定；栈溢出钩子把任务名记到 `gPortRtosOverflowTaskName` 后关中断停机，用调试器查看 |
| `tools/controller/` | 轻量控制算法；当前提供带输出限幅和积分抗饱和的固定周期浮点 PID |
| `tools/ringbuffer/` | 轻量级字节环形缓冲区（日志不再使用，当前未参与编译） |
| `FreeRTOSConfig.h` | FreeRTOS 工程配置；栈溢出检查方式 2 |
| `driver/aiic/` | 软件 I2C 驱动（当前未参与编译） |
| `driver/current/drvCurrent.*` | 相电流监测：注册 ADC 电流回调，逐周期轮换跳过的相，上电自动做零偏标定（丢弃 200 个周期后每相平均 1024 个样本，约 87 ms），把原始值换算成安培（8.057 mA/count，流向电机为正），未采相按三相电流和为零算出，跟踪各相峰值，任一相单次超过 `DRV_CURRENT_TRIP_A` 就锁存过流；只要状态不是 READY，每个 PWM 周期都强制关闭门极。`drvCurrentRead()`、`drvCurrentCalibrateStart()` 仅供任务调用，重新标定同时清除故障；处理逻辑在 ADC 中断中运行，禁止调用 RTOS/LOG |
| `driver/current/drvCurrentDebug.*` | 注册 `cur` 控制台命令：`cur` 打印状态、电流、峰值和零偏，`cur cal` 重新标定并清除故障 |
| `module/log/` | 日志与控制台：core（`log.*`、`console.*`）每行格式化一次后写给所有启用的链路，console 从一条链路读命令；port（`portlog.*`）绑定 RTT 并集中配置；见 `module/log/log.md` |
| `develop/` | VS Code Device Tool 的 CMake 构建、烧录、复位与 RTT 工具，以及日志模块的主机端测试 `test_log.py`，见 `develop/README.md` |

`bsp blower speed <rps>` 调用 `blowerVcmSetFeedback()` 设置回传给 Core 的转速，仅用于链路联调；接入电机速度环后由 MotorTask 写入实测转速。上层通过 `blowerVcmGetControl()` 读取 Core 命令，动作前先用 `blowerVcmIsConnected()` 确认链路有效。

项目代码只能通过 `rtos.h` 使用任务、调度、tick 和临界区能力；FreeRTOS 原生 API 仅允许出现在 `portrtos.c`。日志统一使用 `LOG_I`、`LOG_W`、`LOG_E` 等宏，不能直接使用标准库输出函数。

`module/rtos/portrtos.c` 的毫秒转 tick 在乘法前提升到 64 位，避免默认 `pdMS_TO_TICKS` 在 1000 Hz 下运行约 71 分 35 秒后溢出。
