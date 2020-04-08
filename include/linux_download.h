#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "serial.h"
#include "serial_comm.h"
#include "esp_loader.h"

#define ENABLE_STUB_LOADER 1

#ifdef ENABLE_STUB_LOADER
#define SERIAL_MAX_BLOCK  16384
#elif
#define SERIAL_MAX_BLOCK  4092
#endif

#define MEM_MAX_BLOCK  1024
#define FILE_MAX_BUFFER 1024

#define STUB_CODE_TEXT_ADDR_START 0X4010E000
#define STUB_CODE_DATA_ADDR_START 0x3FFFABA4
#define ENTRY 0X4010E004


uint8_t payload_flash[SERIAL_MAX_BLOCK];
uint8_t payload_mem[MEM_MAX_BLOCK];

typedef uint32_t address;
esp_loader_error_t err;

FILE *get_file_size(char *path, ssize_t *image_size);
uint8_t compute_checksum(const uint8_t *data, uint32_t size);

void get_type_of_chip(int fd);
void get_mac_addr_of_chip(int fd);
void update_stub_code_to_target(int fd);
esp_loader_error_t linux_download_to_esp8266(int fd, int addr, char *path);
void parsing_config_doc_download(int fd, char *config_doc_path);
