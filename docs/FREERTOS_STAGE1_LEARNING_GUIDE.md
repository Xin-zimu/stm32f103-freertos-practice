# FreeRTOS 第一阶段学习指南

适用工程：STM32F103C8 FreeRTOS 练习  
适合读者：第一次接触 RTOS，已经了解一点 C 语言和 STM32 GPIO/串口基础  
本阶段任务：`LED_Task`、`UART_Test_Task`

---

## 1. 本阶段到底要学什么

这一阶段的目标不是一次学完 FreeRTOS，而是先建立一个最小、可观察、能验证的
RTOS 工程。

学完本阶段，你应该能够回答下面这些问题：

1. RTOS 和普通 `while (1)` 裸机程序有什么区别？
2. 什么是任务？
3. 怎样创建任务？
4. 调度器什么时候开始运行？
5. `vTaskDelay()` 为什么不会让整个单片机停止工作？
6. FreeRTOS 为什么要使用 SysTick、PendSV 和 SVC？
7. 每个任务为什么都需要自己的栈？
8. `xTaskCreate()` 使用的内存从哪里来？
9. 怎样判断 FreeRTOS 真的运行起来了？

本工程用两个简单现象回答这些问题：

- LED 每 500 ms 切换一次颜色。
- 串口每 1000 ms 输出一行文本。

如果两个现象可以同时持续发生，就说明调度器正在轮流运行两个任务。

---

## 2. 先理解裸机程序

没有 RTOS 时，常见程序结构如下：

```c
int main(void)
{
    Hardware_Init();

    while (1)
    {
        LED_Process();
        UART_Process();
        Sensor_Process();
        Display_Process();
    }
}
```

这种结构称为前后台系统或裸机轮询系统。CPU 按代码顺序依次调用每个处理函数。

它的优点是简单，适合功能很少的程序。缺点是所有功能互相影响：

- 一个函数执行时间过长，后面的函数就要等待。
- 一个函数内部使用长时间阻塞延时，整个主循环都会停在这里。
- 功能增多后，很难保证每个模块的执行周期。
- 每个模块都要自己判断时间，主循环容易越来越复杂。

例如：

```c
while (1)
{
    Traffic_RedOn();
    delay_ms(500);
    Traffic_GreenOn();
    delay_ms(500);
    printf("running\r\n");
}
```

`delay_ms(500)` 执行期间，CPU 一直困在延时函数中。此时主循环中的其他代码无法
继续执行。这种等待叫作忙等待或阻塞整个程序。

---

## 3. RTOS 程序的基本思路

使用 FreeRTOS 后，我们把不同功能写成不同任务：

```text
LED_Task
    控制 LED
    等待 500 ms
    再次控制 LED

UART_Test_Task
    发送串口文本
    等待 1000 ms
    再次发送文本
```

任务可以理解为由 FreeRTOS 管理的独立执行流程。每个任务都有：

- 自己的任务函数。
- 自己的栈。
- 自己的优先级。
- 自己的运行状态。
- 一个由内核管理的任务控制块，简称 TCB。

在单核 STM32F103 上，同一时刻实际上只有一个任务正在使用 CPU。FreeRTOS
快速保存当前任务的现场，再恢复另一个任务的现场，让它们交替执行。

这种切换速度很快，所以从人的观察角度看，LED 和串口像是在同时工作。

这叫并发，不是真正的多核并行。

---

## 4. 当前工程的整体结构

第一阶段真正需要关注的目录如下：

```text
rtos练习/
├── Project/
│   └── led.uvprojx
├── User/
│   ├── main.c
│   ├── FreeRTOSConfig.h
│   ├── rtos_tasks.c
│   ├── rtos_tasks.h
│   ├── led.c
│   ├── led.h
│   ├── stm32f10x_it.c
│   └── stm32f10x_it.h
├── SYSTEM/
│   ├── usart/
│   │   ├── usart.c
│   │   ├── usart.h
│   │   ├── uart_tx.c
│   │   └── uart_tx.h
│   └── sys/
│       └── sys.h
├── Libraries/
│   ├── CMSIS/
│   ├── inc/
│   └── src/
└── Middlewares/
    └── FreeRTOS-Kernel/
```

