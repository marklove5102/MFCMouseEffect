# Stage69: POSIX 单实例锁跨宿主可编译脚手架

## 1. 问题判定

- 判定: `Bug/架构风险`
- 现象:
  - 在 Windows 主机上构建 `mfx_shell_linux` / `mfx_shell_macos` 时，`SingleInstanceGuard` 直接依赖 POSIX 头（`fcntl.h/sys/file.h/unistd.h`），导致包构建验证中断。
- 影响:
  - 跨平台分包的“结构校验”无法在单机持续推进，阻塞平台迁移节奏。

## 2. 根因

- `LinuxSingleInstanceGuard.cpp` 与 `MacosSingleInstanceGuard.cpp` 只有 POSIX 实现分支，没有跨宿主脚手架分支。

## 3. 方案（本次落地）

1. 保留 Linux/macOS 的真实 POSIX 锁实现不变（非 `_WIN32` 分支）。
2. 新增 `_WIN32` 下的最小 stub 分支:
   - `Acquire` 仅维护进程内状态并返回成功；
   - `Release` 重置状态；
   - 不引入 POSIX 头。
3. 在平台 README 中显式标注:
   - 真机使用 POSIX 文件锁；
   - 跨宿主构建仅用于脚手架验证。

## 4. 主要改动文件

- `MFCMouseEffect/Platform/linux/Shell/LinuxSingleInstanceGuard.cpp`
- `MFCMouseEffect/Platform/macos/Shell/MacosSingleInstanceGuard.cpp`
- `MFCMouseEffect/Platform/linux/Shell/README.md`
- `MFCMouseEffect/Platform/macos/Shell/README.md`

## 5. 验证

在 Windows 主机（VS2026）验证通过：

```powershell
$cmake = "C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

# Linux package
& $cmake --preset linux-shell-vs2026
& $cmake --build ..\cmake_build\linux-shell-vs2026 --config Release --target mfx_shell_posix_common mfx_shell_linux
& $cmake --build ..\cmake_build\linux-shell-vs2026 --config Release --target mfx_entry_runtime_common mfx_entry_posix

# macOS package
& $cmake --preset macos-shell-vs2026
& $cmake --build ..\cmake_build\macos-shell-vs2026 --config Release --target mfx_shell_posix_common mfx_shell_macos
& $cmake --build ..\cmake_build\macos-shell-vs2026 --config Release --target mfx_entry_runtime_common mfx_entry_posix
```

结果: 全部目标在 Windows 主机完成编译。

## 6. 风险与边界

- `_WIN32` stub 仅用于“跨宿主结构编译验证”，不是 Linux/macOS 运行时行为。
- 真机语义仍由 POSIX 分支保证，不应以 stub 行为作为功能验收依据。
