# 阶段05：UART 调试

## 本阶段目标

完成 `USART1` 调试链路打通，建立“上电打印 + 串口接收命令 + 主循环消费命令”的最小交互能力，为后续 I2C、DS18B20、联动控制等阶段提供统一调试入口。

## CubeMX 配置

- 开启 `USART1`
- `PA9` 配置为 `USART1_TX`
- `PA10` 配置为 `USART1_RX`
- 波特率设置为 `115200`
- 数据格式为 `8N1`
- 模式为 `TX/RX`
- 不使用硬件流控
- 启用 `USART1 global interrupt`
- `USART1_IRQn` 优先级当前为 `0`

## AI 指令

- 新增独立 `debug_uart` 模块，不把串口收发逻辑长期堆在 `main.c`
- 发送路径先采用阻塞发送，优先保证调试输出稳定
- 接收路径采用中断 + 环形缓冲区，避免主循环轮询寄存器
- 主循环按“有数据就取 1 字节命令”的方式消费串口输入
- 先完成最小调试链路，再逐步接入阶段 6 之后的业务命令

## 代码放置位置

- [Core/Inc/usart.h](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/Core/Inc/usart.h)
  - CubeMX 生成 `USART1` 初始化声明
- [Core/Src/usart.c](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/Core/Src/usart.c)
  - 配置 `USART1` 波特率、GPIO 与中断
- [Core/Src/stm32f1xx_it.c](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/Core/Src/stm32f1xx_it.c)
  - 保留 `USART1_IRQHandler()` 并转入 HAL
- [Core/Src/main.c](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/Core/Src/main.c)
  - 调用 `MX_USART1_UART_Init()`、`debug_uart_init()`
  - 在 `HAL_UART_RxCpltCallback()` 中转发到 `debug_uart_rx_callback()`
  - 在主循环中通过 `debug_uart_has_data()` / `debug_uart_read_byte()` 读取命令
- [User/Inc/debug_uart.h](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/User/Inc/debug_uart.h)
  - 定义发送、接收、回调接口
- [User/Src/debug_uart.c](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/User/Src/debug_uart.c)
  - 实现阻塞发送、中断接收、16 字节环形缓冲区与数字打印接口
- [CMakeLists.txt](e:/AI-Projects/STM32-Based-Intelligent-Plant-Maintenance-System/stm32-code/CMakeLists.txt)
  - 将 `User/Src/debug_uart.c` 加入构建

## 编写逻辑

- `debug_uart_init()` 上电后立即调用 `HAL_UART_Receive_IT()` 启动 1 字节中断接收
- 发送路径统一封装为：
  - `debug_uart_send_bytes()`
  - `debug_uart_send_string()`
  - `debug_uart_send_line()`
  - `debug_uart_send_number()`
- 接收中断只做两件事：
  - 把收到的字节写入环形缓冲区
  - 重新挂起下一次 `HAL_UART_Receive_IT()`
- 主循环通过 `while (debug_uart_has_data())` 消费命令，避免在中断中直接执行业务逻辑
- 当前阶段形成的最小串口协议后来被继续扩展，用于 LED、继电器、ADC、EEPROM 等后续阶段验证

## 问题与修正

- 为了尽快得到稳定调试链路，发送路径没有一开始就做 DMA，而是先用阻塞发送
- 接收缓存采用固定 16 字节环形队列，避免引入动态内存
- 中断回调中不直接解析命令，只做“收字节 + 续接收”，避免 ISR 过重
- `HAL_UART_RxCpltCallback()` 被集中放在 `main.c` 的用户区实现，便于和当前项目的其他用户回调保持一致

## 验收结果

- 串口助手以 `115200 8N1` 连接后，可看到启动信息 `=== Plant System Boot ===`
- 工程上电后串口收发稳定，无明显异常复位
- 主循环已具备接收单字节命令并执行动作的基础能力
- 本阶段完成后，串口成为后续各阶段的统一调试入口，满足进入阶段 6 的要求