旧 OLED、MPU6050、光敏、温度和摇杆文件仍然保留，但没有加入当前 Keil 编译
列表。保留文件不等于这些功能正在运行。

---

## 5. 从上电到任务运行的完整流程

程序的启动顺序如下：

```text
STM32 复位
    ↓
启动文件设置栈并执行 SystemInit()
    ↓
进入 main()
    ↓
设置 NVIC 优先级分组
    ↓
初始化 LED GPIO
    ↓
初始化 USART1 和串口发送缓冲区
    ↓
创建 LED_Task
    ↓
创建 UART_Test_Task
    ↓
启动 FreeRTOS 调度器
    ↓
创建并运行 FreeRTOS 空闲任务
    ↓
LED_Task、UART_Test_Task 和空闲任务按状态切换
```

调用 `vTaskStartScheduler()` 以前，任务只是已经被创建，还没有正常开始调度。

调用成功以后，程序不会像普通函数那样返回到 `main()` 的下一行。CPU 的执行权
交给 FreeRTOS 调度器。

---

## 6. main.c：启动最小 RTOS 系统

文件：`User/main.c`

### 6.1 包含的头文件

```c
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led.h"
#include "usart.h"
#include "rtos_tasks.h"
```

各头文件的用途：

| 头文件 | 用途 |
| --- | --- |
| `stm32f10x.h` | STM32F103 寄存器、外设和基础类型定义 |
| `FreeRTOS.h` | FreeRTOS 基础配置、类型和公共宏 |
| `task.h` | 任务创建、延时和调度器相关 API |
| `led.h` | LED 驱动接口 |
| `usart.h` | USART1 初始化接口 |
| `rtos_tasks.h` | 本项目任务创建接口 |

### 6.2 设置中断优先级分组

```c
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
```

STM32F103 的中断优先级由 NVIC 管理。`NVIC_PriorityGroup_4` 表示把可用的优先级
位全部用于抢占优先级，不再分配子优先级。

FreeRTOS 在 Cortex-M3 上需要明确的中断优先级规则，因此在创建任务前统一设置
分组。后续学习“中断中调用 FreeRTOS API”时，这个配置非常重要。

### 6.3 初始化硬件

```c
LED_Init();
uart_init(115200);
```

这两步仍然属于普通 STM32 外设初始化，与是否使用 RTOS 没有本质区别。

RTOS 不会替你配置 GPIO、USART、I2C 或 ADC。FreeRTOS 负责的是任务和调度，
硬件驱动仍然需要自己编写或使用厂商库。

### 6.4 创建任务

```c
if (RTOS_Tasks_Create() != pdPASS)
{
    Traffic_AllOff();
    for (;;)
    {
    }
}
```

`RTOS_Tasks_Create()` 在 `rtos_tasks.c` 中创建两个任务。

返回 `pdPASS` 表示创建成功。创建失败通常意味着 FreeRTOS 堆空间不足，或者传入
参数不正确。

失败后进入死循环，是为了防止系统在任务不完整的情况下继续运行。调试时可以在
这个位置设置断点。

### 6.5 启动调度器

```c
vTaskStartScheduler();
```

这个函数完成的主要工作包括：

- 创建 FreeRTOS 空闲任务。
- 配置 SysTick 产生系统节拍。
- 设置 PendSV 和 SysTick 的中断优先级。
- 选择第一个要运行的任务。
- 恢复第一个任务的上下文。

调度器启动成功后不会返回。

后面的死循环只是一层故障保护：

```c
Traffic_AllOff();
for (;;)
{
}
```

如果执行到了这里，通常表示空闲任务创建失败，也就是内存不足。

---

## 7. rtos_tasks.c：创建两个任务

文件：`User/rtos_tasks.c`

### 7.1 任务参数

