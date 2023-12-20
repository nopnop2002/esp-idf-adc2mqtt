typedef enum {ADC_RAW, ADC_MV} OUTPUT;

typedef struct {
	int adc_output;
	int adc_channel1;
	int adc_gpio1;
	int adc_value1;
	int adc_channel2;
	int adc_gpio2;
	int adc_value2;
	int adc_channel3;
	int adc_gpio3;
	int adc_value3;
} ADC_t;

