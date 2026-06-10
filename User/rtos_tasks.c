#include "rtos_tasks.h"
#include "task.h"
#include "led.h"
#include "uart_tx.h"

#define LED_TASK_STACK_DEPTH        128     // LED 任务栈，单位为字
#define UART_TASK_STACK_DEPTH       128     // 串口任务栈，单位为字
#define LED_TASK_PRIORITY           2       // LED 任务优先级
#define UART_TASK_PRIORITY          1       // 串口任务优先级
#define LED_STEP_PERIOD_MS          500     // LED 颜色切换周期
#define UART_REPORT_PERIOD_MS       1000    // 串口测试文本周期

static void LED_Task(void *argument);
static void UART_Test_Task(void *argument);

/*
 * 创建第一阶段使用的两个 FreeRTOS 练习任务。
 *
 * LED 任务优先级高于串口测试任务，但两个任务都会使用
 * vTaskDelay() 主动阻塞，因此不会长期占用处理器。
 *
 * 参数：
 * 无。
 *
 * 返回值：
 * pdPASS：两个任务均创建成功。
 * pdFAIL：至少一个任务创建失败。
 *
 * 副作用：
 * 从 FreeRTOS heap_4 管理的堆中分配任务控制块和任务栈。
 */
BaseType_t RTOS_Tasks_Create(void)
{
    BaseType_t result;

    result = xTaskCreate(LED_Task,
                         "LED_Task",
                         LED_TASK_STACK_DEPTH,
                         0,
                         LED_TASK_PRIORITY,
                         0);
    if (result != pdPASS)
    {
        return pdFAIL;
    }

    result = xTaskCreate(UART_Test_Task,
                         "UART_Test_Task",
                         UART_TASK_STACK_DEPTH,
                         0,
                         UART_TASK_PRIORITY,
                         0);
    if (result != pdPASS)
    {
        return pdFAIL;
    }

    return pdPASS;
}

/*
 * 周期切换红、黄、绿三个交通灯 LED。
 *
 * 每次只点亮一个 LED，并阻塞 500 ms 后切换到下一个颜色，
 * 用于直观看到任务延时和调度器持续运行。
 *
 * 参数：
 * argument：FreeRTOS 任务参数，本任务未使用。
 *
 * 返回值：
 * 无；任务函数不会返回。
 *
 * 副作用：
 * 周期修改 GPIOA 的 PA5、PA6 和 PA7 输出电平。
 */
static void LED_Task(void *argument)
{
    uint8_t step;

    (void)argument;
    step = 0;

    for (;;)
    {
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

        step++;
        if (step >= 3)
        {
            step = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(LED_STEP_PERIOD_MS));
    }
}

/*
 * 周期发送 FreeRTOS 串口运行信息。
 *
 * 通过现有 USART1 中断发送环形缓冲区写入固定文本。写入失败时
 * 本周期直接丢弃文本，避免测试任务因串口异常阻塞调度器。
 *
 * 参数：
 * argument：FreeRTOS 任务参数，本任务未使用。
 *
 * 返回值：
 * 无；任务函数不会返回。
 *
 * 副作用：
 * 每 1000 ms 向 USART1 的 PA9 发送一行 ASCII 文本。
 */
static void UART_Test_Task(void *argument)
{
    static const uint8_t message[] = "FreeRTOS UART task running\r\n";

    (void)argument;

    for (;;)
    {
        (void)UartTx_TryWrite(message, (uint16_t)(sizeof(message) - 1));
        vTaskDelay(pdMS_TO_TICKS(UART_REPORT_PERIOD_MS));
    }
}

/*
 * 处理 FreeRTOS 动态内存分配失败。
 *
 * 当 heap_4 无法满足任务或内核对象分配时关闭中断并停机，
 * 便于调试器稳定定位内存不足问题。
 *
 * 参数：
 * 无。
 *
 * 返回值：
 * 无；失败处理不会返回。
 *
 * 副作用：
 * 全局关闭中断并停止正常任务调度。
 */
void vApplicationMallocFailedHook(void)
{
    __disable_irq();
    for (;;)
    {
    }
}

/*
 * 处理 FreeRTOS 检测到的任务栈溢出。
 *
 * 保留任务句柄和任务名称参数供调试器查看，随后关闭中断并停机，
 * 防止已经损坏的任务栈继续执行。
 *
 * 参数：
 * task_handle：发生栈溢出的任务句柄。
 * task_name：发生栈溢出的任务名称。
 *
 * 返回值：
 * 无；失败处理不会返回。
 *
 * 副作用：
 * 全局关闭中断并停止正常任务调度。
 */
void vApplicationStackOverflowHook(TaskHandle_t task_handle, char *task_name)
{
    (void)task_handle;
    (void)task_name;

    __disable_irq();
    for (;;)
    {
    }
}
