#include "../include/linux_download.h"


uint8_t compute_checksum(const uint8_t *data, uint32_t size)
{
    uint8_t checksum = 0xEF;

    while (size--) {
        checksum ^= *data++;
    }

    return checksum;
}

FILE *get_file_size(char *path, ssize_t *image_size)
{
	//"./load_bin/project_template.bin"
	FILE *fp = fopen(path, "r");
	if(fp == NULL) {
        printf("fopen fail!");
        return (0);
    }
	fseek(fp, 0L, SEEK_END);
    *image_size = ftell(fp);
    rewind(fp);

    printf("Image size: %lu\n", *image_size);

    return fp;
}

void get_type_of_chip(int fd)
{
    uint32_t addr1 = 0;
    uint32_t addr2 = 0;
    uint32_t addr3 = 0;
    uint32_t addr4 = 0;
    uint32_t addr = 0;

    err = loader_read_reg_cmd(fd, 0x3ff0005c, &addr1);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
    }

    err = loader_read_reg_cmd(fd, 0x3ff00058, &addr2);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
    }

    err = loader_read_reg_cmd(fd, 0x3ff00054, &addr3);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
    }

    err = loader_read_reg_cmd(fd, 0x3ff00050, &addr4);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
    }

    addr = addr4;

    if ((addr & ((1 << 4) | 1 << 80)) != 0) {
        printf("is ESP8285!\n");
    } else {
        printf("is ESP8266!\n");
    }

}

void get_mac_addr_of_chip(int fd)
{
    address esp_otp_mac0 = 0x3ff00050;
    address esp_otp_mac1 = 0x3ff00054;
    address esp_otp_mac3 = 0x3ff0005c;

    address addr_output1 ;
    address addr_output2 ;
    address addr_output3 ;

    uint32_t mac0_value ;
    uint32_t mac1_value ;
    uint32_t mac3_value ;

    err = loader_read_reg_cmd(fd, esp_otp_mac0, &mac0_value);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
    }

    err = loader_read_reg_cmd(fd, esp_otp_mac1, &mac1_value);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
    }

    err = loader_read_reg_cmd(fd, esp_otp_mac3, &mac3_value);
     if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot read chip reg.\n");
    }

    if(mac3_value != 0) {
        addr_output1 = (mac3_value >> 16) & 0xff;
        addr_output2 = (mac3_value >> 8) & 0xff;
        addr_output3 = mac3_value & 0xff;
    } else if(((mac1_value >> 16) & 0xff) == 0){
        addr_output1 = 0x18;
        addr_output2 = 0xfe;
        addr_output3 = 0x34;
    } else if(((mac1_value >> 16) & 0xff) == 1){
        addr_output1 = 0xac;
        addr_output2 = 0xd0;
        addr_output3 = 0x74;
    } else {
        printf("mac addr unknow\n");
    }

    printf("mac_addr: %02x,%02x,%02x,%02x,%02x,%02x\n",\
            addr_output1,addr_output2,addr_output3, (mac1_value >> 8) & 0xff,\
            mac1_value & 0xff, (mac0_value >> 24) & 0xff);
}

void update_stub_code_to_target(int fd)
{
    printf("Uploading stub text.bin...\n");
    int32_t packet_number_mem = 0;
    ssize_t stub_text_len = 0;
    FILE *stub_text_bin =  get_file_size("./stub_code/text.bin", &stub_text_len);
    printf("stub_text_len:%lu\n",stub_text_len);
    //fclose(stub_text_bin);
    err = esp_loader_mem_start(fd, STUB_CODE_TEXT_ADDR_START, stub_text_len, sizeof(payload_mem));
    printf("err==%d\n",err);
    if(err != ESP_LOADER_SUCCESS) {
        printf("esp loader mem start fail!");
    }

    while(stub_text_len > 0) {
        ssize_t to_read = READ_BIN_MIN(stub_text_len, sizeof(payload_mem));
        ssize_t read = fread(payload_mem, 1, to_read, stub_text_bin);
        if(read != to_read) {
            printf("read the stub code text bin fail!\n");
        }

        err = esp_loader_mem_write(fd, payload_mem, to_read);
        if(err = ESP_LOADER_SUCCESS) {
            printf("the stub code text bin write to memory fail!\n");
        }
        
        printf("packet: %d  written: %lu B\n", packet_number_mem++, to_read);
        stub_text_len -= to_read;
    }

    fclose(stub_text_bin);

    printf("Uploading stub data.bin...\n");
    memcmp(payload_mem, 0x0, sizeof(payload_mem));

    packet_number_mem = 0;
    ssize_t stub_data_len = 0;
    FILE *stub_data_bin =  get_file_size("./stub_code/data.bin", &stub_data_len);
    printf("stub_data_len:%lu\n",stub_data_len);

    err = esp_loader_mem_start(fd, STUB_CODE_DATA_ADDR_START, stub_data_len, sizeof(payload_mem));
    if(err != ESP_LOADER_SUCCESS) {
        printf("esp loader mem start fail!");
    }

    while(stub_data_len > 0) {
        ssize_t to_read = READ_BIN_MIN(stub_data_len, sizeof(payload_mem));
        ssize_t read = fread(payload_mem, 1, to_read, stub_data_bin);

        if(read != to_read) {
            printf("read the stub code data bin fail!\n");
        }

        err = esp_loader_mem_write(fd, payload_mem, to_read);
        if(err = ESP_LOADER_SUCCESS) {
            printf("the stub code data bin write to memory fail!\n");
        }
        
        stub_data_len -= to_read;
    }

    fclose(stub_data_bin);

    err = esp_loader_mem_finish(fd, true, ENTRY);
    if(err != ESP_LOADER_SUCCESS) {
        printf("the stub code bin end!\n");
    }
    
    printf("stub code running?\n");
    err = esp_loader_mem_active_recv(fd);
    if(err != ESP_LOADER_SUCCESS) {
        printf("the sequence OHAI error!\n");
    }
    printf("stub code running!\n");
}


