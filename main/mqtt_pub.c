/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_mac.h" // esp_base_mac_addr_get
#include "cJSON.h"
#include "mqtt_client.h"

#include "adc.h"

static const char *TAG = "PUB";

EventGroupHandle_t mqtt_status_event_group;
#define MQTT_CONNECTED_BIT BIT2

extern QueueHandle_t xQueuePublish;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
#else
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
#endif
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
	esp_mqtt_event_handle_t event = event_data;
#endif
	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			xEventGroupSetBits(mqtt_status_event_group, MQTT_CONNECTED_BIT);
			break;
		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
			xEventGroupClearBits(mqtt_status_event_group, MQTT_CONNECTED_BIT);
			break;
		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_PUBLISHED:
			ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_DATA:
			ESP_LOGI(TAG, "MQTT_EVENT_DATA");
			break;
		case MQTT_EVENT_ERROR:
			ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
			break;
		default:
			ESP_LOGI(TAG, "Other event id:%d", event->event_id);
			break;
	}
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
	return ESP_OK;
#endif
}

esp_err_t query_mdns_host(const char * host_name, char *ip);
void convert_mdns_host(char * from, char * to);

void mqtt_pub(void *pvParameters)
{
	ESP_LOGI(TAG, "Start Publish Broker:%s", CONFIG_MQTT_BROKER);

	// Create Eventgroup
	mqtt_status_event_group = xEventGroupCreate();
	configASSERT( mqtt_status_event_group );
	xEventGroupClearBits(mqtt_status_event_group, MQTT_CONNECTED_BIT);

	// Set client id from mac
	uint8_t mac[8];
	ESP_ERROR_CHECK(esp_base_mac_addr_get(mac));
	for(int i=0;i<8;i++) {
		ESP_LOGD(TAG, "mac[%d]=%x", i, mac[i]);
	}
	char client_id[64];
	sprintf(client_id, "pub-%02x%02x%02x%02x%02x%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	ESP_LOGI(TAG, "client_id=[%s]", client_id);

	// Resolve mDNS host name
	char ip[128];
	ESP_LOGI(TAG, "CONFIG_MQTT_BROKER=[%s]", CONFIG_MQTT_BROKER);
	convert_mdns_host(CONFIG_MQTT_BROKER, ip);
	ESP_LOGI(TAG, "ip=[%s]", ip);
	char uri[138];
	sprintf(uri, "mqtt://%s", ip);
	ESP_LOGI(TAG, "uri=[%s]", uri);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
	esp_mqtt_client_config_t mqtt_cfg = {
		.broker.address.uri = uri,
		.broker.address.port = 1883,
#if CONFIG_BROKER_AUTHENTICATION
		.credentials.username = CONFIG_AUTHENTICATION_USERNAME,
		.credentials.authentication.password = CONFIG_AUTHENTICATION_PASSWORD,
#endif
		.credentials.client_id = client_id
	};
#else
	esp_mqtt_client_config_t mqtt_cfg = {
		.uri = uri,
		.port = 1883, 
		.event_handle = mqtt_event_handler,
#if CONFIG_BROKER_AUTHENTICATION
		.username = CONFIG_AUTHENTICATION_USERNAME,
		.password = CONFIG_AUTHENTICATION_PASSWORD,
#endif
		.client_id = client_id
	};
#endif


	esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
	esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
#endif

	esp_mqtt_client_start(mqtt_client);
	xEventGroupWaitBits(mqtt_status_event_group, MQTT_CONNECTED_BIT, false, true, portMAX_DELAY);
	ESP_LOGI(TAG, "Connect to MQTT Server");

	ADC_t adcBuf;
	while (1) {
		BaseType_t received = xQueueReceive(xQueuePublish, &adcBuf, portMAX_DELAY);
		ESP_LOGI(TAG, "xQueueReceive received=%d", received);
		if (received == pdTRUE) {
			EventBits_t EventBits = xEventGroupGetBits(mqtt_status_event_group);
			ESP_LOGI(TAG, "EventBits=0x%"PRIx32, EventBits);
			if (EventBits & MQTT_CONNECTED_BIT) {
				ESP_LOGI(TAG, "output=%d", adcBuf.adc_output);
				ESP_LOGI(TAG, "adc_channel1=%d adc_gpio1=%d adc_value1=%d", adcBuf.adc_channel1, adcBuf.adc_gpio1, adcBuf.adc_value1);

#if CONFIG_TOPIC_JSON
				char topic[64];
				cJSON *root;
				sprintf(topic, "%s/json", CONFIG_MQTT_PUB_TOPIC);
				root = cJSON_CreateObject();
				cJSON_AddNumberToObject(root, "adc1", adcBuf.adc_value1);
#if CONFIG_ENABLE_ADC2
				ESP_LOGI(TAG, "adc_channel2=%d adc_gpio2=%d adc_value2=%d", adcBuf.adc_channel2, adcBuf.adc_gpio2, adcBuf.adc_value2);
				cJSON_AddNumberToObject(root, "adc2", adcBuf.adc_value2);
#endif
#if CONFIG_ENABLE_ADC3
				ESP_LOGI(TAG, "adc_channel3=%d adc_gpio3=%d adc_value3=%d", adcBuf.adc_channel3, adcBuf.adc_gpio3, adcBuf.adc_value3);
				cJSON_AddNumberToObject(root, "adc3", adcBuf.adc_value3);
#endif
				char *my_json_string = cJSON_Print(root);
				ESP_LOGI(TAG, "my_json_string\n%s",my_json_string);
				cJSON_Delete(root);
				//int msg_id = esp_mqtt_client_publish(mqtt_client, topic, my_json_string, 0, 1, 0);
				//ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
				esp_mqtt_client_publish(mqtt_client, topic, my_json_string, 0, 1, 0);
				cJSON_free(my_json_string);
#else
				char topic[64];
				char payload[64];
				sprintf(topic, "%s/adc1", CONFIG_MQTT_PUB_TOPIC);
				sprintf(payload, "%d", adcBuf.adc_value1);
				esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
#if CONFIG_ENABLE_ADC2
				ESP_LOGI(TAG, "adc_channel2=%d adc_gpio2=%d adc_value2=%d", adcBuf.adc_channel2, adcBuf.adc_gpio2, adcBuf.adc_value2);
				sprintf(topic, "%s/adc2", CONFIG_MQTT_PUB_TOPIC);
				sprintf(payload, "%d", adcBuf.adc_value2);
				esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
#endif
#if CONFIG_ENABLE_ADC3
				ESP_LOGI(TAG, "adc_channel3=%d adc_gpio3=%d adc_value3=%d", adcBuf.adc_channel3, adcBuf.adc_gpio3, adcBuf.adc_value3);
				sprintf(topic, "%s/adc3", CONFIG_MQTT_PUB_TOPIC);
				sprintf(payload, "%d", adcBuf.adc_value3);
				esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
#endif

#endif

			} else {
				ESP_LOGW(TAG, "Disconnect to MQTT Server. Skip to send");
			}
		} else {
			ESP_LOGE(TAG, "xQueueReceive fail");
			break;
		}
	}

	// Stop connection
	ESP_LOGI(TAG, "Task Delete");
	esp_mqtt_client_stop(mqtt_client);
	vTaskDelete(NULL);

}
