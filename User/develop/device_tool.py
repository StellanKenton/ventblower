#!/usr/bin/env python3

import argparse
import json
import os
import platform
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional


SCRIPT_PATH = Path(__file__).resolve()
REPO_ROOT = SCRIPT_PATH.parents[2]
DEFAULT_CONFIG_PATH = SCRIPT_PATH.with_name("device_tool_config.json")
CONFIGURE_STATE_NAME = ".device_tool_configure.json"


class DeviceToolError(RuntimeError):
    pass


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build, flash, reset, and read RTT logs using per-computer config."
    )
    parser.add_argument(
        "-c",
        "--config",
        default=str(DEFAULT_CONFIG_PATH),
        help="Path to device_tool_config.json.",
    )
    parser.add_argument(
        "--computer",
        help="Computer profile name. If omitted, the script matches hostname and OS.",
    )

    subparsers = parser.add_subparsers(dest="action", required=True)
    subparsers.add_parser("info", help="Show detected machine and matched profile.")
    subparsers.add_parser("build", help="Build firmware with the configured compiler.")
    subparsers.add_parser("flash", help="Flash the configured firmware image by J-Link.")
    subparsers.add_parser("reset", help="Reset the target by J-Link.")
    subparsers.add_parser("rtt", help="Start J-Link RTT and print target logs.")

    return parser.parse_args()


def load_config(path: Path) -> Dict[str, Any]:
    if not path.is_file():
        raise DeviceToolError(f"Config file not found: {path}")

    with path.open("r", encoding="utf-8") as handle:
        config = json.load(handle)

    if not isinstance(config.get("computers"), dict):
        raise DeviceToolError("Config must contain a 'computers' object.")

    return config


def normalized_os_name() -> str:
    system = platform.system().lower()
    if system == "darwin":
        return "mac"
    if system.startswith("win"):
        return "windows"
    if system == "linux":
        return "linux"
    return system


def detected_hostnames() -> List[str]:
    names = [
        platform.node(),
        socket.gethostname(),
        socket.getfqdn(),
        os.environ.get("COMPUTERNAME"),
        os.environ.get("HOSTNAME"),
    ]

    normalized: List[str] = []
    for name in names:
        if not name:
            continue
        for candidate in (name, name.split(".")[0]):
            if candidate and candidate not in normalized:
                normalized.append(candidate)

    return normalized


def select_computer(
    config: Dict[str, Any], requested_name: Optional[str]
) -> tuple[str, Dict[str, Any]]:
    computers = config["computers"]

    if requested_name:
        if requested_name not in computers:
            raise DeviceToolError(f"Computer profile not found: {requested_name}")
        return requested_name, computers[requested_name]

    current_os = normalized_os_name()
    current_hosts = {name.lower() for name in detected_hostnames()}

    for name, profile in computers.items():
        profile_os = str(profile.get("os", "")).lower()
        if profile_os and profile_os != current_os:
            continue

        hostnames = profile.get("hostnames", [])
        for hostname in hostnames:
            if str(hostname).lower() in current_hosts:
                return name, profile

    default_name = config.get("default_computer")
    if default_name:
        if default_name not in computers:
            raise DeviceToolError(f"default_computer does not exist: {default_name}")
        return default_name, computers[default_name]

    raise DeviceToolError(
        "No matching computer profile. Add this hostname to config or pass --computer.\n"
        f"Detected OS: {current_os}\n"
        f"Detected hostnames: {', '.join(detected_hostnames())}"
    )


def repo_path(value: str) -> Path:
    path = Path(value).expanduser()
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def require_file(path: Path, label: str) -> None:
    if not path.is_file():
        raise DeviceToolError(f"{label} not found: {path}")


def require_executable(path_or_name: str, label: str) -> str:
    path = Path(path_or_name).expanduser()
    if path.is_absolute() or any(sep in path_or_name for sep in ("/", "\\")):
        if not path.exists():
            raise DeviceToolError(f"{label} not found: {path}")
        return str(path)

    resolved = shutil.which(path_or_name)
    if not resolved:
        raise DeviceToolError(f"{label} not found in PATH: {path_or_name}")
    return resolved


def run_command(
    command: List[str], cwd: Path = REPO_ROOT, input_text: Optional[str] = None
) -> None:
    print("+ " + " ".join(command), flush=True)
    result = subprocess.run(
        command,
        cwd=str(cwd),
        input=input_text,
        text=True,
    )
    if result.returncode != 0:
        raise DeviceToolError(f"Command failed with exit code {result.returncode}.")


