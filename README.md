# RDR 修改器 (Red Dead Redemption PC Trainer)

荒野大镖客：救赎（RDR1 PC 版）修改器。以 **RedHook** 脚本钩子为底座，用**原生脚本函数（natives）**实现功能，UI 通过 RedHook 的 2D 绘制接口（`RH_DrawText` / `RH_DrawRect`）渲染，**不 hook DX12 交换链**，因此不和游戏的 NVIDIA Streamline 渲染层打架，稳定性更高。

编译产物是 RedHook 插件 `RDRTrainer.red`（本质是 DLL，RedHook 用 `.red` 后缀区分自己的插件）。

---

## ✨ 功能

| 功能 | 说明 |
| --- | --- |
| **无敌** | `SET_ACTOR_INVULNERABILITY` + `SET_ACTOR_UNKILLABLE`；可选每帧补满血量、保护坐骑、无限死神之眼、死神之眼强化 |
| **无限子弹** | 只对手持武器操作（**不再盲遍历武器 ID**，避免崩溃）；弹夹 + 备弹都不减（每半秒自动补满） |
| **自瞄** | 鼠标自瞄模式（真正移动准星）；屏幕中心 FOV 圈，圈内敌人自动锁定；**目标粘滞**（防多敌视角乱晃）；优先离准星最近；**掩体露头也能瞄**（头可见即可瞄）；死区 + 帧率无关平滑（不抖） |
| **敌人透视** | 方框/边角框、名称、距离、血量条、连线、自瞄目标高亮；显示尸体/自己/动物/中立；可调距离/线宽/标签/人物高度/分类颜色/敌对判定方式 |
| **不会被通缉** | 主菜单直达开关；打开瞬间立即清除**当前**通缉（警察立刻停止追捕），之后每 0.5 秒维持 |
| **荣誉值锁定最高** | 每帧 `SET_PLAYER_CURRENT_HONOR(100)` 锁满 |
| **金钱** | 双路写入：游戏原生 `UI_SET_MONEY` + 内存直写全局变量（带 `IsWritable` 可写性校验，防写坏内存崩溃） |
| **杂项** | 传送至地图标记点、时间缩放（子弹时间/加速）、荣誉值、恶名值、重新加载配置 |

菜单与 UI 
暂时没有中文

---

## 📦 安装

把以下文件放到**和 `RDR.exe` 同级**的游戏根目录：

```
winmm.dll        <- RedHook 加载器（v0.8）
RedHook.dll      <- RedHook 本体
RedHook.ini      <- RedHook 配置
RDRTrainer.red   <- 本修改器
```

启动游戏即可，RedHook 会自动加载同目录下的 `.red` 插件。运行后在游戏目录生成：

- `RDRTrainer.ini` — 全部设置与热键（删除即恢复默认）
- `RDRTrainer.log` — 运行日志，出问题时把它发我

> 没有上面的 RedHook 运行时文件？它们在 `dist/RedHook/` 里有一份副本。

---

## ⌨ 按键

| 按键 | 作用 |
| --- | --- |
| `F7` | 打开/关闭菜单（可在游戏内改） |
| `F1` | 无敌 |
| `F2` | 无限子弹 |
| `F3` | 自瞄 |
| `F4` | 敌人透视 |
| `F6` | 金钱 +（数额可设） |
| `F9` | 传送到地图标记点 |
| `↑` `↓` | 菜单上下选择（小键盘 8/2 也可） |
| `←` `→` | 调整数值 / 切换（小键盘 4/6 也可） |
| `Enter` | 确认 / 进入子菜单 / 切换开关 |
| `Backspace` | 返回上一层（主菜单按则关闭菜单） |
| 右键 | 自瞄触发键（默认"按住瞄准键时"模式） |

所有热键都可在菜单里按 `Enter` 后直接按新键重新绑定。

---

## 🛠 编译

### 环境依赖

| 依赖 | 说明 |
| --- | --- |
| **Visual Studio 2022** | 需要 **v143** 平台工具集（vcxproj 里 `<PlatformToolset>v143</PlatformToolset>`）。VS 2019 及更早版本不行。 |
| **工作负载**：使用 C++ 的桌面开发 | 安装 VS 时勾选「Desktop development with C++」，否则没有 MSBuild C++ 工具链和 C++ 编译器。 |
| **Windows 10/11 SDK** | vcxproj 里 `<WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>`，装任意一个 Windows 10 SDK 或 Windows 11 SDK 都行。 |
| **不需要额外第三方库** | RedHook 的导入库 `RedHook.lib` 已随项目放在 `src/SDK/` 下，链接器直接用。ImGui 当前未参与编译（只是源码保留在 `third_party/`）。 |

> 命令行编译需要 MSBuild 在 PATH 里，或用 VS 自带的「Developer Command Prompt for VS 2022」。`build.bat` 会用 `vswhere.exe` 自动定位 MSBuild，无需手动配置。

### 编译命令

```bat
:: 方式一：双击（自动找 MSBuild）
build.bat

:: 方式二：命令行
MSBuild RDR修改器.sln /p:Configuration=Release /p:Platform=x64

:: 方式三：Visual Studio 打开 RDR修改器.sln，选 Release | x64，生成
```

### ⚠️ 手动修改游戏目录

`RDR修改器.vcxproj` 第 53 行写死了编译后自动复制的目标目录：

