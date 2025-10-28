/**
 * @file app_http_server.c
 * @brief HTTP server for chat bot to receive text messages via REST API
 *
 * This file implements a simple HTTP server that accepts JSON messages in the format:
 * POST /api/message
 * Content-Type: application/json
 * {"msg": "your text message"}
 *
 * The server will parse the JSON and send the message to the AI chat bot.
 *
 * @copyright Copyright (c) 2021-2025 Tuya Inc. All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>
#include "tuya_cloud_types.h"
#include "tal_api.h"
#include "tal_network.h"
#include "tkl_output.h"
#include "netmgr.h"
#include "app_http_server.h"
#include "ai_audio.h"
#include "cJSON.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define HTTP_SERVER_PORT          8080
#define HTTP_RECV_BUFFER_SIZE     2048
#define HTTP_RESPONSE_BUFFER_SIZE 1024
#define HTTP_MAX_MSG_LEN          512

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    char method[16];
    char path[256];
    char version[16];
    char headers[1024];
    char body[1024];
} http_request_t;

/***********************************************************
***********************variable define**********************
***********************************************************/
static THREAD_HANDLE sg_http_server = NULL;
static bool sg_http_server_running = false;

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Parse HTTP request from raw data
 */
static int __parse_http_request(const char *raw_request, http_request_t *request)
{
    if (!raw_request || !request) {
        return -1;
    }

    memset(request, 0, sizeof(http_request_t));

    char *line_end = strstr(raw_request, "\r\n");
    if (!line_end) {
        return -1;
    }

    char request_line[512];
    int line_len = line_end - raw_request;
    if (line_len >= sizeof(request_line)) {
        return -1;
    }

    strncpy(request_line, raw_request, line_len);
    request_line[line_len] = '\0';

    if (sscanf(request_line, "%15s %255s %15s", request->method, request->path, request->version) != 3) {
        return -1;
    }

    char *headers_start = line_end + 2;
    char *body_start = strstr(headers_start, "\r\n\r\n");

    if (body_start) {
        int headers_len = body_start - headers_start;
        if (headers_len < sizeof(request->headers)) {
            strncpy(request->headers, headers_start, headers_len);
            request->headers[headers_len] = '\0';
        }

        body_start += 4;
        strncpy(request->body, body_start, sizeof(request->body) - 1);
        request->body[sizeof(request->body) - 1] = '\0';
    } else {
        strncpy(request->headers, headers_start, sizeof(request->headers) - 1);
        request->headers[sizeof(request->headers) - 1] = '\0';
    }

    return 0;
}

/**
 * @brief Handle POST /api/message request
 */