```c
#define LED_TASK_STACK_DEPTH        128
#define UART_TASK_STACK_DEPTH       128
#define LED_TASK_PRIORITY           2
#define UART_TASK_PRIORITY          1
#define LED_STEP_PERIOD_MS          500
#define UART_REPORT_PERIOD_MS       1000
```

这些宏把任务参数集中在文件顶部，方便观察和修改。

### 7.2 栈深度为什么是 128

`xTaskCreate()` 的栈深度单位不是字节，而是 `StackType_t` 的个数。

STM32F103 是 32 位 Cortex-M3，本移植层的 `StackType_t` 为 32 位，所以：

```text
128 个栈元素 × 4 字节 = 512 字节
```

因此：

- `LED_Task` 栈约为 512 字节。
- `UART_Test_Task` 栈约为 512 字节。

任务栈用于保存：

- 函数局部变量。
- 函数调用返回地址。
- CPU 寄存器现场。
- 中断或任务切换时需要保存的数据。

栈不是越大越好。太小会溢出，太大会浪费 STM32F103C8 仅有的 20 KB SRAM。

### 7.3 优先级

```c
#define LED_TASK_PRIORITY   2
#define UART_TASK_PRIORITY  1
```

FreeRTOS 中数值越大，任务优先级越高。

所以本工程中：

```text
LED_Task 优先级 2 > UART_Test_Task 优先级 1 > 空闲任务优先级 0
```

高优先级并不表示 LED 任务会一直运行。因为它会调用 `vTaskDelay()`，主动进入
阻塞状态。LED 任务阻塞后，低优先级串口任务才有机会运行。

这是学习 FreeRTOS 时非常重要的一点：

> 高优先级任务必须在没有工作时阻塞，否则可能饿死低优先级任务。

### 7.4 xTaskCreate() 参数

创建 LED 任务的代码：

```c
result = xTaskCreate(LED_Task,
                     "LED_Task",
                     LED_TASK_STACK_DEPTH,
                     0,
                     LED_TASK_PRIORITY,
                     0);
```

六个参数依次表示：

| 参数 | 当前值 | 含义 |
| --- | --- | --- |
| 任务函数 | `LED_Task` | 任务开始运行后执行的函数 |
| 任务名称 | `"LED_Task"` | 调试时显示的名称 |
| 栈深度 | `128` | 任务栈包含 128 个 32 位元素 |
| 参数指针 | `0` | 不向任务传入参数 |
| 优先级 | `2` | LED 任务优先级 |
| 句柄输出 | `0` | 当前不保存任务句柄 |

串口任务的创建方式相同，只是任务函数、名称和优先级不同。

### 7.5 为什么检查返回值

```c
if (result != pdPASS)
{
    return pdFAIL;
}
```

`xTaskCreate()` 需要动态分配：

- 任务控制块。
- 任务栈。

如果 FreeRTOS 堆不够，任务创建会失败。忽略返回值会导致“程序看似启动了，但
某个任务永远不运行”的问题。

---

## 8. LED_Task：学习任务循环和阻塞延时

任务函数的基本形式：

```c
static void LED_Task(void *argument)
{
    for (;;)
    {
        /* 完成一次任务工作 */
        vTaskDelay(...);
    }
}
```

### 8.1 为什么任务里使用无限循环

普通函数完成工作后可以返回，任务函数通常不能返回。

任务代表一个长期存在的功能。例如 LED 控制不是只执行一次，而是系统运行期间
持续执行，所以使用：

```c
for (;;)
{
}
```

如果需要结束任务，应该调用 FreeRTOS 提供的任务删除接口，而不是直接从任务
函数返回。本阶段关闭了任务删除功能，所以两个任务都会永久存在。

### 8.2 LED 状态变量

```c
uint8_t step;

step = 0;
```

`step` 是 LED 任务自己的局部变量，它保存在 LED 任务自己的栈中。

任务被切换出去后，这个变量不会丢失。下次任务重新运行时，仍然能继续使用原来
的值。

### 8.3 切换颜色

```c
if (step == 0)
{
    Traffic_RedOn();
}
else if (step == 1)
{
    Traffic_YellowOn();
}
else
{
    Traffic_GreenOn();
}
```

