/**
 * @file mqtt.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2020-06-25
 *
 * @copyright Copyright (c) 2020
 *
 */

#include "debug_tools.h"

#include "esp_log.h"
#if (DEBUG_MQTT == VERBOS)
#define LOG_MQTT ESP_LOGI
#define LOG_MQTT_V ESP_LOGI
#elif (DEBUG_MQTT == INFO)
#define LOG_MQTT ESP_LOGI
#define LOG_MQTT_V ESP_LOGD
#else
#define LOG_MQTT ESP_LOGD
#define LOG_MQTT_V ESP_LOGV
#endif

#include "assertion_tools.h"
#include "mqtt_class.h"
#include "nvs_tools.h"
#include "utilities.hpp"
#include <memory>
#include <string>
const esp_mqtt_client_config_t Mqtt::mqttDefaultCfg;
extern const char mqtt_client_crt_start[] asm("_binary_mqtt_client_crt_start");
extern const char mqtt_client_crt_end[] asm("_binary_mqtt_client_crt_end");

extern const char mqtt_client_key_start[] asm("_binary_mqtt_client_key_start");
extern const char mqtt_client_key_end[] asm("_binary_mqtt_client_key_end");
////////////////////////////////////////////////////////////////
///
/// static variables, constants and configuration assignments
///
////////////////////////////////////////////////////////////////

const esp_mqtt_client_config_t Mqtt::mqttDefaultCfg;
RTC_DATA_ATTR uint8_t Mqtt::disconnectionCounter;
/**
 * @brief Constructor , rest topic and config
 *
 */
Mqtt::Mqtt(EventLoop_p_t& eventLoop) : Loop(eventLoop)
{
	mqttRecieveDataQueue = xQueueCreate(10, sizeof(void*));

	privateTopicList.clear();
}

/**
 * @brief init and start
 * this will allocate topics, attach events and start listern automatically (by event)
 *
 * @param mqttCfg
 * @return esp_err_t
 */
esp_err_t Mqtt::Init(const std::string& device, const esp_mqtt_client_config_t& mqttCfg)
{
	LOG_MQTT_V(TAG, " initialization started ");
	if (isInitialized || isConnected)
	{
		LOGW(TAG, " Already initialized ! OK, will retry connection automatically");
		return ESP_ERR_INVALID_STATE;
	}
	LoadFromNVS();
	MqttUserConfig.activeAPIConfig = mqttCfg;
	uint8_t mac[6];
	esp_efuse_mac_get_default(mac);
	std::string clienID = tools::MacToStr(mac, 4, false);
	MqttUserConfig.deviceStr += clienID;
	MqttUserConfig.activeAPIConfig.host = clienID.c_str();
	MqttUserConfig.activeAPIConfig.client_id = MqttUserConfig.deviceStr.c_str();
	const std::string prefix = "mqtt://";
	ESP_LOGW(TAG, "server=%s , port=%d ", MqttUserConfig.serverStr.c_str(), MqttUserConfig.port);
	if (MqttUserConfig.serverStr.compare(0, prefix.size(), prefix) != 0)
	{
		MqttUserConfig.serverStr = prefix + MqttUserConfig.serverStr;
	}
	MqttUserConfig.activeAPIConfig.uri = MqttUserConfig.serverStr.c_str();
	MqttUserConfig.activeAPIConfig.port = MqttUserConfig.port;
	MqttUserConfig.activeAPIConfig.lwt_topic = MqttUserConfig.lwillTopic.c_str();
	MqttUserConfig.activeAPIConfig.lwt_msg = MqttUserConfig.lwMsg.c_str();
	MqttUserConfig.activeAPIConfig.username = MqttUserConfig.username.c_str();
	MqttUserConfig.activeAPIConfig.password = MqttUserConfig.password.c_str();
	MqttUserConfig.clentAPIHandle = esp_mqtt_client_init(&MqttUserConfig.activeAPIConfig);
	esp_mqtt_client_register_event(MqttUserConfig.clentAPIHandle, MQTT_EVENT_ANY, MQTTEventCallbackHandler, this);
	LOG_MQTT(TAG, " Attempting to connect to %s:%d - user:%s , pass:%s as %s ", MqttUserConfig.activeAPIConfig.uri, MqttUserConfig.activeAPIConfig.port, MqttUserConfig.activeAPIConfig.username, MqttUserConfig.activeAPIConfig.password, MqttUserConfig.activeAPIConfig.host);
	esp_err_t ret = esp_mqtt_client_start(MqttUserConfig.clentAPIHandle);
	if (ret == ESP_OK)
	{
		isInitialized = true;
		return (ret);
	}
	else
	{
		ESP_LOGE(TAG, " Something went wrong in initialisation : %s", esp_err_to_name(ret));
		return (ret);
	}
}


