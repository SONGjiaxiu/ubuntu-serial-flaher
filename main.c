
#include "./include/linux_download.h"

#define LOAD_APP_ADDRESS_START 0x10000
#define HIGHER_BAUD_RATE 921600

int main()
{
    esp_loader_error_t err;

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
    esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();
    err = esp_loader_connect(serial_fd, &connect_config);
    if (err != ESP_LOADER_SUCCESS) {
        printf("Cannot connect to target.\n");
        return (0);
    }
    printf("Connected to target\n");

    //STEP2: read some information from Target
    get_type_of_chip(serial_fd);
    get_mac_addr_of_chip(serial_fd);

    //STEP3 update stub code to Target
    update_stub_code_to_target(serial_fd);

    // change baudrate of target
    err = esp_loader_change_baudrate(serial_fd,HIGHER_BAUD_RATE);
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