每次循环只点亮一种颜色。`Traffic_RedOn()` 等函数会先关闭全部 LED，再点亮目标
LED，因此不会同时亮多个灯。

### 8.4 vTaskDelay()

```c
vTaskDelay(pdMS_TO_TICKS(LED_STEP_PERIOD_MS));
```

`LED_STEP_PERIOD_MS` 为 500。

`pdMS_TO_TICKS(500)` 把 500 ms 转换为 FreeRTOS 节拍数。本工程节拍频率为
1000 Hz，即每 1 ms 产生一个 Tick，因此结果约为 500 Tick。

调用 `vTaskDelay()` 后：

1. LED 任务从运行态进入阻塞态。
2. 调度器选择其他就绪任务。
3. CPU 可以运行串口任务或空闲任务。
4. 500 Tick 到达后，LED 任务重新进入就绪态。
5. 因为 LED 任务优先级较高，它会在合适的调度点获得 CPU。

这与裸机 `delay_ms(500)` 最大的区别是：

```text
裸机 delay_ms：CPU 通常一直忙着等待。
vTaskDelay：只阻塞当前任务，其他任务继续运行。
```

---

## 9. UART_Test_Task：学习第二个并发任务

任务中的固定文本：

```c
static const uint8_t message[] = "FreeRTOS UART task running\r\n";
```

`\r\n` 表示回车和换行，串口终端会把下一条文本显示在新的一行。

发送代码：

```c
(void)UartTx_TryWrite(message, (uint16_t)(sizeof(message) - 1));
```

`sizeof(message)` 包含字符串末尾自动添加的 `'\0'`，串口不需要发送字符串结束
符，所以减去 1。

函数返回值表示文本是否成功写入发送缓冲区。当前测试任务不重试，发送缓冲区满
时直接丢弃本次文本，避免任务长时间等待串口。

发送后调用：

```c
vTaskDelay(pdMS_TO_TICKS(UART_REPORT_PERIOD_MS));
```

串口任务阻塞 1000 ms。在这段时间内 LED 任务仍会正常切换。

---

## 10. 任务有哪些状态

本阶段需要理解四种基础状态：

| 状态 | 含义 |
| --- | --- |
| 运行态 Running | 当前正在使用 CPU |
| 就绪态 Ready | 可以运行，但正在等待 CPU |
| 阻塞态 Blocked | 等待时间或事件，暂时不参与 CPU 竞争 |
| 挂起态 Suspended | 被明确挂起，本阶段没有使用 |

本工程的典型过程：

```text
LED_Task 运行
    ↓ 调用 vTaskDelay(500 ms)
LED_Task 阻塞
    ↓
UART_Test_Task 运行
    ↓ 调用 vTaskDelay(1000 ms)
UART_Test_Task 阻塞
    ↓
空闲任务运行
    ↓ 500 ms 到达
LED_Task 就绪并运行
```

当所有用户任务都处于阻塞态时，FreeRTOS 会运行空闲任务。CPU 不会因为没有用户
任务可运行而离开调度器。

---

## 11. FreeRTOSConfig.h：决定内核行为

文件：`User/FreeRTOSConfig.h`

这个文件不是 FreeRTOS 官方固定配置，而是每个工程根据芯片和需求编写的配置。

### 11.1 抢占式调度

```c
#define configUSE_PREEMPTION 1
```

启用抢占式调度。

当高优先级任务从阻塞态变为就绪态时，它可以抢占低优先级任务，尽快运行。

### 11.2 时间片

```c
#define configUSE_TIME_SLICING 1
```

如果多个同优先级任务同时处于就绪态，FreeRTOS 可以按 Tick 在它们之间轮换。

当前两个用户任务优先级不同，所以本阶段很难直接观察时间片效果。

### 11.3 CPU 时钟

```c
#define configCPU_CLOCK_HZ ((uint32_t)SystemCoreClock)
```

FreeRTOS 使用 `SystemCoreClock` 获得 CPU 时钟频率。当前 STM32 系统初始化将芯片
配置为 72 MHz。