Mqtt::~Mqtt()
{
	Disconnect();
	vQueueDelete(mqttRecieveDataQueue);
	esp_mqtt_client_destroy(MqttUserConfig.clentAPIHandle);
	isInitialized = false;
}

/**
 * @brief publish a message (struct)
 *
 * @param msg struct containing message context and settings
 * @return esp_err_t
 */
esp_err_t Mqtt::Publish(const MqttMsg_t& msg) const
{
	if (!isInitialized)
		return ESP_ERR_INVALID_STATE;
	int length = msg.length;
	if (msg.length == 0) // not set
	{
		length = msg.payload.length();
	}
	ASSERT_STRLEN_RET_ERR(msg.payload.c_str());
	ASSERT_STRLEN_RET_ERR(msg.topic.c_str());
	if (!msg.qos) // this mean message could be lost if sent now when not connected
	{
		ASSERT_INIT_AND_RETURN_ERR(Mqtt::isInitialized, TAG)
	}
	LOG_MQTT_V(TAG, "Sending Message %s in topic %s with Qos %d , retained %d", msg.payload.c_str(), msg.topic.c_str(), msg.qos, msg.retained);
	char test[500] = {};
	sprintf(test, "qos :%d retainde %d", msg.qos, msg.retained);
	/*ret = esp_mqtt_client_publish(MqttUserConfig.clentAPIHandle, msg.topic.c_str(), test, strlen(test), msg.qos, msg.retained);*/
	int ret = esp_mqtt_client_publish(MqttUserConfig.clentAPIHandle, msg.topic.c_str(), msg.payload.c_str(), length, msg.qos, msg.retained);
	if (ret == -1)
	{
		LOGE(TAG, "ERRRO Sending Message in topic %s with Qos %d with error", msg.topic.c_str(), msg.qos);
		return (ESP_FAIL);
	}
	return (ESP_OK);
}
void Mqtt::AddToSubscribeList(const std::string& topic)
{
	privateTopicList.push_back(topic);

}

void Mqtt::AddToSubscribeList(std::string&& topic)
{
	privateTopicList.push_back(std::move(topic));

}

esp_err_t Mqtt::Publish(const char* topic, const char* data, const uint8_t qos, const bool retained) const
{
	MqttMsg_t msg = {
		.retained = retained,
		.qos = qos,
		.payload = data,
		.topic = topic,
		.length = strlen(data),
	};
	return Publish(msg);
}

esp_err_t Mqtt::Publish(const std::string& topic, const std::string& data, const uint8_t qos, const bool retained) const
{
	MqttMsg_t msg = {
		.retained = retained,
		.qos = qos,
		.payload = data.c_str(),
		.topic = topic.c_str(),
		.length = data.length(),
	};
	return Publish(msg);
}
esp_err_t Mqtt::Publish(std::pair<std::string, std::string> data, const uint8_t qos, const bool retained) const
{
	MqttMsg_t msg = {
		.retained = retained,
		.qos = qos,
		.payload = data.second.c_str(),
		.topic = data.first.c_str(),
		.length = data.second.length(),
	};
	return Publish(msg);
}

