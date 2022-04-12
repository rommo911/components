/**
 * @def MQTT_H
 * @brief Header file for MQTT (over TCP)
 * provides a client for doing simple publish/subscribe messaging
 * with a server that supports MQTT
 * (basically allows ESP32 to talk with Node-RED).
 * See http://mqtt.org/documentation for additional MQTT documentation............................
 *
 *
 * Library adapted from MQTT example of ESP32
 */

#ifndef MQTT_H
#define MQTT_H
#include "config.hpp"
#include "system.hpp"
#include "esp_attr.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "semphr.hpp"
#include <esp_types.h>
#include <functional>
#include <string>
#include <list>
#define _DEVICENAME "rami"
class Mqtt;
extern std::shared_ptr<Mqtt> MqttDOL;

class Mqtt : public Config
{
	private:
	EventLoop_p_t Loop;
	struct MqttUserConfig_t
	{
		std::string serverStr, clienId, hostName, lwillTopic, lwMsg, username, password, TopicBase, boxBase, roomName;
		const uint8_t MAX_TOPIC_LENGHT = 60;
		uint32_t port = 443;
		std::string deviceStr = ("rami");
		uint8_t defaultQos = 1;
		esp_mqtt_client_handle_t clentAPIHandle;
		esp_mqtt_client_config_t activeAPIConfig;
	} MqttUserConfig;

	const uint8_t MAX_DISCONNECTION_COUNT = 10;
	bool isInitialized = false;
	bool isConnected = false;
	RTC_DATA_ATTR static uint8_t disconnectionCounter;
	std::vector<mqtt_data_callback_describtor_t> mqttRegisteredCommands;
	SemaphorePointer_t lock;

	public:
	static constexpr char TAG[] = "Mqtt-DOL";
	struct MqttMsg_t
	{
		bool retained;
		uint8_t qos;
		const std::string& payload;
		const std::string& topic;
		size_t length;
		size_t topicLen;
	};
	std::list<std::string> privateTopicList;
	std::string deviceName;
	const Event_t EVENT_DATA = { TAG, EventID_t(0) };
	const Event_t EVENT_PUBLISH = { TAG, EventID_t(1) };
	const Event_t EVENT_DISCONNECT = { TAG, EventID_t(2) };
	const Event_t EVENT_ERROR = { TAG, EventID_t(3) };
	const Event_t EVENT_CONNECTED = { TAG, EventID_t(4) };
	const Event_t EVENT_UNSUBSCRIBED = { TAG, EventID_t(5) };
	const Event_t EVENT_SUBSCRIBED = { TAG, EventID_t(6) };
	const Event_t EVENT_FAILED_RECONNECT = { TAG, EventID_t(7) };
	//
	Mqtt(EventLoop_p_t& eventLoop, const std::string& device);
	~Mqtt();
	esp_err_t Init(const esp_mqtt_client_config_t& mqttCfg = mqttDefaultCfg);
	void SetStationMacAddress(const uint8_t* mac_6);
	void SetActiveApMacAddress(const uint8_t* mac_6);
	esp_err_t Disconnect();
	bool IsConnected() const;
	esp_err_t UnSubscribe(const char* topic) const;
	esp_err_t Subscribe(const char* topic, int qos) const;
	esp_err_t Subscribe(const std::string& topic, int qos = 1) const;
	void AddToSubscribeList(std::string&& topic);
	void AddToSubscribeList(const std::string& topic);
	esp_err_t Publish(const MqttMsg_t& msg) const;
	esp_err_t Publish(const std::string&, const std::string&, uint8_t qos = 1, bool retained = false) const;
	esp_err_t Publish(const char*, const char*, uint8_t qos = 1, bool retained = false) const;
	esp_err_t Publish(const char*, const std::string&, uint8_t qos = 1, bool retained = false) const;
	esp_err_t Publish(std::pair<std::string, std::string> data, uint8_t qos, bool retained) const;
	const std::string& GetTopicBase() const;
	esp_err_t SetLastWill(const std::string& LwTopic, const std::string& lwMsg);
	esp_err_t SetConfigurationParameters(const json& config_in) override;
	esp_err_t SaveToNVS() override;
	esp_err_t RegisterCommand(const mqtt_data_callback_describtor_t& command);
	void ResetDisconnectionCounter();
	esp_err_t GetConfiguration(json& config_out) const override;