内核根据 CPU 时钟和 Tick 频率计算 SysTick 的重装值。

### 11.4 系统节拍

```c
#define configTICK_RATE_HZ ((TickType_t)1000)
```

表示每秒产生 1000 次系统节拍：

```text
1 秒 ÷ 1000 = 1 ms
```

所以本工程的调度时间基准为 1 ms。

Tick 越快，时间分辨率越高，但 SysTick 中断也越频繁。对于第一次练习，1000 Hz
直观且常用。

### 11.5 最大优先级数量

```c
#define configMAX_PRIORITIES 5
```

可使用的任务优先级为：

```text
0、1、2、3、4
```

这不是 Cortex-M3 硬件中断优先级，而是 FreeRTOS 任务优先级。两者不能混淆。

### 11.6 FreeRTOS 堆

```c
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configTOTAL_HEAP_SIZE ((size_t)(8 * 1024))
```

启用动态内存分配，并为 FreeRTOS 提供 8 KB 堆。

这块内存主要用于：

- 动态创建任务的 TCB。
- 动态创建任务的栈。
- 后续创建队列、信号量和软件定时器等内核对象。

它不是 C 标准库 `malloc()` 使用的普通堆。本工程由 `heap_4.c` 管理 FreeRTOS
堆。

### 11.7 软件定时器和同步功能

```c
#define configUSE_TIMERS              0
#define configUSE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES 0
```

第一阶段暂时关闭这些功能，原因是：

- 减少需要同时理解的概念。
- 减少内核代码和 RAM 使用量。
- 先确认最基本的任务调度工作正常。

这些功能会在后续阶段逐步开启。

### 11.8 错误检测

```c
#define configUSE_MALLOC_FAILED_HOOK   1
#define configCHECK_FOR_STACK_OVERFLOW 2
```

启用：

- 动态内存分配失败钩子。
- 任务栈溢出检查。

第一次学习时建议保留这些检查，因为任务栈和堆配置错误非常常见。

---

## 12. heap_4.c：任务内存从哪里来

文件：

```text
Middlewares/FreeRTOS-Kernel/portable/MemMang/heap_4.c
```

FreeRTOS 提供多种示例内存管理器。本工程选择 `heap_4.c`。

它的特点：

- 支持分配内存。
- 支持释放内存。
- 会合并相邻的空闲内存块，减轻内存碎片。
- 适合大多数普通 STM32 学习项目。

调用 `xTaskCreate()` 时，内核最终通过 FreeRTOS 的内存分配接口从这 8 KB 堆中
取得 TCB 和任务栈空间。

当前大致需要：

```text
LED_Task 栈          512 字节
UART_Test_Task 栈    512 字节
空闲任务栈           512 字节
三个任务的 TCB       若干字节
内存块管理开销       若干字节
```

因此 8 KB 对本阶段足够，同时仍给后续队列和信号量练习留有空间。

---

## 13. 三个关键异常：SysTick、PendSV、SVC

FreeRTOS 在 Cortex-M3 上实现调度，需要使用三个系统异常。

### 13.1 SysTick_Handler

SysTick 是 Cortex-M3 内核自带的定时器。

本工程每 1 ms 进入一次 FreeRTOS 的 `SysTick_Handler`。它主要负责：

- 增加 FreeRTOS 系统节拍计数。
- 检查延时任务是否到期。
- 如果需要切换任务，请求 PendSV。

例如 LED 任务延时 500 Tick，内核会在 Tick 到达后把它从阻塞态移回就绪态。

### 13.2 PendSV_Handler

PendSV 负责真正的任务上下文切换。

上下文切换大致包括：

1. 保存当前任务的 CPU 寄存器。
2. 把当前栈顶位置保存到任务控制块。
3. 选择下一个最高优先级就绪任务。
4. 取出新任务保存的栈顶位置。
5. 恢复新任务的 CPU 寄存器。
6. 返回后继续执行新任务。

PendSV 被设置为最低中断优先级，使任务切换尽量不会打断更重要的硬件中断。

### 13.3 SVC_Handler

