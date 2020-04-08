# esptool.py 流程

- 预备知识
  - SLIP
  - Command
  - Response
  - Command Opcodes
- 流程
  - 8266 stub loader 为例说明

### SLIP(**Serial Line IP**)

串行线路网际协议，是串行线路上对 IP 数据包进行的简单封装形式。

- IP 数据包以特殊字符 0xc0 结束，有的数据包开始处也会传一个 0xc0 以防止数据报到来之前的线路噪声当做数据报内容。
- IP报文中某个字符为 0xc0，那么就连续传输两个字节 0xdb 和 0xdc 来取代。
- IP报文中某个字符为 0xdb，那么就连续传输两个字节 0xdb 和 0xdd 来取代。

### Command

# ![选区_249](/home/songjiaxiu/图片/选区_249.png)

- commad  为 host 端发出的 SLIP 包，所有的字段都为小端格式。

### Response

![1583909956958](/home/songjiaxiu/.config/Typora/typora-user-images/1583909956958.png)

### Command Opcodes

![1583911624886](/home/songjiaxiu/.config/Typora/typora-user-images/1583911624886.png)

### esptool 与 stub loader 的交互流程 

终端输入: `python esptool.py --trace --chip esp8266 --port /dev/ttyUSB1 --baud 115200 --before defa
ault_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --
-flash_size 2MB 0x0000 bootloader.bin 0x10000 power_save.bin 0x8000 partitions_singleapp.bin`

----------------------------------------------------------------------------------------------------------------------------------------------

1.connect...

[Command]:  c0 00 08 24 00 00000000 0707122055555 55555555555555555 5555555555555555 5555555555555555 5555555555 c0 

Note: 36 bytes: `0x07 0x07 0x12 0x20`, followed by 32 x `0x55`

[Response]:  c0 01 08 02 00 07071220 0000 c0

----------------------------------------------------------------------------------------------------------------------------------------------

2. 读取芯片类型信息

[Command]: c0 00 0a 04 00 00000000 [5c 00 f0 3f] c0

Note: `0a`: `READ_REG ` command, 通常用来读取芯片上的各种地址，以识别芯片子类型、版本等。`[5c 00 f0 3f]` 为 32 位地址

[Response]: c0 01 0a 02 00 ab622404 0000 c0

 Note:  `READ_REG `, Read data as 32-bit word in `value` field.



- 要读取的地址 {5c00f03f, 5800f03f, 5400f03f, 5000f03f} 依次发出，如下：

{c0000a0400000000005c00f03fc0, c0000a0400000000005800f03fc0, c0000a0400000000005400f03fc0, c0000a0400000000005000f03fc0}

依次收到的 Response, 如下：

{c0010a0200ab6224040000c0,  010a020000b000be0000,  010a0200553f00020000, 010a02003000da760000}

----以上读到信息为 Chip is ESP8285

- 要读取的地址 {5c00f03f, 5800f03f, 5400f03f, 5000f03f} 依次发出，如下：

{c0000a0400000000005c00f03fc0, c0000a0400000000005800f03fc0, c0000a0400000000005400f03fc0, c0000a0400000000005000f03fc0}

依次收到的 Response, 如下：

{c0010a0200ab6224040000c0,  010a020000b000be0000,  010a0200553f00020000, 010a02003000da760000}

----以上读到信息为 Features: WiFi, Embedded Flash



- 要读取的地址{5c00f03f, 5400f03f, 5000f03f} 依次发出，如下：

{c0000a0400000000005c00f03fc0, c0000a0400000000005400f03fc0, c0000a0400000000005000f03fc0}

依次收到的 Response, 如下：

{c0010a0200ab6224040000c0, 010a0200553f00020000, 010a02003000da760000}

----以上读到信息为 MAC: 24:62: ab:3f:55:76

-------------------------------------------------------------------------------------------------------------------------------

3. uploading stub

[Command]: c0 00 05 10 00 00000000 [601f0000] [02000000] [00180000] [00e01040] c0

Note:  `05 `: `MEM_BEGIN` Command,  `total size` 为 `1f60` 8032, `number of data packets` 为 2, `data size in one packet` 为 6KB, `memory offset` 为 `4010e000`

[Response]: c0 01 05 02 00 ab622404 0000 c0



[Command]: c0 00 07 10 18 [ed000000] [00180000] [00000000] 00000000 00000000 [data...] c0

Note: `07`: `MEM_DATA` Command, `Checksum` : `ed000000`, (To calculate checksum, start with seed value 0xEF and XOR each individual byte in the "data to write". The 8-bit result is stored in the checksum field of the packet header (as a little endian 32-bit value)). `data size`: `00180000`, `sequence number`: `00000000`.

