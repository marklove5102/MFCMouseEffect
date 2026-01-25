# 安装程序（Inno Setup）配置记录

## 需求实现
按照用户要求，安装程序实现了以下功能：

### 1. 安装前关闭正在运行的程序
- **实现方式**：在 `.iss` 脚本中配置了 `AppMutex=Global\MFCMouseEffect_SingleInstance_Mutex`。
- **效果**：Inno Setup 会在安装前检查该互斥体。如果程序正在运行，安装程序会弹出对话框提示用户关闭程序，或尝试自动关闭（`CloseApplications=yes`）。这利用了我们在代码中实现的进程单例逻辑。

### 2. 覆盖旧版本文件与配置持久化
- **实现方式**：
    - **文件覆盖**：利用 Inno Setup 的默认行为处理程序文件。
    - **配置分离**：修改了程序逻辑，现在配置文将优先保存在专用的用户数据目录 `%AppData%\MFCMouseEffect\config.json` 中。
- **效果**：即使安装在 `Program Files` 等受限目录下，设置也能正常保存和读取。安装包中的 `config.json` 仅作为默认模板，且配置了 `onlyifdoesntexist` 标志，不会覆盖用户已有的个性化设置。

### 3. 多语言支持
- **配置**：脚本包含了英文 (`English`) 的首选支持，并预留了中文接口。

### 4. 默认创建快捷方式与开机自启动
- **实现方式**：在脚本任务配置中移除了 `unchecked` 标志。
- **效果**：安装程序在运行时会默认勾选“创建桌面快捷方式”和“开机自启动”选项。

## 如何构建安装包
1. 确保已安装 [Inno Setup 6+](https://jrsoftware.org/isdl.php)。
2. 在 Visual Studio 中，将配置切换为 **Release | x64** 并执行 **重新生成解决方案**。
3. 打开 `Install/MFCMouseEffect.iss` 文件。
4. 在 Inno Setup 编译器中点击 `Build -> Compile` (或按 `F9`)。
5. 生成的安装包将保存在 `Install/Output/MFCMouseEffect_Setup.exe`。

## 相关文件
- [MFCMouseEffect.iss](file:///f:/language/cpp/code/MFCMouseEffect/Install/MFCMouseEffect.iss)
