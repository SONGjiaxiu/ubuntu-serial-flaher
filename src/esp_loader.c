/* Copyright 2020 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../include/loader_config.h"
#include "../include/serial_comm_prv.h"
#include "../include/serial_comm.h"
#include "../include/serial.h"
#include "../include/esp_loader.h"
#include "../include/md5_hash.h"
#include <string.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#endif

static const uint32_t DEFAULT_TIMEOUT = 500;
static const uint32_t SPI_PIN_CONFIG_DEFAULT = 0;
static const uint32_t DEFAULT_FLASH_TIMEOUT = 3000;       // timeout for most flash operations
static const uint32_t ERASE_REGION_TIMEOUT_PER_MB = 3000; // timeout (per megabyte) for erasing a region
static const uint8_t  PADDING_PATTERN = 0xFF;


static uint32_t s_flash_write_size = 0;
#define MD5_ENABLED  1

#if MD5_ENABLED

static const uint32_t MD5_TIMEOUT_PER_MB = 800;
static struct MD5Context s_md5_context;
static uint32_t s_start_address;
static uint32_t s_image_size;

static inline void init_md5(uint32_t address, uint32_t size)
{
    s_start_address = address;
    s_image_size = size;
    MD5Init(&s_md5_context);
}

static inline void md5_update(const uint8_t *data, uint32_t size)
{
    MD5Update(&s_md5_context, data, size);
}

static inline void md5_final(uint8_t digets[16])
{
    MD5Final(digets, &s_md5_context);
}

#else

static inline void init_md5(uint32_t address, uint32_t size) { }
static inline void md5_update(const uint8_t *data, uint32_t size) { }
static inline void md5_final(uint8_t digets[16]) { }

#endif


static uint32_t timeout_per_mb(uint32_t size_bytes, uint32_t time_per_mb)
{
    uint32_t timeout = ERASE_REGION_TIMEOUT_PER_MB * (size_bytes / 1e6);
    return MAX(timeout, DEFAULT_FLASH_TIMEOUT);
}


esp_loader_error_t esp_loader_connect(int fd, esp_loader_connect_args_t *connect_args)
{
    esp_loader_error_t err;
    int32_t trials = connect_args->trials;
    uint32_t reg = 0;

    // loader_port_enter_bootloader(fd);
int i=0;
    do {
        printf("syn_connect_times:%d\n",i++);
        loader_port_start_timer(connect_args->sync_timeout);
        err = loader_sync_cmd(fd);
printf("err: %d\n",err);
        if (err == ESP_LOADER_ERROR_TIMEOUT) {
            if (--trials == 0) {
                return ESP_LOADER_ERROR_TIMEOUT;
            }
            loader_port_delay_ms(100);
        } else if (err != ESP_LOADER_SUCCESS) {
            return err;
        }
    } while (err != ESP_LOADER_SUCCESS);
printf("ESP_LOADER_SUCCESS: %d\n",ESP_LOADER_SUCCESS);
    loader_port_start_timer(DEFAULT_TIMEOUT);
    // return loader_spi_attach_cmd(fd, SPI_PIN_CONFIG_DEFAULT);
    return err;
}


// rom
// esp_loader_error_t esp_loader_flash_start(int fd, uint32_t offset, uint32_t image_size, uint32_t block_size)
// {
//     uint32_t blocks_to_write = (image_size + block_size - 1) / block_size;
//     uint32_t erase_size = block_size * blocks_to_write;
//     s_flash_write_size = block_size;

//     init_md5(offset, image_size);

//     loader_port_start_timer(timeout_per_mb(erase_size, ERASE_REGION_TIMEOUT_PER_MB));

//     return loader_flash_begin_cmd(fd, offset, erase_size, block_size, blocks_to_write);
// }

esp_loader_error_t esp_loader_flash_start(int fd, uint32_t offset, uint32_t image_size, uint32_t block_size)
{
    uint32_t blocks_to_write = (image_size + block_size - 1) / block_size;
    uint32_t erase_size = block_size * blocks_to_write;
    s_flash_write_size = block_size;

    init_md5(offset, image_size);
    loader_port_start_timer(timeout_per_mb(erase_size, ERASE_REGION_TIMEOUT_PER_MB));
    return loader_flash_begin_cmd(fd, offset, erase_size, block_size, blocks_to_write);
}


esp_loader_error_t esp_loader_flash_write(int fd, void *payload, uint32_t size)
{
    uint32_t padding_bytes = s_flash_write_size - size;
    uint8_t *data = (uint8_t *)payload;
    uint32_t padding_index = size;

    while (padding_bytes--) {
        data[padding_index++] = PADDING_PATTERN;
    }

    md5_update(payload, (size + 3) & ~3);

    loader_port_start_timer(DEFAULT_TIMEOUT);

    return loader_flash_data_cmd(fd, data, s_flash_write_size);
}


esp_loader_error_t esp_loader_flash_finish(int fd, bool reboot)
{
    loader_port_start_timer(DEFAULT_TIMEOUT);

    return loader_flash_end_cmd(fd, !reboot);
}

esp_loader_error_t esp_loader_mem_start(int fd, uint32_t mem_offset, uint32_t image_size, uint32_t block_size)
{
    uint32_t nums_of_block = (image_size + block_size -1) / block_size;
    s_flash_write_size = block_size;

    // printf("nums_of_block:%d\n",nums_of_block);

    init_md5(mem_offset, image_size);
    loader_port_start_timer(DEFAULT_TIMEOUT);

    return loader_mem_begin_cmd(fd, mem_offset, image_size, block_size, nums_of_block);
}


esp_loader_error_t esp_loader_mem_write(int fd, void *playload, uint32_t size)
{
    uint32_t padding_byte_numbers = s_flash_write_size - size;
    uint8_t *data = (uint8_t *)playload;
    uint32_t padding_index = size;

#if 1
    while(padding_byte_numbers--) {
        data[padding_index++] = PADDING_PATTERN;
    }
#endif
    md5_update(data, ((size + 3) & ~3));
    return loader_mem_data_cmd(fd, data, s_flash_write_size);
}


esp_loader_error_t esp_loader_mem_finish(int fd, bool reboot, uint32_t entry)
{
    return loader_mem_end_cmd(fd, !reboot, entry);
}

esp_loader_error_t  esp_loader_mem_active_recv(int fd)
{
    return loader_mem_active_recv(fd);
}

esp_loader_error_t esp_loader_memory_start(int fd, uint32_t offset, uint32_t data_size, uint32_t block_size)
{
    uint32_t blocks_to_write = (data_size + block_size - 1) / block_size;

    loader_port_start_timer(DEFAULT_TIMEOUT);

    return loader_mem_begin_cmd(fd, offset, data_size, block_size, blocks_to_write);
}


esp_loader_error_t esp_loader_memory_write(int fd, void *payload, uint32_t size)
{
    loader_port_start_timer(DEFAULT_TIMEOUT);

    return loader_mem_data_cmd(fd, payload, size);
}


esp_loader_error_t esp_loader_memory_finish(int fd, bool execute, uint32_t entry_point_address)
{
    loader_port_start_timer(DEFAULT_TIMEOUT);

    return loader_mem_end_cmd(fd, !execute, entry_point_address);
}


esp_loader_error_t esp_loader_read_register(int fd, uint32_t address, uint32_t *reg_value)
{
    loader_port_start_timer(DEFAULT_TIMEOUT);

    return loader_read_reg_cmd(fd, address, reg_value);
}


esp_loader_error_t esp_loader_write_register(int fd, uint32_t address, uint32_t reg_value)
{
    loader_port_start_timer(DEFAULT_TIMEOUT);

    return loader_write_reg_cmd(fd, address, reg_value, 0xFFFFFFFF, 0);
}

esp_loader_error_t esp_loader_change_baudrate(int fd, uint32_t baudrate)
{
    loader_port_start_timer(DEFAULT_TIMEOUT);

    return loader_change_baudrate_cmd(fd, baudrate);
}

#if MD5_ENABLED

static void hexify(const uint8_t raw_md5[16], uint8_t hex_md5_out[32])
{
    uint8_t high_nibble, low_nibble;

    static const uint8_t dec_to_hex[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    for (int i = 0; i < 16; i++) {
        high_nibble = (raw_md5[i] / 16);
        low_nibble = (raw_md5[i] - (high_nibble * 16));
        *hex_md5_out++ = dec_to_hex[high_nibble];
        *hex_md5_out++ = dec_to_hex[low_nibble];
    }
}


esp_loader_error_t esp_loader_flash_verify(int fd)
{
    uint8_t raw_md5[16];
    uint8_t hex_md5[MD5_SIZE + 1];
    uint8_t received_md5[MD5_SIZE + 1];

    md5_final(raw_md5);
    hexify(raw_md5, hex_md5);

    loader_port_start_timer(timeout_per_mb(s_image_size, MD5_TIMEOUT_PER_MB));

    RETURN_ON_ERROR( loader_md5_cmd(fd, s_start_address, s_image_size, received_md5) );

    // // debug
    // {
    //     printf("raw_md5:=begin\n");
    //     for(int i = 0; i < MD5_SIZE; i++)
    //     {
    //         printf("=%02x",raw_md5[i]);
    //     }
    //     printf("\nraw_md5\n");
    // }

    // // debug
    // {
    //     printf("received_md5:=begin\n");
    //     for(int i = 0; i < MD5_SIZE; i++)
    //     {
    //         printf("=%02x",received_md5[i]);
    //     }
    //     printf("\nreceived_md5\n");
    // }

    // printf("======memcmp=====%d\n",memcmp(raw_md5, received_md5, MD5_SIZE));

    bool md5_match = memcmp(raw_md5, received_md5, 16) == 0;

    // printf("======md5_match=====%d\n",md5_match);

    // // debug
    // {
    //     printf("md5:=hex_md5\n");
    //     for(int i = 0; i < MD5_SIZE; i++)
    //     {
    //         printf("=%02x",hex_md5[i]);
    //     }
    //     printf("\nmd5:hex_md5\n");
    // }

    // // debug
    // {
    //     printf("raw_md5:=begin\n");
    //     for(int i = 0; i < MD5_SIZE; i++)
    //     {
    //         printf("=%02x",raw_md5[i]);
    //     }
    //     printf("\nraw_md5\n");
    // }

    // // debug
    // {
    //     printf("received_md5:=begin\n");
    //     for(int i = 0; i < MD5_SIZE; i++)
    //     {
    //         printf("=%02x",received_md5[i]);
    //     }
    //     printf("\nreceived_md5\n");
    // }
    
    if(!md5_match) {
        raw_md5[MD5_SIZE] = '\n';
        received_md5[MD5_SIZE] = '\n';

        printf("Error: MD5 checksum does not match:\n");
        printf("Expected:\n");
        printf("%s\n",(char*)received_md5);
        printf("Actual:\n");
        printf("%s\n",(char*)raw_md5);

        return ESP_LOADER_ERROR_INVALID_MD5;
    }

    return ESP_LOADER_SUCCESS;
}

#endif

void esp_loader_reset_target(int fd)
{
    loader_port_reset_target(fd);
}
