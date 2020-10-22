## FORA
```
UUID Base: 1212-efde-1523-785feabcd123
Service UUID: 0x1523
Characteristic: 0x1524 (write/notify)
```
所有fora的產品都引用```fora.h```，並記得初始化```foraInit()```，在```fora.c```內會根據config將指到對應的產品的實作。  
* foraThermometer : 額溫槍
  * 在連線上的時秒內要先發兩個相同的指令（ex:0x24），以保持連線不斷掉
* foraBP ： 血壓計
* foraBPG ： 二合一血壓計（可量測血糖）
