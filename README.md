# ECG Monitor (心电监护仪上位机)

![C++17](https://img.shields.io/badge/C++-17-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.x-brightgreen.svg)
![SQLite](https://img.shields.io/badge/SQLite-3-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

基于 C++17 和 Qt6 开发的医疗级心电监护仪上位机软件。本项目采用严格的**多线程解耦架构**，实现了高频生理波形（ECG/SpO2/RESP）的丝滑渲染、自定义二进制协议的稳定解析、以及完善的声光报警与数据持久化系统。

## 🌟 核心特性 (Core Features)

* **🎨 医疗级 UI 设计**：深度定制 QSS，实现暗黑高对比度医疗风格，支持波形冻结与多体征独立闪烁报警。
* **🚀 高性能波形渲染**：基于 `QCustomPlot`，通过禁用强制即时重绘（`phImmediateRefresh`）、关闭多边形抗锯齿及批量数据投递，实现 3 路 100Hz 波形 25FPS 丝滑平移滚动，CPU 占用极低。
* **🧵 四线程解耦架构**：采用 `QThread` + `moveToThread` 模型，将 UI渲染、串口通信、音频播放、SQLite/日志存储 彻底物理隔离，杜绝界面卡顿。
* **📡 健壮的通信解析**：设计双通道分包协议（高频波形包 + 低频参数包）。底层基于**环形缓冲区 + 有限状态机 (FSM)**，完美解决串口通信中的“粘包”与“半包”问题。
* **💾 异步持久化存储**：
  * **数据库**：SQLite 开启 `WAL` 模式，实现无锁并发读写。后台线程 1Hz 记录体征数据，支持一键导出 24 小时 CSV 报表。
  * **全局日志**：通过 `qInstallMessageHandler` 挂载全局钩子，实现 `qDebug` 日志的无感异步落盘与 7 天自动滚动清理。

## 🏗️ 软件架构 (Architecture)

系统由四个核心线程独立运作，通过 Qt 信号槽（`Qt::QueuedConnection`）与线程安全队列（`ThreadSafeQueue`）进行数据交互：
1. **Main Thread (UI)**：负责 QCustomPlot 渲染、用户交互、25Hz 定时器数据抽取与报警逻辑判定。
2. **Network Thread**：负责 `QSerialPort` 数据接收与状态机解包，将有效载荷压入线程安全队列。
3. **Audio Thread**：利用 `QSoundEffect` 异步处理 R 波心跳音与脱落/超限报警音，避免 WASAPI 底层阻塞主线程。
4. **Storage Thread**：负责 SQLite 数据库的定时 INSERT 操作与 `.log` 文件的追加写入。

## 🛠️ 编译与运行 (Build & Run)

### 环境依赖
* Qt 6.x (需包含 `Core`, `Gui`, `Widgets`, `SerialPort`, `Multimedia`, `Sql` 模块)
* CMake 3.16+
* MSVC 2019+ 或 MinGW

### 运行测试 (软硬件在环仿真)
本项目自带下位机模拟器，可在无真实硬件环境下进行完整测试：
1. 安装虚拟串口软件（如 com0com 或 VSPD），创建一对虚拟串口（如 `COM1` ↔ `COM2`）。
2. 运行编译生成的 `ECG_Simulator.exe`，连接 `COM2` 并开启推流。
3. 运行主程序 `ECG_Monitor.exe`，连接 `COM1`，即可看到实时波形。

## 📸 界面截图 (Screenshots)
*(建议在此处插入 1-2 张你的软件运行截图，展示波形和报警状态)*