esp_err_t Mqtt::PublishStaticPair(Mqtt * ptr, std::pair<std::string, std::string> data, const uint8_t qos, const bool retained)
{
	MqttMsg_t msg = {
		.retained = retained,
		.qos = qos,
		.payload = data.second.c_str(),
		.topic = data.first.c_str(),
		.length = data.second.length(),
	};
	return ptr->Publish(msg);
}

esp_err_t Mqtt::Publish(const char* topic, const std::string& data, const uint8_t qos, const bool retained) const
{
	MqttMsg_t msg = {
		.retained = retained,
		.qos = qos,
		.payload = data.c_str(),
		.topic = topic,
		.length = data.length(),
	};
	return Publish(msg);
}

/**
 * @brief subscribe to a topic , mqtt must be connected
 *
 * @param topic
 * @param qos
 * @return esp_err_t
 */
esp_err_t Mqtt::Subscribe(const char* topic, int qos) const
{
	ASSERT_INIT_AND_RETURN_ERR(Mqtt::isInitialized, TAG)
		if (esp_mqtt_client_subscribe(MqttUserConfig.clentAPIHandle, topic, qos) == -1)
		{
			LOGE(TAG, " subscribe failed returned with -1");
			return ESP_FAIL;
		}
		else
		{
			LOG_MQTT_V(TAG, " subscribe %s with Qos %d OK", topic, qos);
			return ESP_OK;
		}
}

/**
 * @brief
 *
 * @param topic
 * @param qos
 * @return esp_err_t
 */
esp_err_t Mqtt::Subscribe(const std::string& topic, int qos) const
{
	return Subscribe(topic.c_str(), qos);
}

/**
 * @brief unsubsribe from a topic
 *
 * @param topic
 * @return esp_err_t
 */
esp_err_t Mqtt::UnSubscribe(const char* topic) const
{
	ASSERT_INIT_AND_RETURN_ERR(Mqtt::isInitialized, TAG)
		if (esp_mqtt_client_unsubscribe(MqttUserConfig.clentAPIHandle, topic) == -1)
		{
			LOGE(TAG, " unsubscribe failed returned with -1");
			return ESP_FAIL;
		}
		else
		{
			LOG_MQTT_V(TAG, " unsubscribe %s OK", topic);
			return ESP_OK;
		}
}

esp_err_t Mqtt::SetLastWill(const std::string& LwTopic, const std::string& lwMsg)
{
	MqttUserConfig.lwillTopic = LwTopic;
	MqttUserConfig.lwMsg = lwMsg;
	return ESP_OK;
}


/**
 * @brief disconnect mqtt clentAPIHandle (stop)
 *
 */
esp_err_t Mqtt::Disconnect()
{
	ASSERT_INIT_AND_RETURN_ERR(Mqtt::isInitialized, TAG)
		esp_err_t ret = esp_mqtt_client_disconnect(MqttUserConfig.clentAPIHandle);
	ret = esp_mqtt_client_stop(MqttUserConfig.clentAPIHandle);
	isConnected = false;
	if (ret != ESP_OK)
	{
		LOGE(TAG, " Disconnect failed ");
		return ESP_FAIL;
	}
	else
	{
		LOG_MQTT_V(TAG, " Disconnect OK");
		return ESP_OK;
	}
	return ESP_OK;
}

/**
 * @brief get mqtt connection status
 *
 * @return true mqtt connected
 * @return false mqtt disconnected
 */
bool Mqtt::IsConnected() const
{
	return (isConnected);
}