def run_command_checked_output(
    command: List[str], cwd: Path = REPO_ROOT, input_text: Optional[str] = None
) -> str:
    print("+ " + " ".join(command), flush=True)
    result = subprocess.run(
        command,
        cwd=str(cwd),
        input=input_text,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.stdout:
        print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
    if result.returncode != 0:
        raise DeviceToolError(f"Command failed with exit code {result.returncode}.")
    return result.stdout or ""


def cmake_cache_value(cache_file: Path, key: str) -> Optional[str]:
    if not cache_file.is_file():
        return None

    prefix = f"{key}:"
    plain_prefix = f"{key}="
    for line in cache_file.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith(prefix):
            _, value = line.split("=", 1)
            return value
        if line.startswith(plain_prefix):
            return line.split("=", 1)[1]
    return None


def reset_cmake_cache(build_dir: Path) -> None:
    print(f"Removing stale CMake cache: {build_dir}", flush=True)
    for name in [
        "CMakeCache.txt",
        "build.ninja",
        "cmake_install.cmake",
        CONFIGURE_STATE_NAME,
    ]:
        path = build_dir / name
        if path.exists():
            path.unlink()
    for name in ["CMakeFiles", ".cmake"]:
        path = build_dir / name
        if path.exists():
            shutil.rmtree(path)


def reset_cmake_cache_if_needed(
    build_dir: Path, source_dir: Path, toolchain_file: Path, compiler_bin_dir: Path
) -> None:
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.is_file():
        return

    compiler_name = "arm-none-eabi-gcc.exe" if normalized_os_name() == "windows" else "arm-none-eabi-gcc"
    expected_compiler = compiler_bin_dir / compiler_name
    checks = {
        "CMAKE_HOME_DIRECTORY": str(source_dir),
        "CMAKE_TOOLCHAIN_FILE": str(toolchain_file),
        "ARM_GNU_TOOLCHAIN_BIN_DIR": str(compiler_bin_dir),
        "CMAKE_C_COMPILER": str(expected_compiler),
        "CMAKE_ASM_COMPILER": str(expected_compiler),
    }

    for key, expected in checks.items():
        cached = cmake_cache_value(cache_file, key)
        if cached and Path(cached).as_posix() != Path(expected).as_posix():
            reset_cmake_cache(build_dir)
            return


def file_identity(path: Path) -> List[Any]:
    stat = path.stat()
    return [str(path.resolve()), stat.st_size, stat.st_mtime_ns]


def cmake_configure_needed(build_dir: Path, expected_state: Dict[str, Any]) -> bool:
    if not (build_dir / "CMakeCache.txt").is_file():
        return True
    if not (build_dir / "build.ninja").is_file():
        return True

    state_file = build_dir / CONFIGURE_STATE_NAME
    if not state_file.is_file():
        return True

    try:
        actual_state = json.loads(state_file.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return True
    return actual_state != expected_state


def build_with_cmake(build: Dict[str, Any]) -> None:
    cmake = require_executable(str(build["cmake"]), "cmake")
    ninja = require_executable(str(build["ninja"]), "ninja")
    compiler_bin_dir = repo_path(str(build["compiler_bin_dir"]))
    toolchain_file = repo_path(str(build["toolchain_file"]))
    source_dir = repo_path(str(build.get("source_dir", ".")))
    build_dir = repo_path(str(build.get("build_dir", "build/Debug")))

    require_file(toolchain_file, "CMake toolchain file")
    compiler_name = "arm-none-eabi-gcc.exe" if normalized_os_name() == "windows" else "arm-none-eabi-gcc"
    compiler = compiler_bin_dir / compiler_name
    require_file(compiler, "arm-none-eabi-gcc")
    reset_cmake_cache_if_needed(build_dir, source_dir, toolchain_file, compiler_bin_dir)

    configure_cmd = [
        cmake,
        "-S",
        str(source_dir),
        "-B",
        str(build_dir),
        "-G",
        str(build.get("generator", "Ninja")),
        f"-DCMAKE_MAKE_PROGRAM={ninja}",
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}",
        f"-DARM_GNU_TOOLCHAIN_BIN_DIR={compiler_bin_dir}",
        f"-DCMAKE_BUILD_TYPE={build.get('build_type', 'Debug')}",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
        if build.get("export_compile_commands", True)
        else "-DCMAKE_EXPORT_COMPILE_COMMANDS=OFF",
    ]
    configure_cmd.extend(str(arg) for arg in build.get("extra_configure_args", []))
    configure_state = {
        "version": 1,
        "command": configure_cmd,
        "cmake": file_identity(Path(cmake)),
        "ninja": file_identity(Path(ninja)),
        "compiler": file_identity(compiler),
        "toolchain": file_identity(toolchain_file),
    }

    if cmake_configure_needed(build_dir, configure_state):
        run_command(configure_cmd)
        (build_dir / CONFIGURE_STATE_NAME).write_text(
            json.dumps(configure_state, indent=2) + "\n", encoding="utf-8"
        )
    else:
        print("CMake configuration is unchanged; skipping configure.", flush=True)
    run_command([cmake, "--build", str(build_dir)])


def build_with_keil(build: Dict[str, Any]) -> None:
    uv4 = require_executable(str(build["uv4"]), "Keil UV4")
    project = repo_path(str(build["project"]))
    target = str(build["target"])
    log_path = repo_path(str(build.get("log", "build_log.txt")))
    timeout_seconds = int(build.get("timeout_seconds", 180))

    require_file(project, "Keil project")
    log_path.parent.mkdir(parents=True, exist_ok=True)
    if log_path.exists():
        log_path.unlink()

    command = [uv4, "-r", str(project), "-t", target, "-j0", "-o", str(log_path)]
    print("+ " + " ".join(command))
    process = subprocess.Popen(command, cwd=str(project.parent))
    deadline = time.time() + timeout_seconds

    while time.time() < deadline:
        if log_path.exists():
            text = log_path.read_text(encoding="utf-8", errors="replace")
            if (
                "Build Time Elapsed:" in text
                or "Build aborted." in text
                or "Error(s)," in text
            ):
                break
        if process.poll() is not None and log_path.exists():
            break
        time.sleep(0.5)
    else:
        process.terminate()
        raise DeviceToolError(f"Keil build timed out after {timeout_seconds}s.")

    if process.poll() is None:
        process.wait(timeout=5)

    if not log_path.exists():
        raise DeviceToolError(f"Keil build log was not generated: {log_path}")

    build_log = log_path.read_text(encoding="utf-8", errors="replace").rstrip()
    print(build_log)

    if "Build aborted." in build_log:
        raise DeviceToolError("Keil build aborted.")

    import re

    match = re.search(r"(\d+)\s+Error\(s\),\s+(\d+)\s+Warning\(s\)\.", build_log)
    if not match:
        raise DeviceToolError("Keil build completed without a recognizable summary.")
    if int(match.group(1)) != 0:
        raise DeviceToolError("Keil build failed.")


def build(profile: Dict[str, Any]) -> None:
    build_spec = profile.get("build", {})
    build_type = str(build_spec.get("type", "")).lower()
    if build_type == "cmake":
        build_with_cmake(build_spec)
    elif build_type == "keil":
        build_with_keil(build_spec)
    else:
        raise DeviceToolError(f"Unsupported build.type: {build_type}")


def jlink_base_args(jlink: Dict[str, Any]) -> List[str]:
    args = [
        "-device",
        str(jlink["device"]),
        "-if",
        str(jlink.get("interface", "swd")),
        "-speed",
        str(jlink.get("speed_khz", 4000)),
    ]
    serial_number = jlink.get("serial_number")
    if serial_number:
        args.extend(["-SelectEmuBySN", str(serial_number)])
    return args


def invoke_jlink_commander(jlink: Dict[str, Any], commands: Iterable[str]) -> None:
    jlink_exe = require_executable(str(jlink["jlink_exe"]), "J-Link commander")
    script = "\n".join(commands) + "\n"
    output = run_command_checked_output([jlink_exe, *jlink_base_args(jlink)], input_text=script)
    failure_markers = [
        "Cannot connect to the probe/programmer",
        "Cannot connect to J-Link",
        "J-Link connection not established",
        "Failed to connect",
        "Failed to prepare for programming",
        "Failed to power up DAP",
        "ERROR:",
        "Error:",
        "****** Error:",
    ]
    if any(marker in output for marker in failure_markers):
        raise DeviceToolError("J-Link command reported a failure.")
    if "O.K." not in output:
        raise DeviceToolError("J-Link command did not report success.")


def flash(profile: Dict[str, Any]) -> None:
    image = repo_path(str(profile["flash_image"]))
    require_file(image, "Flash image")
    invoke_jlink_commander(
        profile["jlink"],
        [
            "r",
            "h",
            f"loadfile {image}",
            "r",
            "g",
            "qc",
        ],
    )


def reset(profile: Dict[str, Any]) -> None:
    invoke_jlink_commander(profile["jlink"], ["r", "g", "qc"])


def kill_processes_by_name(names: Iterable[str]) -> None:
    if normalized_os_name() == "windows":
        for name in names:
            image_name = name if name.lower().endswith(".exe") else f"{name}.exe"
            subprocess.run(
                ["taskkill", "/F", "/IM", image_name],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
        return

    for name in names:
        subprocess.run(
            ["pkill", "-f", name],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )


def kill_listeners_on_ports(ports: Iterable[int]) -> None:
    if normalized_os_name() == "windows":
        return

    for port in ports:
        result = subprocess.run(
            ["lsof", "-tiTCP:" + str(port), "-sTCP:LISTEN"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
        )
        for pid_text in result.stdout.splitlines():
            try:
                os.kill(int(pid_text), signal.SIGTERM)
            except ProcessLookupError:
                pass


def connect_tcp_port(port: int, timeout_seconds: float) -> Optional[socket.socket]:
    deadline = time.time() + timeout_seconds
    while time.time() < deadline:
        try:
            return socket.create_connection(("127.0.0.1", port), timeout=0.3)
        except OSError:
            time.sleep(0.2)
    return None


def start_rtt(profile: Dict[str, Any]) -> None:
    jlink = profile["jlink"]
    ports = jlink["ports"]
    gdb_server = require_executable(str(jlink["gdb_server"]), "J-Link GDB server")
    rtt_port = int(ports["rtt"])
    capture_seconds = float(os.environ.get("DEVICE_TOOL_RTT_SECONDS", "0") or "0")
    capture_deadline = time.monotonic() + capture_seconds if capture_seconds > 0 else None

    kill_listeners_on_ports(int(port) for port in ports.values())
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

    command = [
        gdb_server,
        *jlink_base_args(jlink),
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

    print("+ " + " ".join(command))
    server = subprocess.Popen(
        command,
        stdout=log_file,
        stderr=subprocess.STDOUT,
        text=True,
    )

    try:
        sock = connect_tcp_port(rtt_port, 15)
        if sock is None:
            server.poll()
            log_file.flush()
            server_output = log_path.read_text(encoding="utf-8", errors="replace")
            raise DeviceToolError(
                f"RTT port did not open: 127.0.0.1:{rtt_port}\n{server_output}"
            )

        print(f"RTT log connected on 127.0.0.1:{rtt_port}. Press Ctrl+C to stop.")
        with sock:
            sock.settimeout(0.5)
            while True:
                if capture_deadline is not None and time.monotonic() >= capture_deadline:
                    break
                try:
                    data = sock.recv(4096)
                    if not data:
                        break
                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()
                except socket.timeout:
                    if server.poll() is not None:
                        break
    except KeyboardInterrupt:
        print("\nRTT stopped.")
    finally:
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


def print_info(name: str, profile: Dict[str, Any]) -> None:
    print(f"Detected OS: {normalized_os_name()}")
    print(f"Detected hostnames: {', '.join(detected_hostnames())}")
    print(f"Matched computer: {name}")
    print(f"Profile OS: {profile.get('os')}")
    print(f"Build type: {profile.get('build', {}).get('type')}")
    print(f"Flash image: {repo_path(str(profile.get('flash_image', '')))}")
    jlink = profile.get("jlink", {})
    print(f"J-Link commander: {jlink.get('jlink_exe')}")
    print(f"J-Link GDB server: {jlink.get('gdb_server')}")
    print(f"J-Link device: {jlink.get('device')}")


def main() -> int:
    args = parse_args()

    try:
        config = load_config(Path(args.config).expanduser())
        computer_name, profile = select_computer(config, args.computer)

        if args.action == "info":
            print_info(computer_name, profile)
        elif args.action == "build":
            build(profile)
        elif args.action == "flash":
            flash(profile)
        elif args.action == "reset":
            reset(profile)
        elif args.action == "rtt":
            start_rtt(profile)
        else:
            raise DeviceToolError(f"Unknown action: {args.action}")
    except DeviceToolError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
