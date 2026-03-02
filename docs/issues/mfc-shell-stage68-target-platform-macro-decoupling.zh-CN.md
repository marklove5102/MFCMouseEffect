# Stage68: 目标平台宏解耦（入口运行时与工厂分支统一）

## 1. 问题判定

- 判定: `Bug/架构风险`
- 现象:
  - `Platform` 包的入口运行时代码分支主要依赖宿主编译宏（`_WIN32/__APPLE__/__linux__`）。
  - 在“包目标平台”和“宿主平台”不一致的构建图中，代码会走错分支，导致构建产物语义偏移。
- 结论:
  - 这不是单点补丁问题，而是“目标平台判定入口不统一”的结构性问题。

## 2. 根因

- `PlatformAppShellFactory.cpp`
- `PlatformShellServicesFactory.cpp`
- `PlatformWasmRuntimeFactory.cpp`
- `PlatformStartupOptionsFactory.cpp`

以上文件按宿主宏分支，未与 `Platform/CMakeLists.txt` 的“包目标平台”形成强绑定。

## 3. 方案（本次落地）

1. 新增统一目标平台判定头 `Platform/PlatformTarget.h`。
2. 将上述 4 个工厂/入口相关实现切换为 `MFX_PLATFORM_*` 分支。
3. 在各平台包目标增加目标宏:
   - `MFX_TARGET_PLATFORM_WINDOWS=1`
   - `MFX_TARGET_PLATFORM_MACOS=1`
   - `MFX_TARGET_PLATFORM_LINUX=1`
4. `mfx_entry_runtime_common` 在 CMake 中强制绑定唯一平台目标，避免多平台同图时分支歧义。
5. 当同一构建图包含多个 shell 包时，若仍开启 `MFX_ENABLE_ENTRY_RUNTIME_TARGETS`，直接 `FATAL_ERROR`，避免产物语义不确定。

## 4. 主要改动文件

- `MFCMouseEffect/Platform/PlatformTarget.h` (new)
- `MFCMouseEffect/Platform/PlatformAppShellFactory.cpp`
- `MFCMouseEffect/Platform/PlatformShellServicesFactory.cpp`
- `MFCMouseEffect/Platform/PlatformWasmRuntimeFactory.cpp`
- `MFCMouseEffect/Platform/PlatformStartupOptionsFactory.cpp`
- `MFCMouseEffect/Platform/CMakeLists.txt`
- `MFCMouseEffect/Platform/windows/CMakeLists.txt`
- `MFCMouseEffect/Platform/macos/CMakeLists.txt`
- `MFCMouseEffect/Platform/linux/CMakeLists.txt`
- `MFCMouseEffect/Platform/README.md`
- `MFCMouseEffect/MFCMouseEffect.vcxproj`
- `MFCMouseEffect/MFCMouseEffect.vcxproj.filters`

## 5. 验证

### 5.1 Windows 解决方案构建

```powershell
"C:\Program Files\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe" `
  MFCMouseEffect.slnx /t:Build /p:Configuration=Release /p:Platform=x64
```

预期: 成功，0 error。

### 5.2 Platform 包构建（Windows）

```powershell
cmake --preset windows-shell-vs2026
cmake --build --preset build-windows-shell-release
cmake --build --preset build-windows-entry-release
```

预期: `mfx_shell_windows`、`mfx_entry_runtime_common`、`mfx_entry_windows` 构建通过。

### 5.3 Win32 边界检查

```powershell
powershell -ExecutionPolicy Bypass -File tools/architecture/Check-NonPlatformWin32Boundary.ps1
```

预期: `OK`。

## 6. 风险与后续

- 本次只解决“目标平台分支判定入口统一”问题，不新增非 Windows 真机能力。
- 若后续需要在单次构建图同时产出 `all` 平台入口产物，建议拆分为 `mfx_entry_runtime_<platform>` 多目标，而不是复用一个 `mfx_entry_runtime_common`。
