# STM32F103 FreeRTOS 练习

本工程用于在 STM32F103C8 上分阶段练习 FreeRTOS。

工程最初复制自一个 OLED、MPU6050 和多传感器综合项目。旧驱动源码仍保留在
`User` 和 `SYSTEM` 目录中，便于后续参考，但第一阶段不参与 Keil 编译。

## 第一阶段目标

当前只运行两个任务：

| 任务 | 优先级 | 周期 | 功能 |
| --- | ---: | ---: | --- |
| `LED_Task` | 2 | 500 ms | 红、黄、绿 LED 依次点亮 |
| `UART_Test_Task` | 1 | 1000 ms | USART1 周期输出测试文本 |

两个任务都使用 `vTaskDelay()` 主动进入阻塞状态，用于观察最基本的任务创建、
优先级和延时调度。

## 硬件与工具

- MCU：STM32F103C8
- 内核：Arm Cortex-M3
- Flash：64 KB
- SRAM：20 KB
- 开发环境：Keil MDK，ARMCC 5
- 外设库：STM32F10x Standard Peripheral Library

## FreeRTOS 内核

工程使用官方 `FreeRTOS-Kernel V11.3.0`：

- 官方仓库：<https://github.com/FreeRTOS/FreeRTOS-Kernel>
- 版本页面：<https://github.com/FreeRTOS/FreeRTOS-Kernel/releases/tag/V11.3.0>
- 下载地址：<https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/V11.3.0.zip>
- 下载文件 SHA-256：
  `9EF2600D5F9DD009D29AE80588B8987DA5A25F626332D054900E60514868730F`

项目内使用的主要文件：

```text
Middlewares/FreeRTOS-Kernel/
├── include/
├── portable/
│   ├── MemMang/heap_4.c
│   └── RVDS/ARM_CM3/
│       ├── port.c
│       └── portmacro.h
├── list.c
├── queue.c
└── tasks.c
```

主要配置：

- 抢占式调度
- 系统节拍：1000 Hz
- 动态内存管理：`heap_4.c`
- FreeRTOS 堆：8 KB
- 软件定时器：关闭
- Tickless 低功耗：关闭
- Cortex-M3 移植层：`portable/RVDS/ARM_CM3`

## LED 接线

| 颜色 | STM32 引脚 |
| --- | --- |
| 红灯 | PA5 |
| 黄灯 | PA6 |
| 绿灯 | PA7 |

当前驱动为高电平点亮。运行后应看到红、黄、绿每隔 500 ms 依次切换。

## USART1

| 信号 | STM32 引脚 |
| --- | --- |
| TX | PA9 |
| RX | PA10 |
| GND | 与 USB-TTL 共地 |

串口参数：

```text
115200 baud
8 data bits
1 stop bit
No parity
```

每隔 1000 ms 输出：

```text
FreeRTOS UART task running
```

## 代码入口

- `User/main.c`：硬件初始化、任务创建、启动调度器
- `User/FreeRTOSConfig.h`：FreeRTOS 工程配置
- `User/rtos_tasks.c`：`LED_Task` 和 `UART_Test_Task`
- `User/led.c`：交通灯 GPIO 驱动
- `SYSTEM/usart/usart.c`：USART1 初始化和中断入口
- `SYSTEM/usart/uart_tx.c`：USART1 中断发送缓冲区

FreeRTOS 接管以下 Cortex-M3 异常：

- `SVC_Handler`
- `PendSV_Handler`
- `SysTick_Handler`

因此第一阶段不再编译旧 `delay.c` 和 `timing.c`，避免它们继续占用 SysTick
或 TIM3。

## Keil 工程

打开：

```text
Project/led.uvprojx
```

第一阶段参与编译的分组：

- `CMSIS`
- `FWLIB`
- `User`
- `SYS`
- `FreeRTOS`

下载运行后，同时检查 LED 切换和 USART1 周期文本。两种现象都正常，说明任务
创建、系统节拍、上下文切换和基础外设调用已经工作。
