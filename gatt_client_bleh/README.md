Gatt Client Multi Connection with 
========================

## 初始流程
1. ```#include "ble_clients.h"```
2. ```initBle()```
3. ```setConnectInfo(conn_info, dev_num)``` : 要掃描及連接的設備```conn_info```格式參照```ConnectInfoT```

## function

* ```uint8_t getBleStatus(uint8_t dev_id)``` : 
    * 0 : 什麼都沒有
    * 1 : 掃描到
    * 2 : 連接(open)了

### 其他
有點趕，之後再補，先讀讀範例程式吧哈哈

-----
[![hackmd-github-sync-badge](https://hackmd.io/WocMmpOMQeCTHXoc287bDQ/badge)](https://hackmd.io/WocMmpOMQeCTHXoc287bDQ)  

