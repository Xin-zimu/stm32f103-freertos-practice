# FreeRTOS 第一阶段学习文档

日期：2026-06-10

## 修改目标

为第一次学习 RTOS 的用户编写一份与当前工程代码对应的第一阶段教学文档，
解释所使用的代码、选择原因、运行流程、预期现象和动手实验。

## 修改前行为

README 只提供工程配置、接线和运行现象的概要说明，没有系统讲解裸机与 RTOS
的区别、任务状态、任务创建参数、任务栈、FreeRTOS 堆和 Cortex-M3 调度异常。

## 修改后行为

新增 `docs/FREERTOS_STAGE1_LEARNING_GUIDE.md`，从裸机轮询开始讲解当前最小
FreeRTOS 工程，逐文件说明 main.c、rtos_tasks.c、FreeRTOSConfig.h、LED 驱动、
串口中断缓冲区和内核文件的作用，并提供实验步骤、常见误区和后续学习路线。
README 增加学习文档入口。

## 逻辑变化范围

代码逻辑与原提交完全一致。本次只新增和更新 Markdown 文档。

## 涉及文件

- `docs/FREERTOS_STAGE1_LEARNING_GUIDE.md`：第一阶段完整教学文档。
- `README.md`：增加学习文档入口。
- `docs/change-logs/2026-06-10-freertos-stage1-learning-guide.md`：本次变更记录。

## 接口与兼容性

不修改 C/H、Keil 工程、硬件接口、任务参数或串口协议，不影响当前固件行为。

## 编码与注释规范

- C/H：GB2312 兼容代码页 936，无 BOM。
- 文档及脚本：UTF-8，无 BOM。
- 函数前详细注释，函数内仅保留关键注释。
- 宏、枚举和结构字段使用对齐的行尾 // 注释。

## 验证结果

- 文档内容已与当前 main.c、rtos_tasks.c、FreeRTOSConfig.h、led.c、
  usart.c、uart_tx.c 和 Keil 编译配置逐项核对。
- Markdown 文件 UTF-8 无 BOM 检查通过。
- `git diff --check` 通过。
- Keil 隔离编译：0 Error(s)，0 Warning(s)。

## Git

- 分支：main
- 起始提交：94c650858a412c1cb19445da1ebcab37d670a1e9
- Commit：this commit
- 提交说明：Add FreeRTOS stage one learning guide