SVC 是 Supervisor Call 的缩写。

FreeRTOS 启动第一个任务时使用 SVC，让处理器从内核启动流程切换到任务上下文。

第一阶段不需要阅读 `port.c` 中的汇编细节，只需要知道：

```text
SVC：启动第一个任务。
SysTick：提供时间节拍。
PendSV：执行任务切换。
```

### 13.4 为什么 stm32f10x_it.c 不能再定义它们

旧工程的 `stm32f10x_it.c` 中曾经有空的：

```c
void SVC_Handler(void)
void PendSV_Handler(void)
void SysTick_Handler(void)
```

如果应用文件和 FreeRTOS `port.c` 同时定义这些函数，链接时会出现重复定义；如果
错误地使用了空处理函数，调度器也无法工作。

所以当前工程让 `port.c` 提供这三个异常处理函数。

---

## 14. FreeRTOS 内核文件分别做什么

Keil 工程中的 `FreeRTOS` 分组包含：

| 文件 | 作用 | 当前是否有核心用途 |
| --- | --- | --- |
| `tasks.c` | 任务创建、调度、延时、状态管理 | 是 |
| `list.c` | 内核链表，管理就绪和阻塞任务 | 是 |
| `queue.c` | 队列、信号量、互斥锁的基础实现 | 本阶段未调用 |
| `port.c` | Cortex-M3 上下文切换和 SysTick | 是 |
| `heap_4.c` | FreeRTOS 动态内存管理 | 是 |

`queue.c` 已加入工程，是为了后续继续学习队列和信号量。当前没有调用队列 API，
链接器会移除未使用的代码，因此不会把整个 `queue.c` 都放进最终 Flash。

头文件目录中还包含定时器、事件组、流缓冲区等接口。它们存在于工程中不表示当前
已经启用或运行。

---

## 15. LED 驱动和 RTOS 的关系

文件：`User/led.c`

LED 驱动做的是硬件层工作：

```c
RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
```

开启 GPIOA 时钟。

```c
GPIO_InitStructure.GPIO_Pin =
    GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
```

把 PA5、PA6、PA7 配置为推挽输出。

```c
GPIO_SetBits(...);
GPIO_ResetBits(...);
```

控制实际引脚电平。

FreeRTOS 不关心 PA5 是红灯还是其他设备。LED 任务只是在合适的时间调用现有驱动
函数。

推荐一直保持这种分层：

```text
任务层：决定什么时候做、按什么顺序做。
驱动层：决定寄存器和硬件具体怎样操作。
```

---

## 16. 串口发送为什么使用中断缓冲区

直接轮询发送常见写法：

```c
USART_SendData(USART1, byte);
while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
{
}
```

这种写法会等待硬件发送，任务在等待期间不能做其他事情。

当前工程使用 `uart_tx.c` 中的环形缓冲区：

```text
UART_Test_Task
    ↓
把整行文本写入 RAM 环形缓冲区
    ↓
开启 USART1 TXE 中断
    ↓
任务很快返回并进入 vTaskDelay()
    ↓
USART1 中断每次取出一个字节发送
```

这样任务不需要逐字节等待串口硬件。

### 16.1 环形缓冲区的基本概念

缓冲区使用两个索引：

- `head`：下一次写入的位置。
- `tail`：下一次读取发送的位置。

写入数据后移动 `head`，中断发送数据后移动 `tail`。到达数组末尾时回到开头，
所以看起来像一个环。

### 16.2 当前串口中断没有调用 FreeRTOS API

`USART1_IRQHandler()` 调用的是普通缓冲区处理函数，没有调用
`xQueueSendFromISR()` 等 FreeRTOS 中断 API。

因此当前 USART1 中断优先级不受“必须低于
`configMAX_SYSCALL_INTERRUPT_PRIORITY` 才能调用 FreeRTOS API”这一规则的
直接限制。

以后如果要在串口中断里使用队列或任务通知，必须重新检查中断优先级。

---

## 17. 一个 Tick 内可能发生什么

假设系统刚启动：