static int __handle_message_api(const char *body, char *response_buffer, size_t buffer_size)
{
    cJSON *json = cJSON_Parse(body);
    if (!json) {
        PR_ERR("Failed to parse JSON body");
        return snprintf(response_buffer, buffer_size,
                        "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: 42\r\n"
                        "Connection: close\r\n\r\n"
                        "{\"error\":\"Invalid JSON format\",\"code\":400}");
    }

    cJSON *msg_item = cJSON_GetObjectItem(json, "msg");
    if (!msg_item || !cJSON_IsString(msg_item)) {
        cJSON_Delete(json);
        return snprintf(response_buffer, buffer_size,
                        "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: 48\r\n"
                        "Connection: close\r\n\r\n"
                        "{\"error\":\"Missing 'msg' field in JSON\",\"code\":400}");
    }

    const char *message = cJSON_GetStringValue(msg_item);
    if (!message || strlen(message) == 0) {
        cJSON_Delete(json);
        return snprintf(response_buffer, buffer_size,
                        "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: 43\r\n"
                        "Connection: close\r\n\r\n"
                        "{\"error\":\"Empty message content\",\"code\":400}");
    }

    if (strlen(message) > HTTP_MAX_MSG_LEN) {
        cJSON_Delete(json);
        return snprintf(response_buffer, buffer_size,
                        "HTTP/1.1 400 Bad Request\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: 45\r\n"
                        "Connection: close\r\n\r\n"
                        "{\"error\":\"Message too long (>512)\",\"code\":400}");
    }

    PR_DEBUG("Received message via HTTP API: %s", message);

    // Send message to AI chat bot
    OPERATE_RET rt = ai_text_agent_upload((uint8_t *)message, strlen(message));
    cJSON_Delete(json);

    if (rt == OPRT_OK) {
        const char *success_body = "{\"status\":\"success\",\"message\":\"Message sent\"}";
        return snprintf(response_buffer, buffer_size,
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n\r\n%s",
                        strlen(success_body), success_body);
    } else if (rt == OPRT_RESOURCE_NOT_READY) {
        char error_body[256];
        snprintf(error_body, sizeof(error_body),
                 "{\"error\":\"Device is busy, please try again later\",\"code\":503,\"rt\":%d}", rt);
        return snprintf(response_buffer, buffer_size,
                        "HTTP/1.1 503 Service Unavailable\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n\r\n%s",
                        strlen(error_body), error_body);
    } else if (rt == OPRT_COM_ERROR) {
        char error_body[256];
        snprintf(error_body, sizeof(error_body),
                 "{\"error\":\"Device is not ready or not opened\",\"code\":503,\"rt\":%d}", rt);
        return snprintf(response_buffer, buffer_size,
                        "HTTP/1.1 503 Service Unavailable\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n\r\n%s",
                        strlen(error_body), error_body);
    } else {
        char error_body[256];
        snprintf(error_body, sizeof(error_body), "{\"error\":\"Failed to send message to AI\",\"code\":500,\"rt\":%d}",
                 rt);
        return snprintf(response_buffer, buffer_size,
                        "HTTP/1.1 500 Internal Server Error\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n\r\n%s",
                        strlen(error_body), error_body);
    }
}

/**
 * @brief Generate HTTP response based on request
 */
static int __generate_http_response(const http_request_t *request, char *response_buffer, size_t buffer_size)
{
    if (!request || !response_buffer) {
        return -1;
    }

    // Handle POST /api/message
    if (strcmp(request->method, "POST") == 0 && strcmp(request->path, "/api/message") == 0) {
        return __handle_message_api(request->body, response_buffer, buffer_size);
    }

    // Handle GET /api/status
    if (strcmp(request->method, "GET") == 0 && strcmp(request->path, "/api/status") == 0) {
        char status_body[256];
        snprintf(status_body, sizeof(status_body),
                 "{\"status\":\"running\",\"service\":\"ChatBot HTTP API\",\"port\":%d}", HTTP_SERVER_PORT);
        return snprintf(response_buffer, buffer_size,
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n\r\n%s",
                        strlen(status_body), status_body);
    }

    // Handle GET /
    if (strcmp(request->method, "GET") == 0 && strcmp(request->path, "/") == 0) {
        const char *help_msg = "{"
                               "\"service\":\"ChatBot HTTP API\","
                               "\"endpoints\":{"
                               "\"POST /api/message\":\"Send text message to AI bot\","
                               "\"GET /api/status\":\"Get server status\""
                               "},"
                               "\"example\":\"POST /api/message with body {\\\"msg\\\":\\\"Hello AI\\\"}\""
                               "}";
        return snprintf(response_buffer, buffer_size,
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n\r\n%s",
                        strlen(help_msg), help_msg);
    }

    // 404 Not Found
    char error_body[256];
    snprintf(error_body, sizeof(error_body), "{\"error\":\"Not Found\",\"path\":\"%s\"}", request->path);
    return snprintf(response_buffer, buffer_size,
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: %zu\r\n"
                    "Connection: close\r\n\r\n%s",
                    strlen(error_body), error_body);
}

/**
 * @brief HTTP server task
 */
