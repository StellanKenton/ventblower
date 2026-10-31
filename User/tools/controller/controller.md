---
doc_role: tool-spec
layer: tools
module: controller
status: active
portability: standalone
public_headers:
  - pid.h
core_files:
  - pid.c
port_files: []
depends_on: []
copy_minimal_set:
  - pid.h
  - pid.c
---

# Controller 目录说明

`controller` 提供轻量、无平台依赖的控制算法。控制器对象由调用方持有，不使用动态内存、RTOS 或日志模块。

| 文件 | 职责 |
| --- | --- |
| `pid.h` | PID 状态对象、状态码与公共 API |
| `pid.c` | 位置式 PID、输出限幅和积分抗饱和实现 |

## PID contract

`pidInit` 中的 `samplePeriod` 单位为秒；`ki` 和 `kd` 分别按连续域的每秒积分增益和微分增益传入。`outputMin` 必须小于 `outputMax`。控制器使用测量值微分，设定值阶跃不会直接形成微分冲击。

```c
stPid controller;
float output;

if (pidInit(&controller, 1.0f, 0.2f, 0.01f, 0.002f, 0.0f, 100.0f) == PID_STATUS_OK) {
    (void)pidUpdate(&controller, setpoint, measurement, &output);
}
```

`pidUpdate` 必须按配置的固定周期调用。修改增益或输出范围不会清除运行状态；需要清除积分量和微分历史时调用 `pidReset`。同一个对象不能被多个任务或 ISR 并发更新；在调用方保证独占访问时，API 可用于任务或 ISR。
