
#include "./include/linux_download.h"
#include "./include/esp_stub.h"


#define LOAD_APP_ADDRESS_START 0x10000
#define HIGHER_BAUD_RATE 921600
#define ESP8266_S2 1

uint8_t esp32_stub_code_using_flag = 0;

int main(void)
{
    esp_loader_error_t err;
    esp_stub_config_t stub_config;

    int serial_fd = -1;
    if ((serial_fd = serial_open("/dev/ttyUSB0")) == -1) {
        printf("open serial fail!\n");
        return (0);
    }else {
        printf("open serial success!\n");
    }

    if (set_serial_para(serial_fd, 115200, 8, 1, 0, 0) == -1) {
        printf("set serial fail!\n");
        serial_close(serial_fd);
    }

    //STEP1: connect to Target 
    printf("Connecting...\n");
    esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();
    err = esp_loader_connect(serial_fd, &connect_config);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot connect to target.\n");
    }
    printf("Connected to target\n");
    sleep(1);

#ifdef ESP8266_S2
//CONFIG SPI REG
    printf("\nconfig spi reg...\n");
    err = loader_write_reg_cmd(serial_fd, 0x60000804, 0x20, 0x130, 0x0a);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = loader_write_reg_cmd(serial_fd, 0x60000808, 0x20, 0x130, 0x0a);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = loader_write_reg_cmd(serial_fd, 0x6000080C, 0x20, 0x130, 0x0a);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = loader_write_reg_cmd(serial_fd, 0x60000810, 0x20, 0x130, 0x0a);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = loader_write_reg_cmd(serial_fd, 0x3FF00028, 0x02, 0x02, 0x0a);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = loader_write_reg_cmd(serial_fd, 0x60000800, 0x0, 0x300, 0x0a);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = esp_loader_flash_start(serial_fd, 0x0000, 0x0000, 0x400);
    if (err != ESP_LOADER_SUCCESS) {
        printf("flash begin cmd err:%d\n",__LINE__);
    }

    stub_config = esp8266_s2_stub_config;
    stub_config.stub_enabled = true;

    if (stub_config.stub_enabled) {
            uint16_t memory_block_size = 0x1800;
            printf("\nUploading stub...\n");

            // stub has 2 sections, text and data section.
            for (int i = 0; i < 2; i++) {
                uint32_t load_size = stub_config.section_config[i].stub_section_length;

                err = esp_loader_memory_start(serial_fd, stub_config.section_config[i].stub_section_offset, load_size, memory_block_size);
                if (err != ESP_LOADER_SUCCESS) {
                    printf("\nMemory start operation failed.\n");
                }

                for (int j = 0; j < (stub_config.section_config[i].stub_section_length + memory_block_size - 1) / memory_block_size; j++) {
                    size_t to_read = READ_BIN_MIN(load_size, memory_block_size);

                    err = esp_loader_memory_write(serial_fd, (void *)(stub_config.section_config[i].stub_section_buffer + j * memory_block_size), to_read);
                    if (err != ESP_LOADER_SUCCESS) {
                        printf("\nMemory could not be written.\n");
                    }

                    load_size -= to_read;
                }
            }

            printf("\nRunning stub...\n");

            err = esp_loader_memory_finish(serial_fd, true, stub_config.stub_entry_point);
            if(err != ESP_LOADER_SUCCESS) {
                printf("\nthe stub code bin end!\n");
            }
            
            printf("\nstub code running?\n");
            err = esp_loader_mem_active_recv(serial_fd);
            if(err != ESP_LOADER_SUCCESS) {
                printf("\nthe sequence OHAI error!\n");
            }
            printf("\nstub code running!\n");
        }

