#ifndef _OTA_SERVER_H_
#define _OTA_SERVER_H__
#include "system.hpp"
#include "task_tools.h"
#include "lwip/sockets.h"
#include "esp_ota_ops.h"

typedef std::function<esp_err_t(void*)>  ota_cb_t;

class OTA_Server : public Task
{
public:
    const Event_t EVENT_OTA_START_FETCHING = {TAG, EventID_t(0)};
    const Event_t EVENT_FORCE_OTA = {TAG, EventID_t(1)};
    const Event_t EVENT_FAILED = {TAG, EventID_t(2)};
    const Event_t EVENT_NEW_VERSION_FOUND = {TAG, EventID_t(3)};
    const Event_t EVENT_NEW_VERSION_FLASHED = {TAG, EventID_t(4)};

private:
    int connect_socket = 0;
    struct sockaddr_in server_addr = {};
    int port = 443;
    int bufferSize = 2048;
    int server_socket = 0;
    int socketDomain = 0;
    sa_family_t sin_famil = {};
    struct in_addr s_addr = {};
    int socketType = 0;
    struct sockaddr_in client_addr = {};
    ota_cb_t OtaPreFlashCallback = nullptr;
    ota_cb_t OtaPostFlashCallback = nullptr;
    EventLoop_p_t Loop{nullptr};
    static constexpr char TAG[] = "ota_socket";
    int get_socket_error_code(int socket);
    int show_socket_error_reason(const char *str, int socket);
    esp_err_t create_tcp_server();
    esp_err_t Do_OTA();
    void run(void *arg);
    //

public:
    OTA_Server(EventLoop_p_t &EventLoop, int _port = 8032);
    ~OTA_Server();
    //set callback to be called before flashing
    esp_err_t SetPreOtaCb(ota_cb_t cb);
    //set callback to be called before flashing
    esp_err_t SetPostOtaCb(ota_cb_t cb);
    esp_err_t StartService();
    esp_err_t StopService();
    esp_app_desc_t GetCurrentVersion();
};

typedef std::unique_ptr<OTA_Server> OTA_Server_t;
extern OTA_Server_t otaServer;
#endif /* _OTA_SERVER_H_ */
