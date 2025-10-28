/**
 * @file app_http_server.h
 * @brief Header file for HTTP server functionality in chat bot
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#ifndef __APP_HTTP_SERVER_H__
#define __APP_HTTP_SERVER_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
********************function declaration********************
***********************************************************/

/**
 * @brief Initialize HTTP server for chat bot
 * 
 * Starts HTTP server on port 8080 to accept REST API calls.
 * Supported endpoints:
 * - POST /api/message - Send text message to AI bot
 * - GET /api/status - Get server status
 * - GET / - Get API documentation
 *
 * @return OPERATE_RET - OPRT_OK on success, error code on failure
 */
OPERATE_RET app_http_server_init(void);

/**
 * @brief Stop HTTP server
 *
 * @return OPERATE_RET - OPRT_OK on success, error code on failure
 */
OPERATE_RET app_http_server_stop(void);

/**
 * @brief Check if HTTP server is running
 *
 * @return bool - true if running, false otherwise
 */
bool app_http_server_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_HTTP_SERVER_H__ */