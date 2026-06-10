# FreeRTOS 最小工程第一阶段

日期：2026-06-10

## 修改目标

把复制来的 STM32F103 多传感器工程收缩为第一阶段 FreeRTOS 最小练习工程，
只运行 LED_Task 和 UART_Test_Task，并加入官方 FreeRTOS-Kernel V11.3.0。

## 修改前行为

主循环依次轮询光敏、温度、MPU6050、姿态串口、摇杆和 OLED UI 等模块。
工程尚未包含 FreeRTOS 内核，旧 delay.c 使用 SysTick，timing.c 使用 TIM3。

## 修改后行为

main.c 初始化 NVIC、交通灯 LED 和 USART1，创建两个任务后启动 FreeRTOS。
LED_Task 每 500 ms 切换红、黄、绿 LED；UART_Test_Task 每 1000 ms 通过
USART1 输出 `FreeRTOS UART task running`。FreeRTOS 接管 SVC、PendSV 和
SysTick 异常。旧传感器源码保留在磁盘，但不参与当前 Keil 编译。

## 逻辑变化范围

- 加入官方 FreeRTOS-Kernel V11.3.0 的任务、列表、队列、ARM_CM3 移植层和
  heap_4 动态内存管理。
- 增加 1000 Hz 抢占式调度配置，FreeRTOS 堆大小为 8 KB。
- 新增 LED_Task、UART_Test_Task、内存分配失败钩子和栈溢出钩子。
- 删除应用中断文件内原有的空 SVC、PendSV、SysTick 处理和 TIM3 节拍处理，
  避免与 FreeRTOS 移植层冲突。
- Keil 工程只编译第一阶段所需的 CMSIS、RCC、GPIO、USART、应用任务和
  FreeRTOS 内核文件。

## 涉及文件

- `Middlewares/FreeRTOS-Kernel/`：官方 FreeRTOS V11.3.0 内核、头文件、
  ARM_CM3 移植层、heap_4 和许可证。
- `User/FreeRTOSConfig.h`：STM32F103C8 的 FreeRTOS 配置。
- `User/rtos_tasks.c`、`User/rtos_tasks.h`：两个练习任务和错误钩子。
- `User/main.c`：最小硬件初始化、任务创建和调度器启动。
- `User/stm32f10x_it.c`：释放 FreeRTOS 使用的系统异常入口。
- `Project/led.uvprojx`：精简编译组并加入 FreeRTOS。
- `README.md`：改为当前第一阶段工程说明。

## 接口与兼容性

- USART1 保持 PA9/PA10、115200、8N1。
- 交通灯保持 PA5/PA6/PA7，高电平点亮。
- SVC_Handler、PendSV_Handler、SysTick_Handler 改由 FreeRTOS port.c 提供。
- 旧 OLED、MPU6050、光敏、温度和摇杆源码未删除，但不参与本阶段固件。

## 编码与注释规范

- C/H：GB2312 兼容代码页 936，无 BOM。
- 文档及脚本：UTF-8，无 BOM。
- 函数前详细注释，函数内仅保留关键注释。
- 宏、枚举和结构字段使用对齐的行尾 // 注释。

## 验证结果

- 官方压缩包 SHA-256：
  `9EF2600D5F9DD009D29AE80588B8987DA5A25F626332D054900E60514868730F`。
- Keil 隔离编译：0 Error(s)，0 Warning(s)。
- 程序大小：Code=5104，RO-data=300，RW-data=148，ZI-data=9836。
- 链接映射确认 SVC_Handler、PendSV_Handler、SysTick_Handler 来自 port.o。
- 链接映射确认 LED_Task 和 UART_Test_Task 已进入最终固件。
- STM32 文本编码策略检查通过。
- 新增/修改函数注释策略检查通过。
- `git diff --check` 通过。

## Git

- 分支：main
- 起始提交：e92b13c28b50557a02e72aa1edf2fc8b827ad90a
- Commit：this commit
- 提交说明：Add minimal FreeRTOS stage one
