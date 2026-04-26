# 阶段03：EXTI中断

## 本阶段目标

完成 `PB12 / KEY_1` 的 EXTI 中断验证，建立“中断置标志位，主循环处理动作”的最小事件驱动链路，并优化指示灯效果：

- 红灯与蓝灯以 0.5 秒为间隔交替闪烁
- `KEY_1` 按下后立即中断红蓝灯闪烁，绿灯常亮
- `KEY_1` 松开后绿灯熄灭，红蓝灯恢复到中断前的运行状态

## CubeMX 配置

- `PB12` 配置为 `GPIO_EXTI12`
- 触发模式为 `GPIO_MODE_IT_FALLING`
- 内部上拉为 `GPIO_PULLUP`
- 启用 `EXTI line[15:10] interrupts`
- `PB13~PB15` 继续保持普通输入上拉，不参与本阶段中断验证

## AI 指令

- 本阶段只做单键 `PB12` 的 EXTI 验证
- 中断中只做消抖判定和置标志位
- 主循环中消费标志位并驱动阶段3指示灯状态机
- 首次验证允许先放在 `main.c`
- 验证稳定后，将阶段3逻辑抽离为独立模块，便于后续阶段继续扩展

## 代码放置位置

- [`User/Inc/exti_demo.h`](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/User/Inc/exti_demo.h)
  - 提供 `exti_demo_init()`、`exti_demo_task()`、`exti_demo_gpio_exti_callback()`
- [`User/Src/exti_demo.c`](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/User/Src/exti_demo.c)
  - 保存阶段3的状态变量、红蓝灯交替状态机、KEY1 抢占显示与释放恢复逻辑
- [`Core/Src/main.c`](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/Core/Src/main.c)
  - 仅保留 `exti_demo_init()` 初始化调用
  - 在主循环中调用 `exti_demo_task()`
  - 在 `HAL_GPIO_EXTI_Callback()` 中转发到 `exti_demo_gpio_exti_callback()`
- [`Core/Src/stm32f1xx_it.c`](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/Core/Src/stm32f1xx_it.c)
  - 使用 CubeMX 生成的 `EXTI15_10_IRQHandler()` 和 `HAL_GPIO_EXTI_IRQHandler(KEY_1_Pin)`

## 编写逻辑

- `PB12` 按下时产生下降沿中断
- `exti_demo_gpio_exti_callback()` 中先检查 `GPIO_Pin == KEY_1_Pin`
- 使用 `HAL_GetTick()` 做 30ms 中断防抖
- 防抖通过后仅设置按下事件标志位
- `exti_demo_task()` 在主循环中维护一个最小状态机：
  - 正常状态下，红灯与蓝灯每 500ms 交替闪烁
  - 按下 `KEY_1` 后，保存红蓝灯当前状态并暂停闪烁
  - 进入抢占显示状态时，红蓝灯关闭，绿灯常亮
  - 松开 `KEY_1` 并稳定 30ms 后，绿灯熄灭，红蓝灯恢复之前状态并继续闪烁

## 问题与修正

- 重新生成代码前，`KEY_1_Pin` 宏未正确出现，根因是 `PB12` 未保留 `KEY_1` 标签
- 修正 `PB12` 标签并重新生成代码后，`main.h` 中已恢复 `KEY_1_Pin` / `KEY_1_GPIO_Port`
- 本阶段停用阶段2的 `key` 轮询路径，避免 `PB12` 同时被轮询和 EXTI 处理
- 阶段3硬件行为验证通过后，已将验证逻辑从 `main.c` 抽离到 `exti_demo` 模块，`main.c` 恢复为入口与调度层

## 验收结果

- 上电后系统运行稳定，无异常复位
- 红灯与蓝灯以 0.5 秒为间隔交替闪烁
- 按下并保持 `PB12 / KEY_1` 时，红蓝灯立即停止显示，绿灯保持常亮
- 松开 `PB12 / KEY_1` 后，绿灯熄灭，红蓝灯恢复到按下前的闪烁状态
- 快速连按与长按释放时，无明显因抖动导致的误亮或双灯同时点亮
- `HAL_GPIO_EXTI_Callback()` 与 `EXTI15_10_IRQHandler()` 可作为阶段3的断点验证入口
