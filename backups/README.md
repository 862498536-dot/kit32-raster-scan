# 项目完整备份

由所有者明确要求上传，保存在当前私有仓库中。

- original/COM8-original-20261103.bin：借用车被覆盖前读取的 4 MB 整片 Flash，用于恢复借用车原程序。这不是当前扫描项目。
- releases/2026-11-03-RasterScan-v1.zip：当前蛇形扫描项目完整包，含当时的真实热点配置、可刷入固件、源码、网页和说明。
- releases/2026-11-03-200141-Before-Major-Changes.zip：大改前的一秒起步基线。
- releases/ 下其余文件：之前的热点、持续前进、PWM35 和一秒停车各版本。
- ../firmware/prebuilt/：当前扫描项目的独立编译产物和固件校验表。

给自己的同型号车刷扫描程序，应使用 RasterScan-v1 或 firmware/prebuilt；恢复借用车原始程序才使用 original/ 中的整片镜像。此次仅上传备份，没有再次刷写小车。

当前扫描固件上传示例（COM8 需替换为目标车实际端口）：

```powershell
arduino-cli upload --port COM8 --fqbn m5stack:esp32:m5stack_stamp_c3:CDCOnBoot=default --input-dir firmware/prebuilt
```