#else

    //STEP2: read some information from Target
    // get_type_of_chip(serial_fd);
    // get_mac_addr_of_chip_esp8266(serial_fd);
    printf("\ndetecting chip type...");
    uint32_t reg_value;
    err = esp_loader_read_register(serial_fd, 0x60000078, &reg_value);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot connect to target.\n");
    }

    //STEP3 update stub code to Target
    if (reg_value == 0x00062000) {
        printf("ESP8266\n");
        update_stub_code_to_target_esp8266("./stub_code/text.bin", serial_fd);
        // stub_config = esp8266_stub_config;
    } else if (reg_value == 0x15122500) {
        printf("ESP32\n");
        // esp32_stub_code_using_flag = 1;
        stub_config = esp32_stub_config;
        stub_config.stub_enabled = true;

        if (stub_config.stub_enabled) {
            uint16_t memory_block_size = 0x1000;
            printf("\nUploading stub...\n");

            // stub has 2 sections, text and data section.
            for (int i = 0; i < 2; i++) {
                uint32_t load_size = stub_config.section_config[i].stub_section_length;

                err = esp_loader_memory_start(serial_fd, stub_config.section_config[i].stub_section_offset, load_size, memory_block_size);
                if (err != ESP_LOADER_SUCCESS) {
                    printf("\nMemory start operation failed.\n");
                }

                for (int j = 0; j < (stub_config.section_config[i].stub_section_length + memory_block_size - 1) / memory_block_size; j++) {
                    size_t to_read = READ_BIN_MIN(load_size, memory_block_size);

                    err = esp_loader_memory_write(serial_fd, (void *)(stub_config.section_config[i].stub_section_buffer + j * memory_block_size), to_read);
                    if (err != ESP_LOADER_SUCCESS) {
                        printf("\nMemory could not be written.\n");
                    }

                    load_size -= to_read;
                }
            }

            printf("\nRunning stub...\n");

            err = esp_loader_memory_finish(serial_fd, true, ENTRY_ESP32);
            if(err != ESP_LOADER_SUCCESS) {
                printf("\nthe stub code bin end!\n");
            }
            
            printf("\nstub code running?\n");
            err = esp_loader_mem_active_recv(serial_fd);
            if(err != ESP_LOADER_SUCCESS) {
                printf("\nthe sequence OHAI error!\n");
            }
            printf("\nstub code running!\n");
        }
        esp32_stub_code_using_flag = 1;
        esp32_stub_code_using_flag = 1;
    }

    // change baudrate of target
    err = esp_loader_change_baudrate(serial_fd, HIGHER_BAUD_RATE);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Unable to change baud rate on target.\n");
        return (0);
    }

    err = serial_set_baudrate(serial_fd, HIGHER_BAUD_RATE);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Unable to change baud rate.\n");
        return (0);
    }
#endif

// change baudrate of target
    err = esp_loader_change_baudrate(serial_fd, HIGHER_BAUD_RATE);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Unable to change baud rate on target.\n");
        return (0);
    }

    err = serial_set_baudrate(serial_fd, HIGHER_BAUD_RATE);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Unable to change baud rate.\n");
        return (0);
    }

    loader_port_delay_ms(21);

#if 0
//CONFIG SPI REG
    printf("\nconfig spi reg...\n");
    err = loader_write_reg_cmd(serial_fd, 0x60000220, 0x17, 0xffffffff, 0x0);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = loader_write_reg_cmd(serial_fd, 0x6000021c, 0x90000000, 0xffffffff, 0x0);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = loader_write_reg_cmd(serial_fd, 0x60000224, 0x7000009f, 0xffffffff, 0x0);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = loader_write_reg_cmd(serial_fd, 0x60000240, 0x0, 0xffffffff, 0x0);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = loader_write_reg_cmd(serial_fd, 0x60000200, 0x00040000, 0xffffffff, 0x0);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

//===
    err = loader_write_reg_cmd(serial_fd, 0x6000021c, 0x80000044, 0xffffffff, 0x0);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }

    err = loader_write_reg_cmd(serial_fd, 0x60000224, 0x70000000, 0xffffffff, 0x0);
    if (err != ESP_LOADER_SUCCESS) {
        printf("write reg cmd err:%d\n",__LINE__);
    }


    // err = loader_write_reg_cmd(serial_fd, 0x60000800, 0x0, 0x300, 0x0a);
    // if (err != ESP_LOADER_SUCCESS) {
    //     printf("write reg cmd err:%d\n",__LINE__);
    // }

// c0 00 09 10  00 00 00 00 00 20 02 00  60 00 17 00 00 ff ff ff  ff 00 00 00  00 c0 
// 01 09 00 00  00 00 00 00  00 00 c0  

