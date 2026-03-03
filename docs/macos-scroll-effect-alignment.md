# macOS 滚轮特效对齐 + 渲染器重构

## 概要

将 macOS 三个滚轮特效（Arrow/Helix/Twinkle）对齐 Windows 视觉表现，并重构渲染架构：将逐帧动画计算提取到共用 C++ Core 层，两端渲染器仅保留绘制代码。

## 架构变更

```
重构前: Win GDI+ 渲染器 ←竞争复制→ macOS Swift 渲染器（各自包含动画逻辑）
重构后: 共用 Core 帧计算 → Win GDI+ 绘制 / macOS CGContext 绘制
```

### 新增共用 Core 文件
| 文件 | 说明 |
|------|------|
| `MouseFx/Core/Effects/ScrollHelixFrameCompute.h` | 3D 螺旋帧数据结构 + 无状态计算函数 |
| `MouseFx/Core/Effects/ScrollTwinkleFrameCompute.h` | 粒子状态类 + 帧数据结构 + 有状态计算 |

### C 桥接（macOS 专用）
| 文件 | 说明 |
|------|------|
| `Platform/macos/Effects/MacosScrollFrameComputeBridge.cpp` | C ABI 暴露 Core 帧计算给 Swift |

### 代码量变化
| 文件 | 变化 |
|------|------|
| Win `HelixRenderer.h` | 199 → 85 行 |
| Win `TwinkleRenderer.h` | 246 → 98 行 |
| macOS `MacosScrollHelixAnimator.swift` | 395 → 215 行 |
| macOS `MacosScrollTwinkleAnimator.swift` | 425 → 218 行 |

## 关键决策

1. **Helix 计算无状态**：每帧仅依赖 `(t, elapsedMs, params)`，可做纯函数
2. **Twinkle 计算有状态**：粒子列表 + RNG 需持久化，用 `ScrollTwinkleState` 类
3. **C 桥接用 thread-local**：帧数据零拷贝传递给 Swift
4. **Swift 6 兼容**：`nonisolated(unsafe)` 用于 draw layer 数据和 timer/state 属性