	private:
	//CONFIG OVERRIDE
	esp_err_t GetConfigurationStatus(json& config_out) const override;
	esp_err_t RestoreDefault() override;
	esp_err_t LoadFromNVS() override;
	esp_err_t Diagnose();
	//Handlers // non-static
	void ErrorHandler(esp_mqtt_error_codes_t);
	void ConnectedHandler();
	void DisconnectedHandler();
	void DataHandler(const std::string& topic, const std::string& data);
	static void MQTTEventCallbackHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);
	//generate topics from Mac string
	//subscribe to default and user defined topics
	esp_err_t SubscribeAllTopics();
	static constexpr esp_mqtt_client_config_t mqttDefaultCfg = {
		.event_handle = NULL,							/*!< handle for MQTT events as a callback in legacy mode */
		.event_loop_handle = NULL,						/*!< handle for MQTT event loop library */
		.host = CONFIG_MQTT_DEFAULT_HOSTNAME,			//"4xdfgd", /*!< MQTT server domain (ipv4 as string) */
		.uri = "mqtt://192.168.1.62",					/*< Complete MQTT broker URI */
		.port = 1800,									/*< MQTT server port  SSL 443 TCP 1883 */
		.client_id = "ESP32",							/*!< default client id is ``ESP32_%CHIPID%`` where %CHIPID% are last 3 bytes of MAC address in hex format */
		.username = "",									/*!< MQTT username */
		.password = "",									/*!< MQTT password */
		.lwt_topic = "home/status/connection",			/*!< LWT (Last Will and Testament) message topic (NULL by default) */
		.lwt_msg = CONFIG_MQTT_DEFAULT_LW_MSG,			/*!< LWT message (NULL by default) */
		.lwt_qos = CONFIG_MQTT_DEFAULT_LW_MSG_QOS,		/*!< LWT message qos */
		.lwt_retain = 1,								/*!< LWT retained message flag */
		.lwt_msg_len = 0,								/*!< LWT message length */
		.disable_clean_session = true,					/*!< mqtt clean session, default clean_session is true */
		.keepalive = 120,								/*!< mqtt keepalive, default is 120 seconds */
		.disable_auto_reconnect = false,				/*!< this mqtt client will reconnect to server (when errors/disconnect). Set disable_auto_reconnect=true to disable */
		.user_context = NULL,							/*!< pass user context to this option, then can receive that context in ``event->user_context`` */
		.task_prio = 2,									/*!< MQTT task priority, default is 5, can be changed in ``make menuconfig`` */
		.task_stack = 0,								/*!< MQTT task stack size, default is 6144 bytes, can be changed in ``make menuconfig`` */
		.buffer_size = CONFIG_MQTT_DEFAULT_BUFFER_SIZE, /*!< size of MQTT send/receive buffer, default is 1024 */
		.cert_pem = NULL,								/*!< Pointer to certificate data in PEM or DER format for server verify (with SSL), default is NULL, not required to verify the server. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in cert_len. */
		.cert_len = 0,									/*!< Length of the buffer pointed to by cert_pem. May be 0 for null-terminated pem */
		.client_cert_pem = NULL,						/*!< Pointer to certificate data in PEM or DER format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_key_pem` has to be provided. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in client_cert_len. */
		.client_cert_len = 0,							/*!< Length of the buffer pointed to by client_cert_pem. May be 0 for null-terminated pem */
		.client_key_pem = NULL,							/*!< Pointer to private key data in PEM or DER format for SSL mutual authentication, default is NULL, not required if mutual authentication is not needed. If it is not NULL, also `client_cert_pem` has to be provided. PEM-format must have a terminating NULL-character. DER-format requires the length to be passed in client_key_len */
		.client_key_len = 0,							/*!< Length of the buffer pointed to by client_key_pem. May be 0 for null-terminated pem */
		.transport = MQTT_TRANSPORT_OVER_TCP,			/*!< overrides URI transport */
		.refresh_connection_after_ms = 0,				/*!< Refresh connection after this value (in milliseconds) */
		.psk_hint_key = NULL,							/*!< Pointer to PSK struct defined in esp_tls.h to enable PSK authentication (as alternative to certificate verification). If not NULL and server/client certificates are NULL, PSK is enabled */
		.use_global_ca_store = 0,
		.crt_bundle_attach = NULL,			/*!< Use a global ca_store for all the connections in which this bool is set. */
		.reconnect_timeout_ms = 10000,					/*!< Reconnect to the broker after this value in miliseconds if auto reconnect is not disabled */
		.alpn_protos = NULL,							/*!< NULL-terminated list of supported application protocols to be used for ALPN */
		.clientkey_password = "",						/*!< Client key decryption password string */
		.clientkey_password_len = 0,					/*!< String length of the password pointed to by clientkey_password */
		.protocol_ver = MQTT_PROTOCOL_UNDEFINED,		/*!< MQTT protocol version used for connection, defaults to value from menuconfig*/
		.out_buffer_size = 0,							/*!< size of MQTT output buffer. If not defined, both output and input buffers have the same size defined as ``buffer_size`` */
		.skip_cert_common_name_check = true,			/*!< Skip any validation of server certificate CN field, this reduces the security of TLS and makes the mqtt client susceptible to MITM attacks  */
		.use_secure_element = false,					/*!< enable secure element for enabling SSL connection */
		.ds_data = NULL,								/*!< carrier of handle for digital signature parameters */
		.network_timeout_ms = 0,						/*!< Abort network operation if it is not completed after this value, in milliseconds (defaults to 10s) */
		.disable_keepalive = 0,
		.path = NULL,                  /*!< Path in the URI*/
		.message_retransmit_timeout = 0       /*!< timeout for retansmit of failded packet */
	};
};

#endif
