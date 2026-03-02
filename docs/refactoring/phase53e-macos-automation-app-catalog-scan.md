# Phase 53e: macOS Automation App Catalog Scan Enablement

## 判定先行
- 现象：macOS 设置页 `Refresh app list` 返回空列表，无法通过扫描选择目标应用。
- 判定：`Bug或回归`（能力缺失）。
- 最短依据：
  - `/api/automation/app-catalog` 使用 `platform::ScanApplicationCatalog()`；
  - `Platform/PlatformApplicationCatalog.cpp` 仅 Windows 分支有实现，非 Windows 直接返回空数组。

## 目标
1. 在不影响 Windows 行为的前提下，为 macOS 补齐应用目录扫描能力。
2. 保持 Linux 跟随策略不变（编译级 + 契约级）。

## 改动
1. 新增 macOS 应用扫描器
- 文件：`MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanner.h`
- 文件：`MFCMouseEffect/Platform/macos/System/MacosApplicationCatalogScanner.mm`
- 说明：
  - 扫描根目录：`/Applications`、`/System/Applications`、`~/Applications`；
  - 识别 `.app` bundle，读取 bundle executable 作为 `processName`；
  - 使用显示名作为 `displayName`，并按 `processName` 去重；
  - 输出继续走统一 `ApplicationCatalogEntry` 合同。

2. 平台门面接入 macOS 分发
- 文件：`MFCMouseEffect/Platform/PlatformApplicationCatalog.cpp`
- 说明：
  - 新增 `__APPLE__` 分支，调用 `macos::MacosApplicationCatalogScanner::Scan()`；
  - Windows 与其他平台原逻辑保持不变。

3. 构建接线（mac core lane）
- 文件：`MFCMouseEffect/Platform/CMakeLists.txt`
- 说明：
  - `mfx_entry_runtime_common` 增加 `MacosApplicationCatalogScanner.mm` 源文件。

## 验证
```bash
cmake --build /tmp/mfx-platform-macos-core-build --target mfx_entry_posix_host -j8
./tools/platform/regression/run-posix-scaffold-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-build
./tools/platform/regression/run-posix-core-automation-contract-regression.sh --platform auto --build-dir /tmp/mfx-platform-macos-core-build
cmake -S MFCMouseEffect/Platform -B /tmp/mfx-platform-linux-build -DMFX_PACKAGE_PLATFORM=linux -DMFX_ENABLE_CROSS_HOST_PACKAGES=ON -DMFX_ENABLE_ENTRY_RUNTIME_TARGETS=ON
cmake --build /tmp/mfx-platform-linux-build --target mfx_shell_linux mfx_entry_posix -j8
```
- 结果：全部通过。

## 脚本收口（2026-02-24）
1. core automation contract 新增 app-catalog 断言：
- `force=true` 与 `force=false` 两条路径都必须 `200 + ok=true + count` 字段。
2. 新增 selected-app scope 持久化断言：
- 从 app-catalog 取首个 `exe`，写入 `automation.mouse_mappings[0].app_scopes=["process:<exe>"]`；
- 再次 `GET /api/state` 必须包含同一 `app_scopes` 值。
3. 断言位置：
- `tools/platform/regression/lib/core_http.sh`
4. 结论：
- “Refresh app list 可用 + 选中后配置可持久化”主链路已脚本化收口。

## 手工验收建议
1. 启动 `mfx_entry_posix_host`（core lane）并打开设置页自动化分区。
2. 将某条映射的 App Scope 切到 `Selected Apps`，点击 `Refresh app list`。
3. 预期：
- 列表不再为空；
- 可选择应用并写入 scope；
- `All Apps` 行为与之前一致。
