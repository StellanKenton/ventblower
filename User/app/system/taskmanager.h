/************************************************************************************
* @file     : taskmanager.h
* @brief    : Project task manager.
* @details  : Declares the entry used to create the project task manager.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#ifndef USER_APP_TASK_MANAGER_H
#define USER_APP_TASK_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Stack sizes are in words (4 bytes). */
#define DEFAULT_TASK_STACK_SIZE          1024U
#define DEFAULT_TASK_PRIORITY            2U
#define DEFAULT_TASK_INTERVAL_MS         10U

#define COMM_TASK_STACK_SIZE             512U
#define COMM_TASK_PRIORITY               20U
#define COMM_TASK_INTERVAL_MS            5U

#define SYS_TASK_STACK_SIZE              256U
#define SYS_TASK_PRIORITY                21U
#define SYS_TASK_INTERVAL_MS             20U

/* Speed loop and motor state machine; the FOC current loop stays in the ADC ISR. */
#define MOTOR_TASK_STACK_SIZE            512U
#define MOTOR_TASK_PRIORITY              22U
#define MOTOR_TASK_INTERVAL_MS           1U

/* Highest priority so it can shut the power stage down even if MotorTask stalls. */
#define PROTECT_TASK_STACK_SIZE          256U
#define PROTECT_TASK_PRIORITY            23U
#define PROTECT_TASK_INTERVAL_MS         2U

bool WorkerTasksRegister(void);

/** Log stack size and minimum free stack per worker task, plus minimum free heap. */
void taskManagerShowStats(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_APP_TASK_MANAGER_H */
/**************************End of file********************************/
