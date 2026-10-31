/************************************************************************************
* @file     : taskmanager.h
* @brief    : Project task manager.
* @details  : Declares the entry used to create the project task manager.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_APP_TASK_MANAGER_H
#define USER_APP_TASK_MANAGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_TASK_STACK_SIZE          1024U
#define DEFAULT_TASK_PRIORITY            2U
#define DEFAULT_TASK_INTERVAL_MS         10U

#define SENSOR_TASK_STACK_SIZE           512U
#define SENSOR_TASK_PRIORITY             20U
#define SENSOR_TASK_INTERVAL_MS          3U

#define SYS_TASK_STACK_SIZE              256U
#define SYS_TASK_PRIORITY                21U
#define SYS_TASK_INTERVAL_MS             20U

bool WorkerTasksRegister(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_APP_TASK_MANAGER_H */
/**************************End of file********************************/
