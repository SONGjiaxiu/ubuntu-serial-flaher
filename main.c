
#include "./include/linux_download.h"
#include "./include/esp_stub.h"


#define LOAD_APP_ADDRESS_START 0x10000
#define HIGHER_BAUD_RATE 921600

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

    //STEP2: read some information from Target
    // get_type_of_chip(serial_fd);
    // get_mac_addr_of_chip_esp8266(serial_fd);
    printf("detecting chip type...");
    uint32_t reg_value;
    err = esp_loader_read_register(serial_fd, 0x60000078, &reg_value);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot connect to target.\n");
    }

    //STEP3 update stub code to Target
    if (reg_value == 0x00062000) {
        printf("ESP8266\n");
        update_stub_code_to_target_esp8266(serial_fd);
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

    loader_port_delay_ms(21);

    // parsing download.config file and download bin file to Target
    parsing_config_doc_download(serial_fd, "./download.config");

    serial_close(serial_fd);
    return (0);
}