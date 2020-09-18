deps_config := \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/app_update/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/aws_iot/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/console/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/esp-google-iot/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/esp8266/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/esp_http_client/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/esp_http_server/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/freertos/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/libsodium/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/log/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/lwip/Kconfig \
	/Users/yenchia/esp/esp-idf/examples/bluetooth/bluedroid/ble/gatt_server/main/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/mdns/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/mqtt/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/newlib/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/pthread/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/spiffs/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/ssl/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/tcpip_adapter/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/util/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/vfs/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/wifi_provisioning/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/wpa_supplicant/Kconfig \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/bootloader/Kconfig.projbuild \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/esptool_py/Kconfig.projbuild \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/components/partition_table/Kconfig.projbuild \
	/Users/yenchia/esp/ESP8266_RTOS_SDK_OLD/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
