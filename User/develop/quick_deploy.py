#!/usr/bin/env python3
# Author   : Stellan
# Copyright: Copyright (c) 2026 ResMed. All rights reserved.

import argparse
import json
import os
import queue
import socket
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional

import device_tool


SCRIPT_PATH = Path(__file__).resolve()
SCRIPT_DIR = SCRIPT_PATH.parent
REPO_ROOT = SCRIPT_PATH.parents[2]
VSCODE_DIR = REPO_ROOT / ".vscode"
TASKS_PATH = VSCODE_DIR / "tasks.json"
SETTINGS_PATH = VSCODE_DIR / "settings.json"
EXTENSIONS_PATH = VSCODE_DIR / "extensions.json"

TASK_PREFIX = "Device Tool:"
COMMAND_PREFIX = "Ventblower"
DEVICE_TOOL_PATH = "${workspaceFolder}/user/develop/quick_deploy.py"
WINDOWS_PYTHON_ARGS = ["-3", DEVICE_TOOL_PATH]


def load_json(path: Path, default: Any) -> Any:
    if not path.is_file():
        return default
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def write_json(path: Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as handle:
        json.dump(data, handle, indent=2, ensure_ascii=False)
        handle.write("\n")


def task(
    label: str,
    action: str,
    *,
    computer: Optional[str] = None,
    dedicated: bool = False,
    build: bool = False,
) -> Dict[str, Any]:
    presentation: Dict[str, Any] = {
        "echo": True,
        "reveal": "always",
        "focus": dedicated,
        "panel": "dedicated" if dedicated else "shared",
        "clear": False if dedicated else True,
        "showReuseMessage": False,
    }

    args = [DEVICE_TOOL_PATH, action]
    windows_args = [*WINDOWS_PYTHON_ARGS, action]
    if computer:
        args.extend(["--computer", computer])
        windows_args.extend(["--computer", computer])

    entry: Dict[str, Any] = {
        "label": label,
        "type": "shell",
        "command": "python3",
        "windows": {"command": "py", "args": windows_args},
        "osx": {"command": "python3"},
        "linux": {"command": "python3"},
        "args": args,
        "problemMatcher": [],
        "presentation": presentation,
        "options": {"cwd": "${workspaceFolder}"},
    }

    if build:
        entry["group"] = {"kind": "build", "isDefault": True}
        entry["problemMatcher"] = ["$gcc"]

    return entry


def upsert_task(tasks: List[Dict[str, Any]], entry: Dict[str, Any]) -> None:
    label = entry["label"]
    for index, existing in enumerate(tasks):
        if existing.get("label") == label:
            tasks[index] = entry
            return
    tasks.append(entry)


def command(name: str, label: str, text: str, icon: str, priority: int, tooltip: str) -> Dict[str, Any]:
    return {
        "command": "workbench.action.tasks.runTask",
        "args": label,
        "icon": icon,
        "statusBar": {
            "alignment": "left",
            "text": text,
            "name": name,
            "priority": priority,
            "tooltip": tooltip,
        },
    }


def deploy(computer: Optional[str]) -> None:
    tasks_json = load_json(TASKS_PATH, {"version": "2.0.0", "tasks": []})
    if "version" not in tasks_json:
        tasks_json["version"] = "2.0.0"
    tasks = tasks_json.setdefault("tasks", [])

    upsert_task(tasks, task("Device Tool: Build", "build", computer=computer, build=True))
    upsert_task(tasks, task("Device Tool: Flash", "flash", computer=computer))
    upsert_task(tasks, task("Device Tool: Reset", "reset", computer=computer))
    upsert_task(tasks, task("Device Tool: RTT", "rtt", computer=computer, dedicated=True))
    write_json(TASKS_PATH, tasks_json)

    settings = load_json(SETTINGS_PATH, {})
    settings["cmake.configureOnOpen"] = False
    settings["C_Cpp.default.compileCommands"] = (
        "${workspaceFolder}/build/Debug/compile_commands.json"
    )
    workspace_commands = settings.setdefault("commands.workspaceCommands", {})
    workspace_commands[f"{COMMAND_PREFIX} Build"] = command(
        f"{COMMAND_PREFIX} Build",
        "Device Tool: Build",
        " Build",
        "tools",
        420,
        "Build firmware with user/develop/device_tool.py in the integrated terminal.",
    )
    workspace_commands[f"{COMMAND_PREFIX} Flash"] = command(
        f"{COMMAND_PREFIX} Flash",
        "Device Tool: Flash",
        " Flash",
        "plug",
        419,
        "Flash the target through J-Link in the integrated terminal.",
    )
    workspace_commands[f"{COMMAND_PREFIX} Reset"] = command(
        f"{COMMAND_PREFIX} Reset",
        "Device Tool: Reset",
        " Reset",
        "debug-restart",
        418,
        "Reset and run the target through J-Link.",
    )
    workspace_commands[f"{COMMAND_PREFIX} RTT"] = command(
        f"{COMMAND_PREFIX} RTT",
        "Device Tool: RTT",
        " RTT",
        "terminal",
        417,
        "Open an interactive J-Link RTT terminal. Keyboard input is forwarded to RTT.",
    )
    write_json(SETTINGS_PATH, settings)

    extensions = load_json(EXTENSIONS_PATH, {"recommendations": []})
    recommendations = extensions.setdefault("recommendations", [])
    for extension in [
        "ms-vscode.cmake-tools",
        "ms-vscode.cpptools",
        "usernamehw.commands",
    ]:
        if extension not in recommendations:
            recommendations.append(extension)
    write_json(EXTENSIONS_PATH, extensions)

    print(f"Updated {TASKS_PATH}")
    print(f"Updated {SETTINGS_PATH}")
    print(f"Updated {EXTENSIONS_PATH}")
    if computer:
        print(f"Buttons are pinned to computer profile: {computer}")
    else:
        print("Buttons will auto-match the current computer from device_tool_config.json.")
    print("Reload VS Code if the new bottom status bar buttons are not visible yet.")


def select_profile(computer: Optional[str]) -> tuple[str, Dict[str, Any]]:
    config = device_tool.load_config(device_tool.DEFAULT_CONFIG_PATH)
    name, profile = device_tool.select_computer(config, computer)
    print(
        f"Matched computer: {name} "
        f"(OS={device_tool.normalized_os_name()}, hosts={', '.join(device_tool.detected_hostnames())})",
        flush=True,
    )
    return name, profile


def run_device_tool(action: str, computer: Optional[str]) -> int:
    _, profile = select_profile(computer)

    try:
        if action == "build":
            device_tool.build(profile)
        elif action == "flash":
            device_tool.flash(profile)
        elif action == "reset":
            device_tool.reset(profile)
        else:
            raise device_tool.DeviceToolError(f"Unsupported action: {action}")
    except device_tool.DeviceToolError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


def kill_processes_by_name(names: Iterable[str]) -> None:
    device_tool.kill_processes_by_name(names)


def connect_tcp_port(port: int, timeout_seconds: float) -> Optional[socket.socket]:
    return device_tool.connect_tcp_port(port, timeout_seconds)


def socket_reader(sock: socket.socket, stop: threading.Event) -> None:
    sock.settimeout(0.2)
    while not stop.is_set():
        try:
            data = sock.recv(4096)
        except socket.timeout:
            continue
        except OSError:
            break
        if not data:
            break
        sys.stdout.buffer.write(data)
        sys.stdout.buffer.flush()
    stop.set()


def stdin_reader(output: "queue.Queue[bytes]", stop: threading.Event) -> None:
    while not stop.is_set():
        data = sys.stdin.buffer.readline()
        if not data:
            break
        output.put(data)


def rtt_control_block_address(profile: Dict[str, Any]) -> Optional[int]:
    """Resolve the built firmware's RTT block before the Telnet handshake deadline."""
    override = os.environ.get("DEVICE_TOOL_RTT_ADDRESS")
    if override:
        return int(override, 0)
    elf = (REPO_ROOT / profile["flash_image"]).with_suffix(".elf")
    compiler_dir = profile.get("build", {}).get("compiler_bin_dir", "")
    nm = Path(compiler_dir) / ("arm-none-eabi-nm.exe" if os.name == "nt" else "arm-none-eabi-nm")
    if not elf.is_file() or not nm.is_file():
        return None
    try:
        result = subprocess.run([str(nm), str(elf)], capture_output=True, text=True,
                                check=True, timeout=5)
        for line in result.stdout.splitlines():
            fields = line.split()
            if len(fields) == 3 and fields[2] == "_SEGGER_RTT":
                return int(fields[0], 16)
    except (OSError, subprocess.SubprocessError, ValueError):
        pass
    return None


def interactive_rtt(computer: Optional[str]) -> int:
    _, profile = select_profile(computer)
    jlink = profile["jlink"]
    ports = jlink["ports"]
    gdb_server = device_tool.require_executable(str(jlink["gdb_server"]), "J-Link GDB server")
    rtt_port = int(ports["rtt"])
    rtt_address = rtt_control_block_address(profile)
    capture_seconds = float(os.environ.get("DEVICE_TOOL_RTT_SECONDS", "0") or "0")
    capture_deadline = time.monotonic() + capture_seconds if capture_seconds > 0 else None

    device_tool.kill_listeners_on_ports(int(port) for port in ports.values())
    kill_processes_by_name([
        "JLinkGDBServer",
        "JLinkGDBServerCL",
        "JLinkRTTClient",
        "JLinkRTTViewer",
        "JLinkGUIServer",
    ])
    time.sleep(0.5)

    log_file = tempfile.NamedTemporaryFile(
        mode="w+", prefix="vent-net-rtt-", suffix=".log", delete=False
    )
    log_path = Path(log_file.name)

    command_line = [
        gdb_server,
        *device_tool.jlink_base_args(jlink),
        "-nohalt",
        "-port",
        str(ports["gdb"]),
        "-swoport",
        str(ports["swo"]),
        "-telnetport",
        str(ports["telnet"]),
        "-RTTTelnetPort",
        str(rtt_port),
    ]

    print("+ " + " ".join(command_line), flush=True)
    server = subprocess.Popen(
        command_line,
        stdout=log_file,
        stderr=subprocess.STDOUT,
        text=True,
    )

    stop = threading.Event()
    pending_input: "queue.Queue[bytes]" = queue.Queue()

    try:
        sock = connect_tcp_port(rtt_port, 15)
        if sock is None:
            server.poll()
            log_file.flush()
            server_output = log_path.read_text(encoding="utf-8", errors="replace")
            print(f"RTT port did not open: 127.0.0.1:{rtt_port}", file=sys.stderr)
            print(server_output, file=sys.stderr)
            return 1

        if rtt_address is not None:
            sock.sendall(f"$$SEGGER_TELNET_ConfigStr=SetRTTAddr;0x{rtt_address:X}$$".encode("ascii"))
        print(
            f"RTT connected on 127.0.0.1:{rtt_port}. Type input here; press Ctrl+C to stop.",
            flush=True,
        )
        reader = threading.Thread(target=socket_reader, args=(sock, stop), daemon=True)
        writer = threading.Thread(target=stdin_reader, args=(pending_input, stop), daemon=True)
        reader.start()
        writer.start()

        with sock:
            while not stop.is_set():
                if capture_deadline is not None and time.monotonic() >= capture_deadline:
                    stop.set()
                    break
                if server.poll() is not None:
                    stop.set()
                    break
                try:
                    data = pending_input.get(timeout=0.1)
                except queue.Empty:
                    continue
                try:
                    sock.sendall(data)
                except OSError:
                    stop.set()
                    break
    except KeyboardInterrupt:
        print("\nRTT stopped.")
    finally:
        stop.set()
        server.terminate()
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
        log_file.close()
        try:
            log_path.unlink()
        except OSError:
            pass

    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Deploy VS Code quick buttons and run device_tool actions."
    )
    parent = argparse.ArgumentParser(add_help=False)
    parent.add_argument(
        "--computer",
        help=(
            "Computer profile name from device_tool_config.json. "
            "If omitted, the current OS and hostname are matched automatically."
        ),
    )
    subparsers = parser.add_subparsers(dest="action", required=True)
    subparsers.add_parser(
        "deploy",
        parents=[parent],
        help="Update .vscode tasks and bottom status bar buttons.",
    )
    subparsers.add_parser(
        "build",
        parents=[parent],
        help="Build firmware and stream output to this terminal.",
    )
    subparsers.add_parser(
        "flash",
        parents=[parent],
        help="Flash firmware and stream output to this terminal.",
    )
    subparsers.add_parser(
        "reset",
        parents=[parent],
        help="Reset target and stream output to this terminal.",
    )
    subparsers.add_parser("rtt", parents=[parent], help="Open interactive RTT terminal.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    os.chdir(REPO_ROOT)

    if args.action == "deploy":
        deploy(args.computer)
        return 0
    if args.action == "rtt":
        return interactive_rtt(args.computer)
    return run_device_tool(args.action, args.computer)


if __name__ == "__main__":
    raise SystemExit(main())