// c0 00 09 10  00 00 00 00  00 1c 02 00  60 00 00 00 90 ff ff ff  ff 00 00 00 00 c0
// 01 09 00 00  00 00 00 00 00 00 c0

// c0 00 09 10  00 00 00 00 00 24 02 00  60 9f 00 00 70 ff ff ff  ff 00 00 00 00 c0
// 01 09 00 00  00 00 00 00 00 00 c0 

// c0 00 09 10  00 00 00 00 00 40 02 00  60 00 00 00  00 ff ff ff  ff 00 00 00 00 c0    
// 01 09 00 00  00 00 00 00 00 00 c0 

// c0 00 09 10  00 00 00 00 00 00 02 00  60 00 00 04 00 ff ff ff  ff 00 00 00 00 c0
// 01 09 00 00  00 00 00 00 00 00 c0

// c0 00 0a 04  00 00 00 00 00 00 02 00  60 c0
// 01 0a 00 00  9f 20 00 00 00 00 c0 

// c0 00 0a 04  00 00 00 00 00 40 02 00  60 c0 
// 01 0a 00 00  c8 40 15 00 00 00 c0  

// c0 00 09 10  00 00 00 00 00 1c 02 00  60 44 00 00 80 ff ff ff  ff 00 00 00 00 c0   
// 01 09 00 00  00 00 00 00 00 00 c0 

//  c0 00 09 10  00 00 00 00 00 24 02 00  60 00 00 00 70 ff ff ff  ff 00 00 00  00 c0
//  01 09 00 00  00 00 00 00  00 00 c0

// c0 00 0b 18  00 00 00 00 00 00 00 00  00 00 00 00 01 00 00 01  00 00 10 00 00 00 01 00  00 ff ff 00 00 c0                     

//  01 0b 00 00  00 00 00 00  00 00 c0

#endif

    // err = loader_spi_set_params_cmd(serial_id,  );
    // if (err != ESP_LOADER_SUCCESS) {
    //     printf("write reg cmd err:%d\n",__LINE__);
    // }

    // fl_id = 0
    //     total_size = size
    //     block_size = 64 * 1024
    //     sector_size = 4 * 1024
    //     page_size = 256
    //     status_mask = 0xffff
    //     self.check_command("set SPI params", ESP32ROM.ESP_SPI_SET_PARAMS,
    //                        struct.pack('<IIIIII', fl_id, total_size, block_size, sector_size, page_size, status_mask))
//ESP8266_TEST
#if 1
    int32_t packet_number = 0;
    ssize_t load_bin_size = 0;
    FILE *image = get_file_size("./load_bin/esp8266/project_template.bin", &load_bin_size);

    err = esp_loader_flash_start(serial_fd, 0x8000, load_bin_size, sizeof(payload_flash));
    if (err != ESP_LOADER_SUCCESS) {
        printf("Flash start operation failed.\n");
        return (0);
    }
    int send_times = 0;
    while(load_bin_size > 0) {
        memset(payload_flash,0x0,sizeof(payload_flash));
        ssize_t load_to_read = READ_BIN_MIN(load_bin_size, sizeof(payload_flash));
        ssize_t read = fread(payload_flash, 1, load_to_read, image);
    
        if (read != load_to_read) {
            printf("Error occurred while reading file.\n");
            return (0);
        }
        send_times++;
        
        err = esp_loader_flash_write(serial_fd, payload_flash, load_to_read);
        printf("stub loader err=%d\n",err);
        if (err != ESP_LOADER_SUCCESS) {
            printf("Packet could not be written.\n");
            return (0);
        }

        load_bin_size -= load_to_read;
    };

    printf("Flash write done.\n");
    err = esp_loader_flash_verify(serial_fd);
    if (err != ESP_LOADER_SUCCESS) {
        printf("MD5 does not match. err: %d\n", err);
    } else {
        printf("Flash verified success!\n");
    }

    fclose(image);
#endif

    // parsing download.config file and download bin file to Target
    // parsing_config_doc_download(serial_fd, "./download.config");

    serial_close(serial_fd);
    return (0);
}