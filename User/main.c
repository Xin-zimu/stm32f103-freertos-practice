#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led.h"
#include "usart.h"
#include "rtos_tasks.h"

/*
 * 初始化最小 FreeRTOS 练习工程并启动任务调度。
 *
 * 使用四位抢占优先级分组，使 Cortex-M3 中断优先级与
 * FreeRTOSConfig.h 中的中断配置一致。完成 LED 和 USART1
 * 初始化后创建 LED_Task 与 UART_Test_Task，随后启动调度器。
 *
 * 参数：
 * 无。
 *
 * 返回值：
 * 正常情况下调度器启动后不会返回。
 *
 * 副作用：
 * 配置 NVIC、GPIOA、USART1、SysTick，并从 FreeRTOS 堆中
 * 分配任务控制块和任务栈。
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    LED_Init();
    uart_init(115200);

    if (RTOS_Tasks_Create() != pdPASS)
    {
        Traffic_AllOff();
        for (;;)
        {
        }
    }

    vTaskStartScheduler();

    Traffic_AllOff();
    for (;;)
    {
    }
}