[Response]: c0 01 07 02 00 ab622404 0000 c0



[Command]: c0 00 07 70 07 f5000000 60070000 01000000 00000000 00000000 [data] c0

[Response]: c0 01 07 02 00 ab622404 0000 c0



[Command]: c0 00 05 10 00 00000000 00030000 01000000 00180000 a4abff3f c0 
Note: 00030000 768 bytes

[Response]: c0 01 05 02 00 ab622404 0000 c0



[Command]: c0 00 07 10 03 9b000000 00030000 00000000 00000000 00000000 [data] c0

[Response]: c0 01 07 02 00 ab622404 0000 c0



[Command]: c0 00 06 08 00 00000000 [00000000] [04e01040] c0    

Note: `06`: `MEM_END` Command, Two 32-bit words: execute flag, entry point address

[Response]: c0 01 06 02 00 ab622404 0000 c0



此时，Stub running...

[Response]:  c0 4f 48 41 49 c0

Note: The software loader then sends a custom SLIP packet of the sequence OHAI (`0xC0 0x4F 0x48 0x41 0x49 0xC0`), indicating that it is now running.



4. flash 的交互

Configuring flash size...

[Command]: c0 00 0b 18 00 00000000 [00000000] [00002000] [00000100] [00100000] [00010000] [ffff0000] c0

Note: `0b`: `SPI_SET_PARAMS`Command, `id`: `00000000`, `total size in bytes`: `00002000` 2MB, `block size`: `00000100` 64KB, `sector size`: `00100000` 4KB,  `page size`: `00010000` 256, `status mask`: `ffff0000` .

[Response]: c0 01 0b 02 00 00000000 0000 c0 



Compressed 10816 bytes to 7174...

[Command]: c0 00 10 10 00 00000000 [402a0000] [01000000] [00400000] [00000000] c0

Note: `10`: `FLASH_DEFL_BEGIN` Command, `uncompressed size`: `402a0000` 10816 bytes, `number of data packets`: 

`01000000`, `data packet size`: `00400000` 16KB, `flash offset`: `00000000`

Note1: With stub loader the uncompressed size is exact byte count to be written, whereas on ROM bootloader it is rounded up to flash erase block size.

Note2: The block size chosen should be small enough to fit into RAM of the device. esptool.py uses 16KB which gives good performance when used with the stub loader.

[Response]: c0 01 10 02 00 00000000 0000 c0 



Writing at 0x00000000...



[Command]: c0 00 11 [16 1c] [5f000000] [061c0000] 00000000 0000000000 [data...] c0

 Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `5f000000`,  `data size`: `061c0000` 7174 bytes

To calculate checksum, start with seed value 0xEF and XOR each individual byte in the "data to write". The 8-bit result is stored in the checksum field of the packet header (as a little endian 32-bit value).

[Response]: c0 01 11 02 00 00000000 0000 c0



Wrote 10816 bytes (7174 compressed) at 0x00000000 in 0.7 seconds (effective 122.7 kbit/s)...



[Command]: c0 00 13 10 00 00000000 00000000 402a0000 00000000 00000000 c0

 Note: `13`:  `SPI_FLASH_MD5 `   Command, `address`: `00000000`,  `data size`: `402a0000` 10KB 

[Response]: c0 01 13 12 00 00000000 a4859ce4 e3fb9206 040d00c5 465f39b1 0000 c0

Note: Body contains 16 raw bytes of MD5 followed by 2 status bytes



Compressed 215216 bytes to 148017...



[Command]: c0 00 10 10 00 00000000 b0480300 0a000000 00400000 00000100 c0 

 Note: `10`: `FLASH_DEFL_BEGIN` Command, `uncompressed size`: `b0480300` 215216 bytes, `number of data packets`: 

`0a000000`, `data packet size`: `00400000` 16KB, `flash offset`: `00000100`

[Response]: c0 01 10 02 00 00000000 0000 c0



Writing at 0x00010000...

[Command]:  c0 00 11 10 40 bd000000 00400000 00000000 00000000 00000000 [data...] c0

 Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `bd000000`,  `data size`: `00400000` 16KB, `sequence`: `00000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



Writing at 0x00014000...

[Command]:  c0 00 11 10 40 72000000 00400000 01000000 00000000 00000000 [data...] c0

 Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `72000000`,  `data size`: `00400000` 16KB, `sequence`: `01000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



Writing at 0x00018000...