```xml
<GameDir>F:\RDR\RDR\</GameDir>
```

编译结束后的 post-build 步骤会把 `RDRTrainer.red` 复制到 `$(GameDir)`（**前提是该目录里有 `RDR.exe`**）。**换电脑或游戏装在别的盘符时，必须改成你的实际游戏目录**，例如：

```xml
<GameDir>D:\Games\Red Dead Redemption\</GameDir>
```

改完重新编译，`.red` 就会自动落到新目录。不改的话产物只会留在 `build/Release/`，你得手动复制到游戏目录。

### 产物位置

```
RDR修改器/build/Release/RDRTrainer.red    <- 编译输出
F:\RDR\RDR\RDRTrainer.red                  <- 自动复制到这里（GameDir 改对的话）
```

---

## 📁 目录结构

```
RDR修改器/
├── RDR修改器.sln / .vcxproj        项目文件（x64 DLL，输出 .red）
├── build.bat                       一键编译
├── README.md                       本文件
├── src/
│   ├── SDK/                        RedHook SDK（RedHook.h / Natives.h / Structs.h /
│   │                               RhMath.h / NativesCaller.h / RedHook.lib）
│   ├── Core/                       工具层：配置、日志、输入、屏幕、字体校准、文本池、游戏状态
│   ├── UI/                         界面层：主题、绘制封装、菜单、透视、提示
│   └── Features/                   功能层：GodMode / InfiniteAmmo / Aimbot / Money / Misc
├── third_party/imgui/              Dear ImGui 源码（备用，当前未启用渲染）
└── dist/RedHook/                   RedHook v0.8 官方运行时副本
```

---

## 🔧 技术说明

### 为什么不用 ImGui 画 UI？
RDR1 PC 版用 NVIDIA Streamline（`sl.interposer.dll`）包装了 DXGI 交换链，想画 ImGui 得 hook DX12 Present + 队列 + 描述符堆，稳定性和兼容性风险大得多。改用 RedHook 自带的 `RH_DrawText` / `RH_DrawRect`（0~1 归一化坐标），**永远不会和游戏渲染器打架**。ImGui 源码保留在 `third_party/` 作为备用升级路径。

### 字体大小自动校准
RedHook 的 `RH_DrawText` 字体 scale 单位未文档化，1.0 会撑满整个屏幕。`Core/Font.cpp` 用 `RH_GetTextWidth` 测量一次，推导出约 9px/字符的 scale，并加 1/70 屏幕宽的硬上限，**单个字符永远不会霸占屏幕**。


### 崩溃防护
- 每帧用 SEH（`__try/__except`）包裹，连续 10 次异常自动停用修改器；
- 自瞄 native 调用加**状态缓存**（不每帧重复调同一 native）；
- 无限子弹**只对手持武器**操作（不盲遍历武器 ID，避免无效 ID 崩游戏）；
- 金钱内存写入前用 `VirtualQuery` 校验页可写性（`IsWritable`），只读段/代码段直接跳过。

### 通缉清除
RDR1 的通缉系统靠 AI 记忆 + 恶名驱动。本修改器每 0.5 秒执行：
- `MEMORY_EVERYBODY_FORGET_ABOUT(玩家)` — 所有 AI（含警察）立刻忘记你，追捕队解散；
- `AI_GLOBAL_CLEAR_ALL_DANGER()` — 清全局威胁；
- `SET_ACTOR_TIME_OF_LAST_CRIME(0)` + `SET_PLAYER_CURRENT_NOTORIETY(0)` — 无活跃犯罪/恶名；
- `WANTEDFX_SET_LEVEL(0)` — 隐藏屏幕星星。


打开开关的**瞬间立即执行一轮**，正在被通缉也能立即解围。
（有时候这个功能会失效）

---

## ⚠️ 已知限制 / 需实测项

本修改器按 RedHook 官方文档 + RDR1 原生函数表 + 社区实现编写并编译通过，但部分功能依赖游戏实测：

1. **中文菜单**：取决于 RedHook 字体 0–12 中是否有含 CJK 字形的编号。RDR1 PC 版自带中文字体，但具体映射需运行时探测（日志会记录 `auto-selected CJK-capable font id X` 或 `no CJK-capable font found`）。
2. **不会被通缉**：用了 `MEMORY_EVERYBODY_FORGET_ABOUT` 等清除 AI 记忆的 native；如果 PC 版上这些 native 不生效，警察可能仍追——反馈后可换 `SET_FACTION_STATUS_TO_INDIVIDUAL_ACTOR` 路线。
3. **荣誉值范围**：用 `SET_PLAYER_CURRENT_HONOR(100)`，RDR1 内部刻度推测为 0–100。
4. **金钱全局编号** `3391` 来自 PS3/360 脚本全局表，PC 版一般一致但不保证；菜单里可直接改。
5. **敌对判定** `AI_IS_HOSTILE_OR_ENEMY` 参数顺序未文档化，提供三种模式（两种 native 参数序 + "全部视为敌人"）。
6. **杀毒软件报毒**：`winmm.dll` 是 DLL 劫持式加载器，普遍被误报，建议把游戏目录加入白名单。

---

## 📜 许可与声明

- RedHook SDK：MIT（K3rhos © 2024）
- Dear ImGui：MIT
- 本修改器代码：自由使用与修改

**仅用于单机游戏体验**（RDR1 PC 版无官方多人模式）。请支持正版游戏。
