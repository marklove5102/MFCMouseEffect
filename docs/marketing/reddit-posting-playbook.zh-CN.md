# Reddit 发布打法

## 目的
- 帮你把项目发出去时更像真实开发者分享，而不是像“求 Star 广告”。
- 优先解决两个问题：
  - 发什么
  - 怎么发才不显得尬

## 先判定
在 Reddit 上，`MFCMouseEffect` 更适合被判定成：
- `开发中的开源桌面引擎/工具`
- 不是纯消费级成品 App 广告
- 不是“只会点几下特效”的演示玩具

所以文案重心应该是：
- 你解决了什么问题
- 这个问题为什么不是小玩具
- 你做了哪些架构取舍
- 你现在最想要什么反馈

不要把重心放在：
- 我做了很久
- 求大家支持一下
- 功能很多很强大

## 推荐发帖顺序
### 第一波
- 目标：拿到“别人怎么理解这个项目”的反馈
- 场景：`r/programming`、`r/cpp`、`r/opensource`
- 文案：用 [reddit-promo-pack.en.md](/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/marketing/reddit-promo-pack.en.md) 里的 `Short Post` 或 `Medium Post`
- 配图：一张 GIF 或 15-30 秒短视频

### 第二波
- 目标：拿到“哪个能力最吸引人”的反馈
- 场景：更偏应用的社区，比如 `r/macapps`
- 文案：强调输入可视化、录屏、演示、教程场景
- 配图：输入指示器 + 光标效果联合展示

### 第三波
- 目标：拿到“插件/WASM 路线是否值得继续打磨”的反馈
- 场景：`r/cpp`、`r/programming`
- 文案：聚焦宿主边界、WASM 插件、回退与诊断
- 配图：架构图优先于产品图

## 一条帖子只讲一件事
不要一条帖子里同时强调：
- 鼠标特效
- 输入指示器
- 自动化映射
- WASM 插件
- 桌面宠物

这样会让外部读者不知道项目到底是什么。

更好的做法是每次只选一个主叙事：
- `A. 这是个输入可视化工具/引擎`
- `B. 这是个原生桌面宿主 + WASM 插件边界实验`
- `C. 这是个给录屏/教程/演示增强反馈的项目`

## 推荐叙事模板
### 叙事 A
- 开头：我本来只是想做鼠标特效
- 中段：后来发现真正的问题是“输入反馈系统”而不是一个动画
- 结尾：现在项目演进成 effects + indicator + automation + plugin runtime

### 叙事 B
- 开头：我想做可扩展插件，但不想让插件直接控制渲染和窗口
- 中段：所以做了 bounded WASM host/plugin boundary
- 结尾：想听听大家觉得这个边界是合理还是过度约束

### 叙事 C
- 开头：很多教程/录屏内容其实缺的是可理解的输入反馈
- 中段：所以我把 click/trail/scroll/hold/hover、键盘/滚轮指示器、自动化映射放到一个项目里
- 结尾：想知道哪些场景最值得先做 polished demo

## 发帖前检查
- 标题里只保留一个主卖点
- 正文前 3 行就说明项目是干什么的
- 放一个明确链接：
  - GitHub 仓库
- 配图/动图最好真实，不要只放设置页
- 结尾要问一个具体问题，而不是泛泛地“求反馈”

## 发帖后动作
- 第一时间自己发一条评论，补：
  - GitHub 链接
  - 平台现状
  - 你最想得到的反馈
- 有人质疑“太 niche”时，不要急着反驳
  - 直接承认 niche，但说明可服务的高价值场景
- 有人问“为什么不用别的技术栈”时
  - 回答设计边界，不要变成技术站队

## 高质量问题示例
- Which part sounds most useful from the outside: effects, indicator, automation, or plugins?
- If I polish one demo first, which one should it be?
- Does this read more like a tool people would use, or an engine developers would build on?
- Is the WASM boundary interesting, or does it feel too constrained to matter?

## 低质量结尾示例
- Please star if you like it
- Hope you all support this project
- Let me know what you think

上面这些太空，互动质量通常不会高。

## 本目录推荐用法
1. 先读 [reddit-promo-pack.en.md](/Users/sunqin/study/language/cpp/code/MFCMouseEffect/docs/marketing/reddit-promo-pack.en.md)
2. 选一个目标社区和一个主叙事
3. 选一条标题
4. 选一版正文
5. 再从评论模板里预写第一条自评论