void Mqtt::MQTTEventCallbackHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* _event_data)
{
	Mqtt* _this = (Mqtt*)handler_args;
	esp_mqtt_event_id_t event = static_cast<esp_mqtt_event_id_t>(event_id);
	esp_mqtt_event_handle_t event_data = (esp_mqtt_event_handle_t)_event_data;
	switch (event)
	{
	case MQTT_EVENT_ANY:
	{
	}
	case MQTT_EVENT_ERROR:
	{
		_this->Loop->post_event_data(_this->EVENT_ERROR, *event_data->error_handle);
		_this->ErrorHandler(*event_data->error_handle);
		break;
	}
	case MQTT_EVENT_CONNECTED:
	{
		_this->Loop->post_event(_this->EVENT_CONNECTED);
		_this->ConnectedHandler();
		break;
	}
	case MQTT_EVENT_DISCONNECTED:
	{
		_this->Loop->post_event(_this->EVENT_DISCONNECT);
		_this->DisconnectedHandler();
		break;
	}
	case MQTT_EVENT_SUBSCRIBED:
	{
		_this->Loop->post_event(_this->EVENT_SUBSCRIBED);
		LOG_MQTT_V(TAG, "EVENT SUBSCRIBED");
		break;
	}
	case MQTT_EVENT_UNSUBSCRIBED:
	{
		_this->Loop->post_event(_this->EVENT_UNSUBSCRIBED);
		LOG_MQTT_V(TAG, "EVENT UNSUBSCRIBED");
		break;
	}
	case MQTT_EVENT_PUBLISHED:
	{
		ESP_LOGV(TAG, "EVENT PUBLISHED OK");
		_this->Loop->post_event(_this->EVENT_PUBLISH);
		break;
	}
	case MQTT_EVENT_DATA:
	{
		if (event_data->topic_len && event_data->data_len)
		{

			_this->Loop->post_event(_this->EVENT_DATA);
			_this->DataHandlerStatic(event_data->topic, event_data->topic_len, event_data->data, event_data->data_len);
		}
		break;
	}
	default:
		break;
	}
}

/**
 * @brief callback after data recieved
 *
 *  will send an event to the xMqttEventGroup and send the
 * 	recieved data via a queue to the outside
 */
void Mqtt::DataHandlerStatic(const char* topicBuffer, const int topic_len, const char* dataBuffer, const int data_len)
{
	//  redirect to Loop
	this->DataHandlerToQueue(topicBuffer, topic_len, dataBuffer, data_len);
	return;
}


void Mqtt::DataHandlerToQueue(const char* topicBuffer, const int topic_len, const char* dataBuffer, const int data_len)
{
	LOG_MQTT_V(TAG, " message HANDLER  %.*s, on topic %.*s", data_len, dataBuffer, topic_len, topicBuffer);
	MqttStringConstruct* const msg = new MqttStringConstruct(topicBuffer, topic_len, dataBuffer, data_len);
	auto ret = xQueueSend(this->mqttRecieveDataQueue, (void*)(&msg), pdMS_TO_TICKS(200));
	if (ret != pdPASS)
	{
		delete msg;
	}
	else
	{
		Loop->post_event(this->EVENT_DATA);
	}
	return;
}

QueueHandle_t Mqtt::GetDataReceiverHandle()
{
	return this->mqttRecieveDataQueue;
}

/**
 * @brief error handler
 * 		print all error types
 * 		send error event to the xMqttEventGroup
 *
 */
