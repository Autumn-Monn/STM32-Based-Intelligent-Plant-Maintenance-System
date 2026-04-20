# 阶段01：GPIO点灯

## 本阶段目标

完成阶段1的 GPIO 输出验证，打通板载 LED 与外接 LED 的点亮、熄灭和顺序闪烁链路，并将验证逻辑整理为独立模块，避免继续堆积在 `main.c` 中。

## CubeMX 配置

- `PC13` 配置为 `GPIO_Output`，标签为 `LED0`，用于板载 LED 验证。
- `PA8` 配置为 `GPIO_Output`，标签为 `LED_GREEN`，用于外接绿灯。
- `PB0` 配置为 `GPIO_Output`，标签为 `LED_RED`，用于外接红灯。
- `PB1` 配置为 `GPIO_Output`，标签为 `LED_BLUE`，用于外接蓝灯。
- `PA13/PA14` 保持 SWD 调试下载功能。
- 时钟保持 HSE + PLL 72MHz，不额外引入其他外设。

## AI 指令

本阶段围绕以下要求与 AI 协作：

- 阶段1点灯功能迁移到 `User` 目录中独立保存。
- 函数命名统一使用 `led_xxx`，不使用 `app_led_xxx`。
- 保持当前阻塞式顺序闪灯逻辑不变，先保证行为与 `main.c` 验证版本一致。
- 同步更新 `CMakeLists.txt`，确保 `User/Inc` 和 `User/Src/led.c` 进入构建。

## 代码放置位置

- [`Core/Src/main.c`](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/Core/Src/main.c)
  仅保留初始化与阶段1演示入口，调用 `led_init()` 和 `led_stage1_demo()`。
- [`User/Inc/led.h`](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/User/Inc/led.h)
  定义 LED 枚举与模块接口。
- [`User/Src/led.c`](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/User/Src/led.c)
  封装 LED 引脚映射、亮灭控制、翻转控制和阶段1演示逻辑。
- [`CMakeLists.txt`](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/CMakeLists.txt)
  添加 `User/Src/led.c` 源文件，并保留 `User/Inc` 头文件路径。

## 编写逻辑

- 板载 LED `PC13` 为低电平点亮，外接 `PA8/PB0/PB1` 为高电平点亮，因此在 `led.c` 中增加统一的引脚映射表，分别记录每个 LED 的 `port`、`pin`、`on_state` 和 `off_state`。
- 阶段1仍采用阻塞式 `HAL_Delay(500)` 顺序闪灯，便于和最初在 `main.c` 中的验证行为保持一致。
- 将 GPIO 操作从 `main.c` 中抽离后，后续阶段可以继续在 `User` 目录扩展 `key`、`uart`、`relay` 等模块。

## 问题与修正

- 最初外接绿灯不亮，并不是代码问题，而是核心板在面包板上的插入方向反了，导致排针与预期连线位置不一致。
- 排查过程中确认：
  - `PA8` 电压能够按程序规律变化；
  - LED、本体电阻与二极管方向无损坏；
  - 纠正开发板插接方向后，外接 LED 可以正常响应。
- 在模块化整理时，保留了阶段1当前的阻塞式演示方案，避免过早引入非阻塞状态机，减少后续调试复杂度。

## 验收结果

- 板载 LED、绿灯、红灯、蓝灯均可独立点亮与熄灭。
- 顺序闪灯现象稳定可复现。
- 工程可成功编译，`User/Src/led.c` 已被 CMake 正常纳入构建。
- 阶段1代码已经从 `main.c` 迁移为独立 LED 模块，满足后续项目规划需要。