static void __http_server_task(void *args)
{
    char recv_buf[HTTP_RECV_BUFFER_SIZE] = {0};
    char response_buf[HTTP_RESPONSE_BUFFER_SIZE] = {0};
    int listen_fd, client_fd;
    TUYA_ERRNO net_errno = 0;
    http_request_t request;

    PR_NOTICE("--- Starting ChatBot HTTP server on port %d", HTTP_SERVER_PORT);

    listen_fd = tal_net_socket_create(PROTOCOL_TCP);
    if (listen_fd < 0) {
        PR_ERR("Failed to create socket");
        goto exit;
    }

    if (tal_net_bind(listen_fd, TY_IPADDR_ANY, HTTP_SERVER_PORT) != OPRT_OK) {
        PR_ERR("Failed to bind socket to port %d", HTTP_SERVER_PORT);
        tal_net_close(listen_fd);
        goto exit;
    }

    if (tal_net_listen(listen_fd, 5) != OPRT_OK) {
        PR_ERR("Failed to listen on socket");
        tal_net_close(listen_fd);
        goto exit;
    }

    PR_NOTICE("ChatBot HTTP server listening on port %d", HTTP_SERVER_PORT);
    sg_http_server_running = true;

    while (sg_http_server_running) {
        TUYA_IP_ADDR_T client_ip = 0;
        uint16_t client_port = 0;

        client_fd = tal_net_accept(listen_fd, &client_ip, &client_port);
        if (client_fd < 0) {
            if (sg_http_server_running) {
                PR_ERR("Failed to accept client connection");
            }
            continue;
        }

        char *client_ip_str = tal_net_addr2str(client_ip);
        PR_DEBUG("Accepted connection from %s:%d", client_ip_str, client_port);

        memset(recv_buf, 0, HTTP_RECV_BUFFER_SIZE);
        net_errno = tal_net_recv(client_fd, recv_buf, HTTP_RECV_BUFFER_SIZE - 1);

        if (net_errno <= 0) {
            PR_ERR("Failed to receive data from client");
            tal_net_close(client_fd);
            continue;
        }

        recv_buf[net_errno] = '\0';
        PR_DEBUG("Received HTTP request from %s:%d", client_ip_str, client_port);

        if (__parse_http_request(recv_buf, &request) == 0) {
            PR_DEBUG("Parsed request: %s %s", request.method, request.path);

            int response_len = __generate_http_response(&request, response_buf, HTTP_RESPONSE_BUFFER_SIZE);
            if (response_len > 0) {
                tal_net_send(client_fd, response_buf, response_len);
                PR_DEBUG("Sent HTTP response (%d bytes)", response_len);
            } else {
                PR_ERR("Failed to generate HTTP response");
            }
        } else {
            PR_ERR("Failed to parse HTTP request");
            const char *error_response = "HTTP/1.1 400 Bad Request\r\n"
                                         "Content-Length: 13\r\n"
                                         "Connection: close\r\n\r\n"
                                         "Bad Request\n";
            tal_net_send(client_fd, error_response, strlen(error_response));
        }

        tal_net_close(client_fd);
    }

    tal_net_close(listen_fd);

exit:
    sg_http_server_running = false;
    tal_thread_delete(sg_http_server);
    sg_http_server = NULL;
    PR_NOTICE("--- ChatBot HTTP server stopped");
}

/**
 * @brief Initialize HTTP server
 */
OPERATE_RET app_http_server_init(void)
{
    if (sg_http_server != NULL) {
        PR_DEBUG("HTTP server already initialized");
        return OPRT_OK;
    }

    THREAD_CFG_T thread_cfg = {
        .thrdname = "chat_http_srv",
        .stackDepth = 8192,
        .priority = THREAD_PRIO_3,
    };

    OPERATE_RET rt = tal_thread_create_and_start(&sg_http_server, NULL, NULL, __http_server_task, NULL, &thread_cfg);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to create HTTP server thread, rt:%d", rt);
        return rt;
    }

    PR_DEBUG("HTTP server initialized successfully");
    return OPRT_OK;
}

/**
 * @brief Stop HTTP server
 */
OPERATE_RET app_http_server_stop(void)
{
    if (!sg_http_server_running) {
        return OPRT_OK;
    }

    sg_http_server_running = false;

    // Wait for thread to stop
    int timeout = 50; // 5 seconds
    while (sg_http_server != NULL && timeout > 0) {
        tal_system_sleep(100);
        timeout--;
    }

    if (sg_http_server != NULL) {
        PR_WARN("HTTP server thread did not stop gracefully");
    }

    return OPRT_OK;
}

/**
 * @brief Check if HTTP server is running
 */
bool app_http_server_is_running(void)
{
    return sg_http_server_running;
}