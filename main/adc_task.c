/*
	adc example.
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
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "adc.h"

static const char *TAG = "ADC";

#include "soc/adc_channel.h"
#if ESP_IDF_VERSION_MAJOR == 5 && ESP_IDF_VERSION_MINOR == 0
#include "driver/adc.h" //	Need legacy adc driver for ADC1_GPIOxx_CHANNEL
#endif
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

extern QueueHandle_t xQueuePublish;

// convert from gpio to adc1 channel
adc_channel_t gpio2adc(int gpio) {
#if CONFIG_IDF_TARGET_ESP32
	if (gpio == 32) return ADC1_GPIO32_CHANNEL;
	if (gpio == 33) return ADC1_GPIO33_CHANNEL;
	if (gpio == 34) return ADC1_GPIO34_CHANNEL;
	if (gpio == 35) return ADC1_GPIO35_CHANNEL;
	if (gpio == 36) return ADC1_GPIO36_CHANNEL;
	if (gpio == 37) return ADC1_GPIO37_CHANNEL;
	if (gpio == 38) return ADC1_GPIO38_CHANNEL;
	if (gpio == 39) return ADC1_GPIO39_CHANNEL;

#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
	if (gpio == 1) return ADC1_GPIO1_CHANNEL;
	if (gpio == 2) return ADC1_GPIO2_CHANNEL;
	if (gpio == 3) return ADC1_GPIO3_CHANNEL;
	if (gpio == 4) return ADC1_GPIO4_CHANNEL;
	if (gpio == 5) return ADC1_GPIO5_CHANNEL;
	if (gpio == 6) return ADC1_GPIO6_CHANNEL;
	if (gpio == 7) return ADC1_GPIO7_CHANNEL;
	if (gpio == 8) return ADC1_GPIO8_CHANNEL;
	if (gpio == 9) return ADC1_GPIO9_CHANNEL;
	if (gpio == 10) return ADC1_GPIO10_CHANNEL;

#elif CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32C3
	if (gpio == 0) return ADC1_GPIO0_CHANNEL;
	if (gpio == 1) return ADC1_GPIO1_CHANNEL;
	if (gpio == 2) return ADC1_GPIO2_CHANNEL;
	if (gpio == 3) return ADC1_GPIO3_CHANNEL;
	if (gpio == 4) return ADC1_GPIO4_CHANNEL;

#elif CONFIG_IDF_TARGET_ESP32C6
	if (gpio == 0) return ADC1_GPIO0_CHANNEL;
	if (gpio == 1) return ADC1_GPIO1_CHANNEL;
	if (gpio == 2) return ADC1_GPIO2_CHANNEL;
	if (gpio == 3) return ADC1_GPIO3_CHANNEL;
	if (gpio == 4) return ADC1_GPIO4_CHANNEL;
	if (gpio == 5) return ADC1_GPIO5_CHANNEL;
	if (gpio == 6) return ADC1_GPIO6_CHANNEL;

#endif
	return -1;
}

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
	adc_cali_handle_t handle = NULL;
	esp_err_t ret = ESP_FAIL;
	bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	if (!calibrated) {
		ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
		adc_cali_curve_fitting_config_t cali_config = {
			.unit_id = unit,
			.atten = atten,
			.bitwidth = ADC_BITWIDTH_DEFAULT,
		};
		ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	if (!calibrated) {
		ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
		adc_cali_line_fitting_config_t cali_config = {
			.unit_id = unit,
			.atten = atten,
			.bitwidth = ADC_BITWIDTH_DEFAULT,
		};
		ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}
#endif

	*out_handle = handle;
	if (ret == ESP_OK) {
		ESP_LOGI(TAG, "Calibration Success");
	} else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
		ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
	} else {
		ESP_LOGE(TAG, "Invalid arg or no memory");
	}

	return calibrated;
}

#define NO_OF_SAMPLES 64 //Multisampling


void adc_task(void* pvParameters) {
	ESP_LOGI(TAG, "Start");

	// ADC1 Calibration Init
	adc_cali_handle_t adc1_cali_handle = NULL;
	bool do_calibration = adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_11, &adc1_cali_handle);
	if (do_calibration == false) {
		ESP_LOGE(TAG, "calibration fail");
		vTaskDelete(NULL);
	}

	// ADC1 Init
	adc_oneshot_unit_handle_t adc1_handle;
	adc_oneshot_unit_init_cfg_t init_config1 = {
		.unit_id = ADC_UNIT_1,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

	// ADC1 config
	adc_oneshot_chan_cfg_t config = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = ADC_ATTEN_DB_11,
	};
	adc_channel_t adc1_channel1 = gpio2adc(CONFIG_ADC1_GPIO);
	ESP_LOGI(TAG, "CONFIG_ADC1_GPIO=%d adc1_channel1=%d", CONFIG_ADC1_GPIO, adc1_channel1);
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc1_channel1, &config));

#if CONFIG_ENABLE_ADC2
	adc_channel_t adc1_channel2 = gpio2adc(CONFIG_ADC2_GPIO);
	ESP_LOGI(TAG, "CONFIG_ADC2_GPIO=%d adc1_channel2=%d", CONFIG_ADC2_GPIO, adc1_channel2);
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc1_channel2, &config));
	//sprintf(meter2, "GPIO%02d", CONFIG_ADC2_GPIO);
#endif

#if CONFIG_ENABLE_ADC3
	adc_channel_t adc1_channel3 = gpio2adc(CONFIG_ADC3_GPIO);
	ESP_LOGI(TAG, "CONFIG_ADC3_GPIO=%d adc1_channel3=%d", CONFIG_ADC3_GPIO, adc1_channel3);
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc1_channel3, &config));
	//sprintf(meter3, "GPIO%02d", CONFIG_ADC3_GPIO);
#endif

	ADC_t adcBuf;
#if CONFIG_ADC_RAW
	adcBuf.adc_output = ADC_RAW;
#else
	adcBuf.adc_output = ADC_MV;
#endif

	while (1) {
		int voltage1 = 0;
		int32_t adc_reading1 = 0;
		int adc_raw;
		for (int i = 0; i < NO_OF_SAMPLES; i++) {
			ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channel1, &adc_raw));
			adc_reading1 += adc_raw;
		}
		adc_reading1 /= NO_OF_SAMPLES;
		ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_reading1, &voltage1));
		ESP_LOGI(TAG, "GPIO%02d adc1_channel1: %d Raw: %"PRIi32" Voltage: %dmV", CONFIG_ADC1_GPIO, adc1_channel1, adc_reading1, voltage1);
		adcBuf.adc_channel1 = adc1_channel1;
		adcBuf.adc_gpio1 = CONFIG_ADC1_GPIO;
#if CONFIG_ADC_RAW
		adcBuf.adc_value1 = adc_reading1;
#else
		adcBuf.adc_value1 = voltage1;
#endif

		int voltage2 = 0;
		adcBuf.adc_value2 = voltage2;
#if CONFIG_ENABLE_ADC2
		int32_t adc_reading2 = 0;
		for (int i = 0; i < NO_OF_SAMPLES; i++) {
			ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channel2, &adc_raw));
			adc_reading2 += adc_raw;
		}
		adc_reading2 /= NO_OF_SAMPLES;
		ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_reading2, &voltage2));
		ESP_LOGI(TAG, "GPIO%02d adc1_channel2: %d Raw: %"PRIi32" Voltage: %dmV", CONFIG_ADC2_GPIO, adc1_channel2, adc_reading2, voltage2);
		adcBuf.adc_channel2 = adc1_channel2;
		adcBuf.adc_gpio2 = CONFIG_ADC2_GPIO;
#if CONFIG_ADC_RAW
		adcBuf.adc_value2 = adc_reading2;
#else
		adcBuf.adc_value2 = voltage2;
#endif
#endif // CONFIG_ENABLE_ADC2

		int voltage3 = 0;
		adcBuf.adc_value3 = voltage3;
#if CONFIG_ENABLE_ADC3
		int32_t adc_reading3 = 0;
		for (int i = 0; i < NO_OF_SAMPLES; i++) {
			ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc1_channel3, &adc_raw));
			adc_reading3 += adc_raw;
		}
		adc_reading3 /= NO_OF_SAMPLES;
		ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_reading3, &voltage3));
		ESP_LOGI(TAG, "GPIO%02d adc1_channel3: %d Raw: %"PRIi32" Voltage: %dmV", CONFIG_ADC3_GPIO, adc1_channel3, adc_reading3, voltage3);
		adcBuf.adc_channel3 = adc1_channel3;
		adcBuf.adc_gpio3 = CONFIG_ADC3_GPIO;
#if CONFIG_ADC_RAW
		adcBuf.adc_value3 = adc_reading3;
#else
		adcBuf.adc_value3 = voltage3;
#endif
#endif // CONFIG_ENABLE_ADC3

		xQueueSend(xQueuePublish, &adcBuf, 0);
		vTaskDelay(CONFIG_ADC_CYCLE);

	} // end while

	// Never reach here
	vTaskDelete(NULL);
}