```text
时间 0 ms：
    LED_Task 优先级最高，先运行
    点亮红灯
    延时 500 ms，进入阻塞态

紧接着：
    UART_Test_Task 运行
    写入串口文本
    延时 1000 ms，进入阻塞态

0 到 500 ms：
    两个用户任务都阻塞
    空闲任务运行
    SysTick 每 1 ms 更新节拍

时间约 500 ms：
    LED_Task 延时到期
    LED_Task 运行并点亮黄灯
    再次阻塞 500 ms

时间约 1000 ms：
    LED_Task 和 UART_Test_Task 都到期
    LED_Task 优先级 2，先运行并切换绿灯
    LED_Task 再次阻塞
    UART_Test_Task 随后运行并发送文本
```

这段过程说明优先级只决定“多个任务都可以运行时谁先运行”，并不决定任务执行
周期。周期由任务中的延时或等待事件决定。

---

## 18. 如何判断实验成功

### 18.1 LED 现象

接线：

| LED | 引脚 |
| --- | --- |
| 红灯 | PA5 |
| 黄灯 | PA6 |
| 绿灯 | PA7 |

当前驱动为高电平点亮。

预期：

```text
红灯 500 ms
黄灯 500 ms
绿灯 500 ms
重复
```

### 18.2 串口现象

接线：

| USB-TTL | STM32 |
| --- | --- |
| RXD | PA9 / USART1_TX |
| TXD | PA10 / USART1_RX |
| GND | GND |

串口参数：

```text
115200
8 数据位
1 停止位
无校验
无流控
```

预期每秒看到：

```text
FreeRTOS UART task running
```

### 18.3 成功代表什么

两个现象同时正确，至少说明：

- 芯片时钟正常。
- LED 和 USART1 初始化正常。
- 两个任务创建成功。
- FreeRTOS 堆可以正常分配。
- SysTick 正常产生节拍。
- `vTaskDelay()` 正常解除阻塞。
- PendSV 可以进行上下文切换。
- 串口中断发送正常。

---

## 19. 建议你亲手完成的实验

不要只阅读代码。每次只改一个参数，编译、下载、观察，再恢复或记录结果。

### 实验 1：修改 LED 周期

把：

```c
#define LED_STEP_PERIOD_MS 500
```

改为：

```c
#define LED_STEP_PERIOD_MS 1000
```

预测：每种颜色持续 1 秒，串口仍每秒输出。

学习目标：理解任务延时只影响当前任务。

### 实验 2：修改串口周期

把串口周期改为 2000 ms。

预测：LED 速度不变，串口每 2 秒输出一次。

学习目标：验证两个任务拥有独立的阻塞时间。

### 实验 3：交换优先级

把 LED 优先级改为 1，串口优先级改为 2。

预测：正常情况下肉眼现象几乎不变，因为两个任务绝大多数时间都在阻塞。

学习目标：理解“高优先级”不等于“执行频率更高”。

### 实验 4：暂时移除 LED 任务的 vTaskDelay()

这个实验只能短时间进行：

```c
for (;;)
{
    Traffic_RedOn();
}
```

预测：LED 高优先级任务永远处于运行态，低优先级串口任务得不到 CPU，串口停止
输出。

学习目标：直观看到高优先级任务不阻塞会造成低优先级任务饥饿。

实验完成后必须恢复 `vTaskDelay()`。

### 实验 5：观察任务创建失败

把 FreeRTOS 堆临时改得非常小，例如 1 KB。

预测：任务或空闲任务可能创建失败，程序停在故障死循环或内存失败钩子。

学习目标：理解任务栈和 TCB 都需要 RAM。

实验完成后恢复为 8 KB。

---

## 20. 常见误区

### 误区 1：使用 RTOS 就是真正同时运行

STM32F103C8 是单核 MCU。同一时刻只有一个任务执行。RTOS 通过快速切换实现并发。

### 误区 2：任务优先级越高，运行次数越多

优先级决定就绪任务之间谁先获得 CPU。任务周期主要取决于延时、事件和任务逻辑。

