# 构建与运行

核心和命令行入口使用 C++17，不依赖 EasyX。图形入口使用 Windows、MSVC 和 EasyX，保留课程程序的菜单与绘图。

## 构建核心

需要 CMake 3.20 及以上，以及 C++17 编译器。在仓库根目录执行：

```sh
cmake -S . -B build -DBUILD_GUI=OFF
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Visual Studio 等多配置生成器通常将可执行文件放在 `build/Release/`；Ninja、Makefiles 等单配置生成器通常放在 `build/`。单配置构建可在配置时增加 `-DCMAKE_BUILD_TYPE=Release`。

## 命令行运行

以下示例使用 Windows PowerShell 与多配置输出路径。单配置构建请将路径换成 `./build/WarehouseTransportCLI`，Windows 单配置构建则换成 `.\build\WarehouseTransportCLI.exe`。

```powershell
.\build\Release\WarehouseTransportCLI.exe --mode schedule --cars 3 --seed 42 --max-seconds 10800 --output outputs/schedule-3
```

巡游模式仅使用三辆车，通过仿真时长限制结束运行：

```powershell
.\build\Release\WarehouseTransportCLI.exe --mode patrol --cars 3 --seed 42 --max-seconds 600 --output outputs/patrol-3
```

查看参数帮助：

```powershell
.\build\Release\WarehouseTransportCLI.exe --help
```

| 参数 | 用途 |
| --- | --- |
| `--mode schedule` 或 `--mode patrol` | 调度或巡游，默认调度 |
| `--cars 3`、`5`、`7` | 调度车辆数，默认 3；巡游仅允许 3 |
| `--seed 42` | 随机种子，默认 1 |
| `--max-seconds 10800` | 最多推进的仿真秒数，默认 10800；不是实际等待时间 |
| `--time-step 0.05` | 每步推进的仿真秒数，默认 0.05 |
| `--output outputs/schedule-3` | 运行输出目录，默认 `outputs` |
| `--no-logs` | 关闭任务、设备、运行日志和分析文本；仍保存生效配置 |
| `--snapshot outputs/state.json` | 保存订单时间线及车辆结束状态摘要，不含所有站点和队列 |
| `--verbose` | 向控制台显示详细运行日志；默认不刷详细日志 |

调度在订单全部完成或达到时间上限时结束；巡游在达到时间上限时结束。退出码 `0` 表示调度完成或巡游按限制结束，`2` 表示调度到时仍未完成，`1` 表示参数或输入输出错误。巡游退出码不用于证明车辆满足完整一圈的课程要求，见[限制说明](LIMITATIONS.md)。

时长与步长必须为有限正数，时长至少覆盖一个时间步。`max-seconds / time-step` 不能超过一亿步；超出时程序拒绝运行并返回 `1`。改变时间步长会改变离散运动和到站判断，比较运行时应保持一致。

需要少量输出并保存最终状态时：

```powershell
.\build\Release\WarehouseTransportCLI.exe --mode schedule --cars 3 --seed 42 --max-seconds 10800 --output outputs/schedule-3 --no-logs --snapshot outputs/state.json
```

比较 3、5、7 辆车时分别运行，保持种子与步长一致，输出目录分开：

```powershell
foreach ($cars in 3, 5, 7) {
    .\build\Release\WarehouseTransportCLI.exe --mode schedule --cars $cars --seed 42 --max-seconds 10800 --output "outputs/schedule-$cars"
}
```

## 输出目录

程序以 UTF-8 保存文本。`runtime-config.json` 记录最终生效配置，是重复运行时应先核对的文件。默认开启日志，同名输出可能在下一次运行时重写，建议为不同实验指定不同目录。

| 文件 | 内容 |
| --- | --- |
| `runtime-config.json` | 模式、车辆数、种子、时间步长、输出等生效参数 |
| `TaskExeLog3units.txt` 等 | 任务编号、物料、类型、起终设备、准备/取货/放货/清货时间 |
| `DeviceStateLog3units.txt` 等 | 设备货物状态变化；穿梭车设备编号从 101 起 |
| `SystemLog_*.txt` | 系统运行事件 |
| `SystemAnalysisReport_Schedule_3units.txt` 等 | 调度运行统计 |
| `PatrolAnalysisReport.txt` | 巡游运动阶段与车辆统计 |
| `--snapshot` 指定的 JSON | 订单时间线及车辆结束状态摘要；不包含完整站点或队列状态 |

任务与设备日志采用制表符分隔。车辆 1 对应设备号 101，车辆 2 对应 102，以此类推；站点设备号仍为 1—18。巡游不执行货物订单，任务日志不包含运输记录。

`outputs/`、`build/`、`build-gui/`、`bin/` 及 IDE 缓存由 Git 忽略。自定义输出目录时应放在已忽略目录下。仓库只保留源码、构建说明和演示素材。

## 原版算法对照

如果本地保留了原课程单文件，可以用 [`tools/compare_legacy.py`](../tools/compare_legacy.py) 检查模块拆分前后的行为。仓库不包含原始作业源码，对照时请自行提供本地文件；脚本只读原文件，生成的探针、日志和 JSON 写入忽略的构建目录。

对照需要 Windows、MSVC、EasyX 和 Python 3。先保持 `BUILD_TESTING=ON`（默认开启），构建模块化探针：

```powershell
cmake --build build --config Release --target warehouse_parity_probe
python tools/compare_legacy.py --legacy-source "D:/Course/warehouse_automated_transport.cpp" --probe build/Release/warehouse_parity_probe.exe --seed 42 --work-dir build/legacy-parity
```

将 `--legacy-source` 的示例路径替换为实际文件位置。若没有自动找到 EasyX，增加 `--easyx-root "D:/Libraries/EasyX"`；若 CMake 不在命令搜索路径中，可用 `--cmake "<cmake可执行文件>"` 指定。

脚本会编译原版探针，在关闭图形特效的条件下对照 3、5、7 车调度和三车巡游，检查多个时刻的订单、车辆、站点及队列状态，并生成 `build/legacy-parity/comparison-report.json`。这是拆分行为一致性的验证，不是课程所有物理约束的验证；原版与新版都未完成的订单也会如实记录。

## 构建 Windows 图形界面

安装 Visual Studio 2022 的 C++ 桌面开发工具和 EasyX。安装方式见 [EasyX 官方说明](https://easyx.cn/setup)，手动部署见 [EasyX 文档](https://docs.easyx.cn/zh-cn/manual-setup)。

```powershell
cmake -S . -B build-gui -G "Visual Studio 17 2022" -A x64 -DBUILD_GUI=ON
cmake --build build-gui --config Release
.\build-gui\Release\WarehouseTransportSystem.exe
```

如果自动查找失败，请指定包含头文件和库文件的 EasyX 根目录：

```powershell
cmake -S . -B build-gui -G "Visual Studio 17 2022" -A x64 -DBUILD_GUI=ON -DEASYX_ROOT="D:/Libraries/EasyX"
```

将示例目录替换为实际安装位置，根目录应包含 `include/graphics.h` 和适配 x64 的 `EasyXa.lib`（通常位于 `lib/x64/`）。不要将 EasyX 安装包或库文件提交到本仓库。

## 图形界面操作

菜单支持鼠标点击；键盘快捷键由随程序启动的控制台读取。如果按键无响应，请将焦点切回控制台。

| 菜单操作 | 快捷键 |
| --- | --- |
| 开始 `BEGIN` | `1` |
| 切换巡游/调度 `TASK` | `2` |
| 调度车辆数按 3 → 5 → 7 循环 `CARS` | `3` |
| 退出 `EXIT` | `4` |

巡游菜单固定三辆车。运行时可以操作：

| 操作 | 快捷键 |
| --- | --- |
| 暂停或继续 | `P` |
| 增加或降低倍率 | `+` / `-`（`=` 也可增加） |
| 返回菜单 | `Q` |
| 切换模式并返回菜单 | `M` |
| 切换粒子、字符雨、光晕、动画 | `1` / `2` / `3` / `4` |
| 保存调试快照 | `D` |

界面倍率影响仿真推进的展示速度，运动逻辑仍采用配置的仿真时间步长。需要严格比较相同种子的轨迹时使用 CLI；旧渲染效果消耗共享随机数，见[随机数与复现](LIMITATIONS.md#随机数与复现)。

## 生成界面预览

在仓库根目录执行：

```powershell
.\build-gui\Release\WarehouseTransportSystem.exe --preview assets/simulation-preview.png
```

此命令输出程序界面的演示图片，供 README 展示。截图不用于替代算法结果或性能验证。
