#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f10x.h"

#define configUSE_PREEMPTION                    1                       // 使用抢占式调度
#define configUSE_TIME_SLICING                  1                       // 同优先级任务使用时间片
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1                       // 使用 Cortex-M3 优化选择
#define configUSE_TICKLESS_IDLE                 0                       // 第一阶段关闭低功耗节拍

#define configCPU_CLOCK_HZ                      ((uint32_t)SystemCoreClock) // 运行时系统时钟
#define configTICK_RATE_HZ                      ((TickType_t)1000)          // 1 ms 系统节拍
#define configMAX_PRIORITIES                    5                          // 可用任务优先级数量
#define configMINIMAL_STACK_SIZE                ((uint16_t)128)            // 空闲任务栈，单位为字
#define configMAX_TASK_NAME_LEN                 16                         // 任务名称最大长度
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS    // 使用 32 位节拍计数
#define configIDLE_SHOULD_YIELD                 1                          // 空闲任务允许主动让出

#define configSUPPORT_STATIC_ALLOCATION         0                       // 第一阶段不使用静态创建
#define configSUPPORT_DYNAMIC_ALLOCATION        1                       // 使用动态任务创建
#define configTOTAL_HEAP_SIZE                   ((size_t)(8 * 1024))    // FreeRTOS 堆为 8 KB
#define configAPPLICATION_ALLOCATED_HEAP        0                       // 内核定义堆数组

#define configUSE_TIMERS                        0                       // 第一阶段关闭软件定时器
#define configUSE_MUTEXES                       0                       // 第一阶段关闭互斥锁
#define configUSE_RECURSIVE_MUTEXES             0                       // 关闭递归互斥锁
#define configUSE_COUNTING_SEMAPHORES           0                       // 关闭计数信号量
#define configUSE_QUEUE_SETS                    0                       // 关闭队列集
#define configQUEUE_REGISTRY_SIZE               0                       // 不启用队列注册表
#define configUSE_TASK_NOTIFICATIONS            1                       // 保留轻量任务通知

#define configUSE_IDLE_HOOK                     0                       // 不使用空闲钩子
#define configUSE_TICK_HOOK                     0                       // 不使用节拍钩子
#define configUSE_MALLOC_FAILED_HOOK            1                       // 捕获堆分配失败
#define configCHECK_FOR_STACK_OVERFLOW          2                       // 检查任务栈边界

#define configUSE_TRACE_FACILITY                0                       // 关闭跟踪扩展字段
#define configUSE_STATS_FORMATTING_FUNCTIONS    0                       // 关闭统计格式化函数
#define configGENERATE_RUN_TIME_STATS           0                       // 关闭运行时间统计
#define configUSE_CO_ROUTINES                   0                       // 不使用协程
#define configMAX_CO_ROUTINE_PRIORITIES         1                       // 协程关闭时保留合法值

#define configKERNEL_INTERRUPT_PRIORITY         (15 << 4)               // SysTick 和 PendSV 最低优先级
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (5 << 4)                // RTOS API 中断优先级边界

#define INCLUDE_vTaskDelay                      1                       // 启用相对延时
#define INCLUDE_vTaskDelayUntil                 1                       // 保留周期延时接口
#define INCLUDE_vTaskDelete                     0                       // 第一阶段不删除任务
#define INCLUDE_vTaskSuspend                    0                       // 第一阶段不挂起任务
#define INCLUDE_xTaskGetSchedulerState          0                       // 不查询调度器状态

#define vPortSVCHandler                         SVC_Handler             // 映射到启动文件异常名
#define xPortPendSVHandler                      PendSV_Handler          // 映射到启动文件异常名
#define xPortSysTickHandler                     SysTick_Handler         // 映射到启动文件异常名

#define configASSERT(condition)                 \
    do                                          \
    {                                           \
        if ((condition) == 0)                   \
        {                                       \
            __disable_irq();                    \
            for (;;)                            \
            {                                   \
            }                                   \
        }                                       \
    } while (0)

#endif