void Mqtt::ErrorHandler(esp_mqtt_error_codes_t err)
{
	if (err.esp_tls_last_esp_err)
		LOGE(TAG, "EVENT ERROR !!  %s", esp_err_to_name(err.esp_tls_last_esp_err));
	if (err.esp_tls_stack_err)
		LOGE(TAG, "EVENT ERROR !! esp_tls_stack_err %d", err.esp_tls_stack_err);
	if (err.esp_tls_cert_verify_flags)
		LOGE(TAG, "EVENT ERROR !! esp_tls_cert_verify_flags %d", err.esp_tls_cert_verify_flags);
	switch (err.error_type)
	{
	case MQTT_ERROR_TYPE_NONE:
		LOGE(TAG, "MQTT_ERROR_TYPE_NONE");
		break;
	case MQTT_ERROR_TYPE_ESP_TLS:
		LOGE(TAG, "MQTT_ERROR_TYPE_ESP_TLS");
		break;
	case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
		LOGE(TAG, "MQTT_ERROR_TYPE_CONNECTION_REFUSED");
		break;
	default:
		break;
	}
	switch (err.connect_return_code)
	{
	case MQTT_CONNECTION_ACCEPTED:
		LOGE(TAG, "MQTT_CONNECTION_ACCEPTED"); /*!< Connection accepted  */
		break;
	case MQTT_CONNECTION_REFUSE_PROTOCOL:
		LOGE(TAG, "MQTT_CONNECTION_REFUSE_PROTOCOL"); /*!< MQTT connection refused reason: Wrong protocol */
		break;
	case MQTT_CONNECTION_REFUSE_ID_REJECTED:
		LOGE(TAG, "MQTT_CONNECTION_REFUSE_ID_REJECTED"); /*!< MQTT connection refused reason: ID rejected */
		break;
	case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE:
		LOGE(TAG, "MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE"); /*!< MQTT connection refused reason: Server unavailable */
		break;
	case MQTT_CONNECTION_REFUSE_BAD_USERNAME:
		LOGE(TAG, "MQTT_CONNECTION_REFUSE_BAD_USERNAME"); /*!< MQTT connection refused reason: Wrong user */
		break;
	case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED:
		LOGE(TAG, "MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED"); /*!< MQTT connection refused reason: Wrong username or password */
		break;
	default:
		break;
	}
	//isInitialized = false;
}

/**
 * @brief callback after mqtt connection established
 *
 *
 */
void Mqtt::ConnectedHandler()
{
	isInitialized = true;
	isConnected = true;
	LOG_MQTT(TAG, "EVENT CONNECTED");
	ResetDisconnectionCounter();
	SubscribeAllTopics();
}

/**
 * @brief callback after Disconnection event
 *
 *		will send an event to the xMqttEventGroup
 */
void Mqtt::DisconnectedHandler()
{
	LOGW(TAG, "EVENT DISCONNECTED");
	disconnectionCounter++;
	if (disconnectionCounter > MAX_DISCONNECTION_COUNT)
	{
		Loop->post_event_data(EVENT_FAILED_RECONNECT, disconnectionCounter);
	}
}

/**
 * @brief reset disconnection timer
 *
 */
void Mqtt::ResetDisconnectionCounter()
{
	disconnectionCounter = 0;
}

/**
 * @brief
 *
 * @param topic
 * @param qos
 * @return esp_err_t
 */
esp_err_t Mqtt::SubscribeAllTopics()
{
	ASSERT_INIT_AND_RETURN_ERR(Mqtt::isInitialized, TAG)
		esp_err_t ret = ESP_OK;
	for (const auto& topic : privateTopicList)
	{
		ret |= Subscribe(topic, MqttUserConfig.defaultQos);
	}
	return ret;
}


//CONFIG OVERRIDE

esp_err_t Mqtt::LoadFromNVS()
{
	NvsPointer nvs = OPEN_NVS_DEAFULT();
	esp_err_t ret = ESP_FAIL;
	if (nvs->isOpen())
	{
		ret |= nvs->getS("deviceName", MqttUserConfig.deviceStr);
		ret |= nvs->getS("mqtt_server", MqttUserConfig.serverStr);
		ret |= nvs->get("mqtt_port", MqttUserConfig.port);
		ret |= nvs->getS("mqtt_user", MqttUserConfig.username);
		ret |= nvs->getS("mqtt_pass", MqttUserConfig.password);
		ret |= nvs->getS("room", MqttUserConfig.roomName);
		ESP_LOGI(TAG, "%s  %s  %d  %s  %s  %s ", MqttUserConfig.deviceStr.c_str(), MqttUserConfig.serverStr.c_str(), MqttUserConfig.port, MqttUserConfig.username.c_str(), MqttUserConfig.password.c_str(), MqttUserConfig.roomName.c_str());
	}
	return ret;
}

std::shared_ptr<Mqtt> MqttClass = nullptr;
