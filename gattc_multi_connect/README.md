Gatt Client Multi Connection
========================

## 寫死版本 
* file : ```../files/gattc_multi_connect1```
* config要改：
    * Include GATT server module(GATTS) = cancel
    * BT/BLE MAX ACL CONNECTIONS(1~7) = 7
* 要改的全域
    * #define CONN_DEVICE_NUM  7
    * #define PROFILE_NUM 7
* ```static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM]``` 裡面記得要加對應handler
* profile handler裡面要改的參數:
    * conn_device[]
    * get_service[]
    * gl_profile_tab[]


## handler合併 
* file : ```../files/gattc_multi_connect2```
* config要改：
    * Include GATT server module(GATTS) = cancel
    * BT/BLE MAX ACL CONNECTIONS(1~7) = 7
* 要改的全域
    * #define CONN_DEVICE_NUM  7
    * #define PROFILE_NUM 7

## 可以看到notify訊息版＆加入不同service 
* file : ```../files/gattc_multi_connect3```
* config要改：
    * Include GATT server module(GATTS) = cancel
    * BT/BLE MAX ACL CONNECTIONS(1~7) = 7
* 要改的全域
    * #define CONN_DEVICE_NUM  7
    * #define PROFILE_NUM 7
* 其他
    * ```app_main```開頭做初始化

## handler合併 （不同service）
* file : ```../files/gattc_multi_connect4```
* config要改：
    * Include GATT server module(GATTS) = cancel
    * BT/BLE MAX ACL CONNECTIONS(1~7) = 7
* 要改的全域
    * #define CONN_DEVICE_NUM  7
    * #define PROFILE_NUM 7
* 其他
    * 在scan時```esp_ble_gattc_open```前要做uuid設定 

## 用bda(mac+2)來找裝置 ： 避免遇到名字相同的情況
* file : ```../files/gattc_multi_connect5```
* config要改：
    * Include GATT server module(GATTS) = cancel
    * BT/BLE MAX ACL CONNECTIONS(1~7) = 7
* 要改的全域
    * #define CONN_DEVICE_NUM  7
    * #define PROFILE_NUM 7
* 新增 struct
```c=
struct remote_device {
    char name[50];
    uint8_t bda[6];
    uint16_t serviceUUID;
    uint16_t charUUID;
};
```
* 其他
    * ```app_main```開頭做初始化
    * 在scan時```esp_ble_gattc_open```前要做uuid設定 

-----
* [Reference : ESP-IDF Gatt Client Multi Connection Demo](https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/ble/gattc_multi_connect)