### 误区 3：vTaskDelay() 会停止整个系统

它只阻塞调用它的任务，其他就绪任务仍能运行。

### 误区 4：任务栈深度 128 就是 128 字节

本工程中是 128 个 32 位元素，大约 512 字节。

### 误区 5：FreeRTOS 会自动解决所有并发问题

不会。多个任务访问同一个全局变量、外设或缓冲区时，仍需要考虑竞争条件，后续
要学习临界区、互斥锁、队列和任务通知。

### 误区 6：工程目录里的所有 FreeRTOS 文件都在运行

只有被编译并且被链接器保留的代码才进入最终固件。配置为关闭的功能不会自动
运行，未被调用的函数通常会被链接器移除。

### 误区 7：可以在任何中断里调用普通 FreeRTOS API

不能。中断中要使用带 `FromISR` 后缀的 API，还必须满足 Cortex-M 中断优先级
规则。本阶段串口中断没有调用 FreeRTOS API。

---

## 21. 当前工程暂时没有学习的内容

为了保持第一阶段足够小，当前没有使用：

- 队列。
- 二值信号量。
- 计数信号量。
- 互斥锁。
- 任务通知的实际通信。
- 软件定时器。
- 事件组。
- 动态创建后删除任务。
- Tickless 低功耗。
- 中断向任务发送事件。

这不是缺失，而是刻意控制学习范围。

---

## 22. 推荐的后续学习顺序

建议后续按下面顺序继续，每个阶段只增加一个核心概念。

### 第二阶段：任务状态和精确定时

- 使用 `vTaskDelayUntil()`。
- 对比 `vTaskDelay()` 和 `vTaskDelayUntil()`。
- 观察周期任务的累计误差。

### 第三阶段：队列

- 串口接收任务产生数据。
- LED 控制任务通过队列接收命令。
- 学习任务之间不要直接共享复杂数据。

### 第四阶段：二值信号量或任务通知

- GPIO/串口中断唤醒任务。
- 学习中断只做快速处理，把耗时工作交给任务。

### 第五阶段：互斥锁

- 两个任务共享串口。
- 学习优先级继承和资源互斥。

### 第六阶段：软件定时器

- 使用软件定时器完成低频状态指示。
- 对比软件定时器回调和普通任务。

### 第七阶段：栈和运行状态分析

- 查看任务剩余栈空间。
- 查看 FreeRTOS 剩余堆空间。
- 学习根据测量结果调整任务栈。

---

## 23. 本阶段应记住的 API

现在只需要牢牢记住下面三个：

```c
xTaskCreate(...)
```

创建任务并为它分配 TCB 和栈。

```c
vTaskStartScheduler()
```

启动 FreeRTOS 调度器。成功后通常不返回。

```c
vTaskDelay(...)
```

让当前任务阻塞指定 Tick，其他任务可以继续运行。

时间转换使用：

```c
pdMS_TO_TICKS(milliseconds)
```

不要在应用代码中直接假设“1 Tick 永远等于 1 ms”。虽然当前配置确实如此，但用
转换宏能让代码适应以后不同的 Tick 频率。

---

## 24. 本阶段总结

当前工程用最少的功能建立了一个完整 RTOS 运行链路：

```text
硬件初始化
    ↓
xTaskCreate 创建任务
    ↓
heap_4 分配任务内存
    ↓
vTaskStartScheduler 启动调度
    ↓
SysTick 提供 1 ms 时间基准
    ↓
PendSV 切换任务上下文
    ↓
LED_Task 和 UART_Test_Task 交替运行
    ↓
vTaskDelay 让任务主动阻塞
```

这一阶段最重要的认识不是记住所有配置，而是理解：

> RTOS 把一个大循环拆成多个长期存在的任务；任务没有工作时应主动阻塞，调度器
> 再把 CPU 交给其他就绪任务。

当你能够根据 LED 和串口现象解释两个任务的状态变化，并能说明
`xTaskCreate()`、`vTaskStartScheduler()`、`vTaskDelay()` 的作用，就已经完成了
FreeRTOS 入门的第一步。
