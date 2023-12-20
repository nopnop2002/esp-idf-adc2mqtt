# esp-idf-adc2mqtt
Demonstration of visualizing published MQTT data.   

![Postman-1](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/59923403-ba5f-4218-b47a-86e6998f63c5)

The ESP32 series has two ADCs: ADC1 and ADC2.   
You can use these to convert analog values to digital values.   
The analog values change dynamically and are suitable for this demonstration.   
This project uses ADC conversion values, but it can also be applied to analog sensors such as thermometers and hygrometers.   

# Software requiment
ESP-IDF V5.x.   
ESP-IDF V5.0 is required when using ESP32-C2.   
ESP-IDF V5.1 is required when using ESP32-C6.   

# Installation
```
git clone https://github.com/nopnop2002/esp-idf-adc2mqtt
cd esp-idf-adc2mqtt/
idf.py set-target {esp32/esp32s2/esp32s3/esp32c2/esp32c3/esp32c6}
idf.py menuconfig
idf.py flash monitor
```

__Note for ESP-IDF V5.0__   
ESP-IDF V5.0 ADC driver has a bug and does not define GPIO lookup macros like ADC1_GPIO32_CHANNEL.   
Therefore, when using ESP-IDF V5.0, it is necessary to include the legacy driver.   
This bug has been resolved in ESP-IDF V5.1.   

__Note for ESP32S2__   
__Only ESP32S2__ has ADC resolution of 13 bits.   

__Note for ESP32C6__   
The ESP32C6's ADC resolution is 12 bits.   
The theoretical maximum value is 4095, but it becomes 4081.   
Perhaps this is a bug in the ADC component of the ESP32C6.   

# Configuration
![config-top](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/24765885-be3e-48e0-a152-d51cac1715ce)
![config-app](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/9acaee7a-b2c9-4af1-9d7b-73c4712a17b4)


## WiFi Setting
Set the information of your access point.
![config-wifi](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/2774c6c5-1c9b-468f-9116-4c146fb0cd77)


## Broker Setting

MQTT broker is specified by one of the following.
- IP address   
 ```192.168.10.20```   
- mDNS host name   
 ```mqtt-broker.local```   
- Fully Qualified Domain Name   
 ```broker.emqx.io```

You can download the MQTT broker from [here](https://github.com/nopnop2002/esp-idf-mqtt-broker).   

![config-broker-1](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/6c356b93-c032-4c50-8965-dc31a78bcfcc)

Specifies the username and password if the server requires a password when connecting.   
[Here's](https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-debian-10) how to install and secure the Mosquitto MQTT messaging broker on Debian 10.   

![config-broker-2](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/14d93639-4132-43b7-8a20-c68fd17179d1)

## ADC Setting
Set the analog input gpio.   
![config-adc-1](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/c051efe6-6c60-4c0e-b0a0-cf9b283d3b1a)

It is possible to monitor 3 channels at the same time.   
![config-adc-2](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/e1e5c780-dfbe-4dcb-9246-dea7f4746fc3)

Analog input gpio for ESP32 is GPIO32 ~ GPIO39. 12Bits width.   
Analog input gpio for ESP32S2 is GPIO01 ~ GPIO10. 13Bits width.   
Analog input gpio for ESP32S3 is GPIO01 ~ GPIO10. 12Bits width.   
Analog input gpio for ESP32C2 is GPIO00 ~ GPIO04. 12Bits width.   
Analog input gpio for ESP32C3 is GPIO00 ~ GPIO04. 12Bits width.   
Analog input gpio for ESP32C3 is GPIO00 ~ GPIO06. 12Bits width.   

You can select raw data or millivolt-converted data as the ADC conversion value.   
![config-adc-3](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/59dbd445-920c-46e6-aaab-7960935596c6)

You can submit mqtt topics in JOSN format.
- Separate fomrmat   
Publish the results for adc1, adc2, and adc3 as separate topics.   
The default topic names are:   
```
/topic/test/adc1   
/topic/test/adc2   
/topic/test/adc3   
```

- JSON format   
Publish the results for adc1, adc2, and adc3 as one topics.   
The default topic names is 
```
/topic/test/json
```   
The topic format is json.   
```
{
        "adc1": 535,
        "adc2": 311,
        "adc3": 96
}
```

![config-adc-4](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/b9e44807-405f-44f1-9ad3-146ff91e4477)

# ADC Attenuation   
This project uses ADC_ATTEN_DB_11(11dB) for attenuation.   
11dB attenuation (ADC_ATTEN_DB_11) gives full-scale voltage 3.9V.   
But the range that can be measured accurately is as follows:   
- Measurable input voltage range for ESP32 is 150 mV ~ 2450 mV.   
- Measurable input voltage range for ESP32S2 is 0 mV ~ 2500 mV.   
- Measurable input voltage range for ESP32S3 is 0 mV ~ 3100 mV.   
- Measurable input voltage range for ESP32C2 is 0 mV ~ 2800 mV.   
- Measurable input voltage range for ESP32C3 is 0 mV ~ 2500 mV.   
- There is no information on ESP32C6 anywhere.   

# Analog source
Connect ESP32 and Analog source using wire cable.   
I used a variable resistor for testing.
```
                                          +---------------------------+
                                          |      variable resistor    |
ESP32 3.3V   -----------------------------+ Ra of variable resistor   |
                                          |                           |
                                          |                           |
ESP32 GPIO32 -------------------------+---+ Vout of variable resistor |
                                      |   |                           |
                  R1      R2      R3  |   |                           |
ESP32 GND    ----^^^--+--^^^--+--^^^--+   |                           |
                      |       |           |                           |
                      |       |           |                           |
ESP32 GPIO33 ---------+       |           |                           |
                              |           |                           |
                              |           |                           |
ESP32 GPIO34 -----------------+           |                           |
                                          |                           |
                                          |                           |
ESP32 GND    -----------------------------+ Rb of variable resistor   |
                                          |                           |
                                          +---------------------------+
```



# Visualize MQTT data

## Using postman application

Postman works as a native app on all major operating systems including Linux (32-bit/64-bit), macOS, and Windows (32-bit/64-bit).   
Postman supports MQTT visualization.   
You do not need to create a application for visualization.   
[Here's](https://blog.postman.com/postman-supports-mqtt-apis/) how to get started with MQTT with Postman.   

![Postman-1](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/57784303-357c-4373-ad2b-a5f6e7eedb3b)
![Postman-2](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/e2039aaa-4965-4d08-9689-bb0d7db985c6)
![Postman-3](https://github.com/nopnop2002/esp-idf-adc2mqtt/assets/6020549/4627b59d-fca1-4f7c-908f-4f36a2ad0a33)

## Using python
There is a lot of information on the internet about the Python + visualization library.   
- [matplotlib](https://matplotlib.org/)
- [seaborn](https://seaborn.pydata.org/index.html)
- [bokeh](https://bokeh.org/)
- [plotly](https://plotly.com/python/)

## Using node.js
There is a lot of information on the internet about the node.js + __real time__ visualization library.   
- [epoch](https://epochjs.github.io/epoch/real-time/)
- [plotly](https://plotly.com/javascript/streaming/)
- [chartjs-plugin-streaming](https://nagix.github.io/chartjs-plugin-streaming/1.9.0/)


