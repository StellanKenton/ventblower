## Session Start

Every new session must begin by reading this [AGENTS.md](AGENTS.md), then immediately read the [rule.md](rule/rule.md) in the current directory

当前项目的编译、烧录、复位和 RTT 日志查看，默认使用 VS Code 下方的 Device Tool 按钮或同名任务，不要绕过这套入口。

Device Tool 脚本入口位于 `user/develop/`：

- `quick_deploy.py`：负责把 `Device Tool: Build/Flash/Reset/RTT` 任务和底部状态栏按钮写入工作区；`deploy` 动作用来更新 `.vscode/tasks.json`、`.vscode/settings.json`、`.vscode/extensions.json`。
- `device_tool.py`：负责实际执行 `build`、`flash`、`reset`、`rtt`。它会读取 `device_tool_config.json`，按当前主机的 OS 和 hostname 自动匹配 computer profile，也可以通过 `--computer` 强制指定。
- 在当前 Windows 仓库中，Device Tool 任务应走 `py -3 user/develop/quick_deploy.py <action>`，不要用 `python3`。

脚本行为约束：

- `build`：按匹配到的 profile 执行 CMake 或 Keil 构建。
- `flash`：调用 J-Link Commander 下载 profile 中配置的固件镜像。
- `reset`：调用 J-Link Commander 复位并运行目标板。
- `rtt`：先清理已有 J-Link 相关进程，再启动 J-Link GDB Server，并通过 RTT Telnet 端口读取日志；若 RTT 无输出，优先考虑重启固件。
