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

/* Stack sizes are in words (4 bytes). Static worst case: default ~1.0 KB (console + LOG_R),
 * sensor ~0.7 KB (LOG), sys ~60 B; plus ~240 B FPU exception frame and context save. */
#define DEFAULT_TASK_STACK_SIZE          512U
#define DEFAULT_TASK_PRIORITY            2U
#define DEFAULT_TASK_INTERVAL_MS         10U

#define SENSOR_TASK_STACK_SIZE           384U
#define SENSOR_TASK_PRIORITY             20U
#define SENSOR_TASK_INTERVAL_MS          3U

/* Raise to >= 384 words once SysTask starts calling LOG_x. */
#define SYS_TASK_STACK_SIZE              128U
#define SYS_TASK_PRIORITY                21U
#define SYS_TASK_INTERVAL_MS             20U

bool WorkerTasksRegister(void);

/** Log stack size and minimum free stack per worker task, plus minimum free heap. */
void taskManagerShowStats(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_APP_TASK_MANAGER_H */
/**************************End of file********************************/
