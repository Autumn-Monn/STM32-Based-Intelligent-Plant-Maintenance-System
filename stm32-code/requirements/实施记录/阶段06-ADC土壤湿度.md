# 阶段06：ADC 土壤湿度

## 本阶段目标

完成土壤湿度传感器的 ADC 采样验证，建立“单次采样 + 多次均值 + 串口打印”的最小模拟量输入链路，为后续水泵联动和 OLED 显示提供湿度输入。

## CubeMX 配置

- 开启 `ADC1`
- `PA0` 配置为 `ADC1_IN0`
- 扫描模式关闭
- 连续转换关闭
- 触发源为 `Software Start`
- 数据右对齐
- 常规通道数为 `1`
- 当前采样时间为 `239.5 cycles`
- ADC 外设时钟由 `SystemClock_Config()` 中配置为 `RCC_ADCPCLK2_DIV6`

## AI 指令

- 新增独立 `soil` 模块，封装土壤湿度采样逻辑
- 先做最小单通道 ADC 读取，不引入校准、百分比换算等复杂处理
- 保留“原始值”和“均值”两种接口，方便后续调试和显示
- 阶段 6 先通过串口命令触发打印，避免主循环持续刷屏
- 采样逻辑保持简单可靠，优先为后续阶段提供基础输入

## 代码放置位置

- [Core/Inc/adc.h](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/Core/Inc/adc.h)
  - CubeMX 生成 `ADC1` 初始化声明
- [Core/Src/adc.c](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/Core/Src/adc.c)
  - 配置 `ADC1` 单通道软件触发采样
- [Core/Src/main.c](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/Core/Src/main.c)
  - 调用 `MX_ADC1_Init()` 与 `soil_init()`
  - 通过串口命令 `s` 调用 `soil_stage6_demo()`
- [User/Inc/soil.h](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/User/Inc/soil.h)
  - 定义原始值读取、均值读取与阶段 6 演示接口
- [User/Src/soil.c](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/User/Src/soil.c)
  - 实现 `soil_read_raw()`、`soil_read_avg()` 与串口打印演示逻辑
- [CMakeLists.txt](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/CMakeLists.txt)
  - 将 `User/Src/soil.c` 加入构建

## 编写逻辑

- `soil_read_raw()` 每次按以下顺序完成一次采样：
  - `HAL_ADC_Start()`
  - `HAL_ADC_PollForConversion()`
  - `HAL_ADC_GetValue()`
  - `HAL_ADC_Stop()`
- `soil_read_avg(samples)` 通过循环调用 `soil_read_raw()` 求平均值
- 当 `samples == 0` 时，自动回退为 1 次采样，避免除零
- `soil_stage6_demo()` 用 `HAL_GetTick()` 做 1 秒节流，避免串口打印过于密集
- 串口输出同时保留 `raw` 和 `avg`，便于观察瞬时波动和均值效果

## 问题与修正

- 本阶段先保留 ADC 原始值，不急于映射成“百分比湿度”，避免在校准数据不足时产生误导
- 采样流程当前为阻塞式轮询，便于验证；后续若主循环负载上升，再考虑 DMA 或定时器触发
- 当前均值算法采用简单平均，优先保证实现透明和可调试
- `soil_stage6_demo()` 采用命令触发 + 内部节流，避免每次进入主循环都无节制打印

## 验收结果

- 工程已具备读取 `PA0 / ADC1_IN0` 原始模拟值的能力
- 串口命令 `s` 可输出土壤湿度原始值与均值
- 代码已沉淀为 `soil` 模块，后续可直接被 OLED、数据存储和联动控制复用
- 本阶段目标已达成，满足进入阶段 7 和阶段 9 的输入准备条件
