# Device Tool

`device_tool.py` is a local development helper for this firmware project. It reads
`device_tool_config.json`, matches the current computer by OS and hostname, then
uses the matched profile to build, flash, reset, and read RTT logs from the
target board.

## Files

- `device_tool.py`: command line entry point.
- `quick_deploy.py`: writes the VS Code Device Tool tasks and status-bar buttons,
  and forwards `build`/`flash`/`reset`/`rtt` to `device_tool.py`.
- `device_tool_config.json`: per-computer tool paths and target settings.
- `test_log.py`: host-side test of the log core and console in `User/module/log`. It
  compiles `log.c` and `console.c` with a native GCC/Clang (or `CC`) against two fake
  links; run `python3 User/develop/test_log.py` (`py -3` on Windows).
- `ventblower.jdebug`: Ozone application debug project for `build/Debug/ventblower.elf`.
  Builds and normal flashing/reset/RTT operations use Device Tool as above.
  Ozone reset/download callbacks initialize SP, PC and VTOR from the application
  vectors at `0x08010000`; do not leave these callbacks empty. This avoids running
  a stale image at `0x08000000`. Reset strategy 0 uses SYSRESETREQ on this target;
  the reset-pin strategy did not clear the captured HardFault on the current board.
  It does not install or repair a bootloader:
  standalone power-on/reset still requires a valid bootloader that hands off to
  `0x08010000`. Use this project for application startup debugging; attach to an
  already running target when the bootloader handoff itself must be inspected.

## Computer Matching

On startup the tool detects:

- operating system: `mac`, `windows`, or `linux`
- hostname values from the local machine

It compares those values with each entry under `computers` in the config file.
If automatic matching is not enough, pass the profile name manually:

```bash
py -3 user/develop/device_tool.py --computer computer2 info
```

## Commands

Run from the repository root:

```bash
py -3 user/develop/quick_deploy.py deploy
py -3 user/develop/device_tool.py info
py -3 user/develop/quick_deploy.py build
py -3 user/develop/quick_deploy.py flash
py -3 user/develop/quick_deploy.py reset
py -3 user/develop/quick_deploy.py rtt
```

Command behavior:

- `info`: prints the detected computer information and selected profile.
- `deploy`: writes the VS Code tasks, status-bar commands, and extension recommendation.
- `build`: builds the firmware with CMake, Ninja, and Arm GNU Toolchain. CMake
  configure runs only when its cache is missing or the configured tools and
  build settings change; Ninja still regenerates automatically when project
  CMake files change.
- `flash`: downloads the configured firmware image through J-Link.
- `reset`: resets the target through J-Link and lets it run.
- `rtt`: stops existing J-Link RTT/GDB server processes, starts this tool's RTT
  server, then prints RTT output from the target.
  The interactive entry resolves `_SEGGER_RTT` from the ELF beside the configured
  flash image and supplies it during the Telnet handshake. Build and flash matching
  firmware first; `DEVICE_TOOL_RTT_ADDRESS` overrides the address when the board
  runs a different image. Without an ELF/toolchain it falls back to J-Link scanning.

## Config Notes

Each computer profile contains:

- `os`: expected host OS.
- `hostnames`: hostnames that identify this computer.
- `build`: compiler/build tool paths and build settings.
- `flash_image`: firmware image to download.
- `jlink`: J-Link executable paths, target device, SWD/JTAG speed, optional
  serial number, and RTT/debug ports.

Add more computers by copying `computer1` or `computer2` and changing the paths