[Command]:c0 00 11 10 40 6a000000 00400000 02000000 00000000 00000000 [data...] c0

Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `6a000000`,  `data size`: `00400000` 16KB, `sequence`: `02000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



Writing at 0x0001c000...

[Command]: c0 00 11 10 40 d9000000 00400000 03000000 00000000 00000000 [data...] c0

Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `d9000000`,  `data size`: `00400000` 16KB, `sequence`: `03000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



Writing at 0x00020000... 

[Command]: c0 00 11 10 40 b0000000 00400000 04000000 00000000 00000000 [data...] c0

Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `b0000000`,  `data size`: `00400000` 16KB, `sequence`: `04000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



Writing at 0x00024000...

[Command]: c0 00 11 10 40 62000000 00400000 05000000 00000000 00000000  [data...] c0

Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `62000000`,  `data size`: `00400000` 16KB, `sequence`: `05000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



Writing at 0x00028000...

[Command]: c0 00 11 10 40 4a000000 00400000 06000000 00000000 00000000 [data...] c0

Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `4a000000`,  `data size`: `00400000` 16KB, `sequence`: `06000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



Writing at 0x0002c000...

[Command]: c0 00 11 10 40 c4000000 00400000 07000000 00000000 00000000 [data...] c0

Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `c4000000`,  `data size`: `00400000` 16KB, 

`sequence`: `07000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



Writing at 0x00030000...

[Command]: c0 00 11 10 40 c6000000 00400000 08000000 00000000 00000000 [data...] c0

Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `c6000000`,  `data size`: `00400000` 16KB, `sequence`: `08000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



Writing at 0x00034000... data len 577

[Command]: c0 00 11 41 02 [dbdc000000] 31020000 09000000 00000000 00000000 [data...]

Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `c0000000`,  `data size`: `31020000` 561 bytes, `sequence`: `09000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



计算：16KB * 9 + 561= 147456 + 561 = 148017



Wrote 215216 bytes (148017 compressed) at 0x00010000 in 14.8 seconds (effective 116.1 kbit/s)...



[Command]: c0 00 13 10 00 00000000 00000100 b0480300 00000000 00000000 c0

 Note: `13`:  `SPI_FLASH_MD5 `   Command, `address`: `00000100`,  `data size`: `b0480300` 215216  bytes

[Response]:c0  01 13 12 00 00000000 44b63543 9d06633d 02118292 12189a09 0000 c0

Hash of data verified.



Compressed 3072 bytes to 83...



[Command]: c0 00 10 10 00 00000000 000c0000 01000000 00400000 00800000 c0

Note: `10`: `FLASH_DEFL_BEGIN` Command, `uncompressed size`: `000c0000` 3072 bytes, `number of data packets`: `01000000`, `data packet size`: `00400000` 16KB, `flash offset`: `00800000`

[Response]: c0 01 10 02 00 00000000 0000 c0



Writing at 0x00008000... 

[Command]: c0 00 11 63 00 19000000 53000000 00000000 00000000 00000000 78da5b15dbdcc8 c430818181218181 21afac98011dac0a 606464f800640830 30146454c667e665 96a0ca83484620e6 67484b4c2ec92faa 44d1ff7f148c8251 300a46c1281805a3 60148c8251306800 00da98a1ac c0 

Note: `11`:  `FLASH_DEFL_DATA `   Command, checksum: `19000000`,  `data size`: `53000000` 83 bytes, `sequence`: `00000000`

[Response]: c0 01 11 02 00 00000000 0000 c0



Wrote 3072 bytes (83 compressed) at 0x00008000 in 0.0 seconds (effective 1423.7 kbit/s)...

[Command]:  c0 00 13 10 00 00000000 00800000 000c0000 00000000 00000000 c0

 Note: `13`:  `SPI_FLASH_MD5 `   Command, `address`: `00800000`,  `data size`: `000c0000` 3072  bytes

[Response]: 01 13 12 00 00000000 952ca75a 329b7738 b145db3b 007150b2 0000 

Hash of data verified.



Leaving...

[Command]: c0 00 02 10 00 00000000 [00000000] [00000000] [00400000] [00000000] c0

Note: `02`:  `FLASH_BEGIN `  Command, `size to erase ` : `00000000`, `number of data packets`: `00000000`,  `datadata size in one packetsize`: `00400000`,  `flash offset`: `00000000`

[Response]: c0 01 02 02 00 00000000 0000 c0 



FLASH_DEFL_END

[coamand] c0 00 12 04 00 00000000 01000000 c0

Note:`12`:  `FLASH_DEFL_END `  Command, One 32-bit word: `0` to reboot, `1` to "run user code". Not necessary to send this command if you wish to stay in the loader.

[Response] 01 12 02 00 00000000 0000

Hard resetting via RTS pin...