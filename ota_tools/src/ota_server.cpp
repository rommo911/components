#include <string.h>
#include "nvs_tools.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "lwip/sockets.h"
#include "ota_server.h"

OTA_Server::OTA_Server(EventLoop_p_t& EventLoop, int _port) : Task(TAG, CONFIG_OTA_SOCKET_TASK_STACK_SIZE, CONFIG_OTA_SOCKET_TASK_PRIORITY, CONFIG_OTA_SOCKET_TASK_CORE),
Loop(EventLoop)
{
    connect_socket = 0;
    port = CONFIG_OTA_SOCKET_DEFAULT_PORT;
    bufferSize = CONFIG_OTA_BUFFER_SIZE;
    server_socket = 0;
    socketDomain = 0;
    sin_famil = 0;
    s_addr.s_addr = htonl(INADDR_ANY);
    socketType = 0;
}

OTA_Server::~OTA_Server()
{
    StopService();
}

esp_err_t OTA_Server::StartService()
{
    if (TaskIsRunning())
    {
        return ESP_OK;
    }
    return StartTask(this);
}

esp_err_t OTA_Server::StopService()
{
    lwip_close(connect_socket);
    StopTask();
    return ESP_OK;
}
esp_err_t OTA_Server::SetPreOtaCb(ota_cb_t cb)
{
    if (cb != NULL)
    {
        OtaPreFlashCallback = cb;
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t OTA_Server::SetPostOtaCb(ota_cb_t cb)
{
    if (cb != NULL)
    {
        OtaPostFlashCallback = cb;
        return ESP_OK;
    }
    return ESP_FAIL;
}
esp_app_desc_t OTA_Server::GetCurrentVersion()
{
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info = {};
    esp_ota_get_partition_description(running, &running_app_info);
    return running_app_info;
}

int OTA_Server::get_socket_error_code(int socket)
{
    int result;
    u32_t optlen = sizeof(int);
    int err = getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen);
    if (err == -1)
    {
        ESP_LOGE(TAG, "getsockopt failed:%s", strerror(err));
        return -1;
    }
    return result;
}

int OTA_Server::show_socket_error_reason(const char* str, int socket)
{
    int err = get_socket_error_code(socket);

    if (err != 0)
    {
        ESP_LOGW(TAG, "%s socket error %d %s", str, err, strerror(err));
    }

    return err;
}

esp_err_t OTA_Server::create_tcp_server()
{
    ESP_LOGI(TAG, "server socket....port=%d", port);
    server_socket = 0;
    server_addr = {};
    socketDomain = AF_INET;
    socketType = SOCK_STREAM;
    server_socket = lwip_socket(socketDomain, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        show_socket_error_reason("create_server", server_socket);
        return ESP_FAIL;
    }
    sin_famil = AF_INET;
    server_addr.sin_family = sin_famil;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = s_addr.s_addr;
    if (lwip_bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        show_socket_error_reason("bind_server", server_socket);
        close(server_socket);
        return ESP_FAIL;
    }
    if (lwip_listen(server_socket, 5) < 0)
    {
        show_socket_error_reason("listen_server", server_socket);
        lwip_close(server_socket);
        return ESP_FAIL;
    }
    client_addr = {};
    unsigned int socklen = sizeof(client_addr);
    connect_socket = 0;
    connect_socket = lwip_accept(server_socket, (struct sockaddr*)&client_addr, &socklen);
    if (connect_socket < 0)
    {
        show_socket_error_reason("accept_server", connect_socket);
        lwip_close(server_socket);
        return ESP_FAIL;
    }
    /*connection established，now can send/recv*/
    lwip_close(server_socket);
    ESP_LOGI(TAG, "tcp connection established!");
    return ESP_OK;
}

esp_err_t OTA_Server::Do_OTA()
{
    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
        update_partition->subtype, update_partition->address);
    int recv_len;
    char* ota_buff = (char*)calloc(1, bufferSize);
    bool is_req_body_started = false;
    int content_length = -1;
    int content_received = 0;
    char authToken[64] = { 0 };
    esp_ota_handle_t ota_handle;
    do
    {
        recv_len = lwip_recv(connect_socket, ota_buff, bufferSize, 0);
        if (recv_len > 0)
        {
            if (!is_req_body_started)
            {
                const char* content_length_start = "Content-Length: ";
                const char* content_authToken = "token: ";
                char* content_length_start_p = strstr(ota_buff, content_length_start) + strlen(content_length_start);
                char* content_authToken_start_p = strstr(ota_buff, content_authToken) + strlen(content_authToken);
                sscanf(content_length_start_p, "%d", &content_length);
                sscanf(content_authToken_start_p, "%s", authToken);
                ESP_LOGI(TAG, "content_length=%d, authToken=%s", content_length, authToken);
                ESP_LOGI(TAG, "Detected content length: %d", content_length);
                if (strcmp(authToken, "MYPASSWORD") != 0)
                {
                    ESP_LOGE(TAG, "authToken error!");
                    return ESP_FAIL;
                }
                esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
                const char* header_end = "\r\n\r\n";
                char* body_start_p = strstr(ota_buff, header_end) + strlen(header_end);
                int body_part_len = recv_len - (body_start_p - ota_buff);
                esp_ota_write(ota_handle, body_start_p, body_part_len);
                content_received += body_part_len;
                is_req_body_started = true;
                if (this->OtaPreFlashCallback != nullptr)
                {
                    this->OtaPreFlashCallback(this);
                }
            }
            else
            {
                esp_ota_write(ota_handle, ota_buff, recv_len);
                content_received += recv_len;
            }
        }
        else if (recv_len < 0)
        {
            ESP_LOGE(TAG, "Error: recv data error! errno=%d", errno);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
        ESP_LOGI(TAG, " recieved %d", content_received);
    } while (recv_len > 0 && content_received < content_length);
    ESP_LOGI(TAG, "Binary transferred finished: %d bytes", content_received);
    esp_err_t err = (esp_ota_end(ota_handle));
    std::string res_buff;
    int send_len;
    if (err == ESP_OK)
    {
        err = esp_ota_set_boot_partition(update_partition);
        if (err == ESP_OK)
        {
            res_buff = "200 OK \n\n Success. Next boot partition is " + std::string(update_partition->label);
            const esp_partition_t* boot_partition = esp_ota_get_boot_partition();
            ESP_LOGI(TAG, "Next boot partition subtype %d at offset %s", boot_partition->subtype, boot_partition->label);
            if (this->OtaPostFlashCallback != nullptr)
            {
                this->OtaPostFlashCallback(this);
            }

        }
    }
    else
    {
        res_buff = "400 Bad Request Failure. Error code:" + std::string(esp_err_to_name(err));
    }
    send_len = res_buff.length();
    free(ota_buff);
    send(connect_socket, res_buff.c_str(), send_len, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    close(connect_socket);
    vTaskDelay(pdMS_TO_TICKS(1000));
    return err;
}

void OTA_Server::run(void* arg)
{

    while (1)
    {
        create_tcp_server(); // rhis will block until connection is open
        ctlLock->lock("DO OTA");
        esp_err_t ret = Do_OTA();
        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Prepare to restart system!");
            vTaskDelay(pdMS_TO_TICKS(1500));
            esp_restart();
        }
        ctlLock->unlock();
    }
}

OTA_Server_t otaServer = nullptr;