# 启动调试
gdb -q ./your_program

# 设置断点（例如在调用MAC_FindAddress前）
(gdb) b main.c:123

# 运行程序
(gdb) run
# 在断点处执行以下命令

## 调试MAC_FindAddress
```shell
set $pusNewVlanID = (unsigned short*)malloc(sizeof(unsigned short))
set $puiNewStatus = (unsigned int*)malloc(sizeof(unsigned int))
set $aifIndexList = (unsigned int*)malloc(128)
set $puiIfNum = (unsigned int*)malloc(sizeof(unsigned int))
set $puiTimeToLive = (unsigned int*)malloc(sizeof(unsigned int))
set $puiMacLocation = (unsigned int*)malloc(sizeof(unsigned int))
set $uiFlag = 0x01

"\354\326\212\342\235@" EC:D6:8A:E2:9D:40
"\200al1R\177" 8061-6c31-527f
"\200al\276\265\250" 8061-6cbe-b5a8

(gdb) call MAC_FindAddress(1, "\200al1R\177", 1, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)
//成功
(gdb) call MAC_FindAddress(1, "\200al1R\177", 2, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)
//成功
(gdb) call MAC_FindAddress(4094, "\200al1R\177", 2, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)
//1073807368
(gdb) call MAC_FindAddress(1, "\200al1R\178", 2, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)
//1073807368
(gdb) call MAC_FindAddress(4095, "\200al1R\177", 1, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)
//失败 产品不支持
//更换接口
(gdb) call MAC_FindAddress(600, "\200al1R\177", 1, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)
// 成功
call MAC_FindAddress(1, "\200al\276\265\250", 1, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)
// 成功
call MAC_FindAddress(1, "\200al\276\265\251", 1, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)

4094
call memset($pusNewVlanID, 0, sizeof(unsigned short))
p *$pusNewVlanID
call MAC_FindAddress(4094, "\200\344U\226\027 ", $uiFlag, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)
返回值 1073807368
ERROR_NOTSUPPORT
//
call MAC_FindAddress(4099,"\200\344U\226\027 ",  $uiFlag, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)
返回值 1073807368
ERROR_NOTSUPPORT

call MAC_FindAddress(600, "\200\344U\226\027 ", $uiFlag, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)
返回值 1073807368
ERROR_NOTSUPPORT

call MAC_FindAddress(600, "\200al\276\265\250", $uiFlag, $pusNewVlanID, $puiNewStatus, $aifIndexList, $puiIfNum, $puiTimeToLive, $puiMacLocation)

(gdb) p/x $ret
(gdb) p *$pusNewVlanID
(gdb) p *$puiNewStatus
(gdb) p *$aifIndexList@(*$puiIfNum)

# 释放内存
(gdb) call free($pusNewVlanID)
```

## 调试GetCurStartupCfgFile
```shell
set $pcCharCfgFile = (char*)malloc(512)
call COMSH_SaveTxtConfig("/tmp/1.cfg")
# 保存文件到/tmp/1.cfg，然后再读取文件，解析出DHCP的配置
set $pcVersion = (char*)malloc(513)
call memset($pcVersion, 0, 513)
call GetCfgVersion($pcCharCfgFile, $pcVersion)
call GetCfgVersion((char*)0, $pcVersion)
```

## 调试VLAN_GetVlanList
```shell
(gdb) set $vlanBitmap = (BITMAP_S*)malloc(512) 
(gdb) call memset($vlanBitmap , 0, 512)
(gdb) call VLAN_GetVlanList(3, $vlanBitmap) 
(gdb) p *$vlanBitmap
(gdb) call free($vlanBitmap) 
```

## 调试GetCfgVersion
```shell
set $pcNextStartupFile = (char*)malloc(513)
call memset($pcNextStartupFile, 0, 513)
call GetCfgVersion(513, $pcNextStartupFile)
```
## 调试ARP_GetFirstEntry
```shell
set $pstarpinfo  = (ARP_INFO_S*)malloc(sizeof(ARP_INFO_S))
call memset($pstarpinfo, 0, sizeof(ARP_INFO_S))
call ARP_GetFirstEntry($pstarpinfo)
p *$pstarpinfo
call ARP_GetNextEntry($pstarpinfo)
p *$pstarpinfo
```