esp_loader_error_t linux_download_to_esp8266(int fd, int addr, char *path)
{
    //STEP4 flash interaction esp8266 stub loader
#if ENABLE_STUB_LOADER

    int32_t packet_number = 0;
    ssize_t load_bin_size = 0;
    FILE *image = get_file_size(path, &load_bin_size);

    err = esp_loader_flash_start(fd, addr, load_bin_size, sizeof(payload_flash));
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
        
        err = esp_loader_flash_write(fd, payload_flash, load_to_read);
        printf("stub loader err=%d\n",err);
        if (err != ESP_LOADER_SUCCESS) {
            printf("Packet could not be written.\n");
            return (0);
        }

        load_bin_size -= load_to_read;
    };

    printf("Flash write done.\n");
    err = esp_loader_flash_verify(fd);
    if (err != ESP_LOADER_SUCCESS) {
        printf("MD5 does not match. err: %d\n", err);
    } else {
        printf("Flash verified success!\n");
    }

    fclose(image);

#elif

    //STEP4 flash interaction esp8266 rom loader
    int32_t packet_number = 0;
    ssize_t load_bin_size = 0;
    FILE *image = get_file_size(path, &load_bin_size);

    err = esp_loader_flash_start(serial_fd, LOAD_APP_ADDRESS_START, load_bin_size, sizeof(payload_flash));
    if (err != ESP_LOADER_SUCCESS) {
        printf("Flash start operation failed.\n");
        return (0);
    } 
    while(load_bin_size > 0) {
        ssize_t load_to_read = READ_BIN_MIN(load_bin_size, sizeof(payload_flash));
        ssize_t read = fread(payload_flash, 1, load_to_read, image);
        if (read != load_to_read) {
            printf("Error occurred while reading file.\n");
            return (0);
        }

        err = esp_loader_flash_write(serial_fd, payload_flash, load_to_read);
        if (err != ESP_LOADER_SUCCESS) {
            printf("Packet could not be written.\n");
            return (0);
        }

        printf("packet: %d  written: %lu B\n", packet_number++, load_to_read);

        load_bin_size -= load_to_read;
        
    };

    // 8266 ROM NOT SUPPORT md5 VERIFY
    err = esp_loader_flash_verify(serial_fd);
    if (err != ESP_LOADER_SUCCESS) {
        printf("MD5 does not match. err: %d\n", err);
        
    }
    printf("Flash verified\n");

    fclose(image);
#endif

    return ESP_LOADER_SUCCESS;
}


void parsing_config_doc_download(int fd, char *config_doc_path)
{
    char count = 0;
    char addr_local = 0;
    FILE* fp;
    char store_para[100][100] = { 0 };
    char store_addr_local[100] = { 0 };

    fp = fopen(config_doc_path, "r");

    while (fscanf(fp, "%s", store_para[count]) != EOF)
    {
        if (strncmp(store_para[count],"0x",2) == 0) {

            // debug
            // printf("-----------%s\n", store_para[count]);
            // printf("-----------%d------------\n", count);

            store_addr_local[addr_local] = count;
            addr_local++;
        }
        count++;
    }

    for (int addr_local_times = 0; addr_local_times < addr_local; addr_local_times++) {

        int nValude = 0;
        //debug
        // printf("------addr----%s\n",store_para[store_addr_local[addr_local_times]]);
        // printf("------command----%s\n",store_para[store_addr_local[addr_local_times] + 1]);
        
        sscanf(store_para[store_addr_local[addr_local_times]], "%x", &nValude); 
        linux_download_to_esp8266(fd, nValude, store_para[store_addr_local[addr_local_times] + 1]);

        //printf("----%0x----\n",nValude);
        //  printf("------command----%s\n",store_para[store_addr_local[addr_local_times] + 1]);
    }
    fclose(fp);
}