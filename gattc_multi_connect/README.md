Gatt Client Multi Connection
========================

## 寫死版本 
* commit: ver.01
* changed file : ```../files/gattc_multi_connect```
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
* commit: ver.02
* changed file : ```../files/gattc_multi_connect```
* config要改：
    * Include GATT server module(GATTS) = cancel
    * BT/BLE MAX ACL CONNECTIONS(1~7) = 7
* 要改的全域
    * #define CONN_DEVICE_NUM  7
    * #define PROFILE_NUM 7

## 可以看到notify訊息版＆加入不同service 
* commit: ver.03
* changed file : ```../files/gattc_multi_connect```
* config要改：
    * Include GATT server module(GATTS) = cancel
    * BT/BLE MAX ACL CONNECTIONS(1~7) = 7
* 要改的全域
    * #define CONN_DEVICE_NUM  7
    * #define PROFILE_NUM 7
* 其他
    * ```app_main```開頭做初始化

## handler合併 （不同service）
* commit: ver.04
* changed file : ```../files/gattc_multi_connect```
* config要改：
    * Include GATT server module(GATTS) = cancel
    * BT/BLE MAX ACL CONNECTIONS(1~7) = 7
* 要改的全域
    * #define CONN_DEVICE_NUM  7
    * #define PROFILE_NUM 7
* 其他
    * 在scan時```esp_ble_gattc_open```前要做uuid設定 

## 用bda(mac+2)來找裝置 ： 避免遇到名字相同的情況
* commit: ver.05
* changed file : ```../files/gattc_multi_connect```
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



## 精簡一點
* commit: ver.06
* changed file : ```../files/gattc_multi_connect```
* config要改：
    * Include GATT server module(GATTS) = cancel
    * BT/BLE MAX ACL CONNECTIONS(1~7) = 7
* 要改的全域
    * #define CONN_DEVICE_NUM  7
    * #define PROFILE_NUM 7
* 刪除
    scan裡面的`if (adv_name != NULL)`，因為已經用bda來找了所以不需要管裝置名稱是否有被設置
* 修改
    把`allConnected`改名為`connectedNum`，比較貼合功能


## 修改 connID bug
* commit: ver.07
* changed file : ```../files/gattc_multi_connect```
* config要改：
    * 同上
* 要改的全域
    * 同上
* 修改
    * 在斷線時：```gl_profile_tab[i].conn_id = -1```
    * scan時：以```conn_id == -1```來判斷連上之後的```conn_id```
        * 系統的``conn_id``為由小而大填上，以連上的id不會因為其他人斷線而改變
* 新增
    * ```gl_profile_tab[i].connectTo```來判斷連線上的裝置是哪個


-----
* [Reference : ESP-IDF Gatt Client Multi Connection Demo](https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/ble/gattc_multi_connect)





