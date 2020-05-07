#ifndef __UART_LINUX_H__
#define __UART_LINUX_H__

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "esp_loader.h"

#define UART_MIN(A,B) ((A) < (B) ? (A):(B))
#define READ_BIN_MIN(A,B) ((A) < (B) ? (A):(B))

#define SERIAL_MAX_BUFFER 1024 

#define BLOCK_IO 0
#define NONBLOCK_IO 1



int serial_open(char *serial_path);
int serial_close(int fd);
int serial_set_block_flag(int fd, int value);
int serial_get_in_queue_byte(int fd, int *byte_counts);
int set_serial_para(int fd, unsigned int baud, int databit, int stopbit, int parity, int flow);
int serial_set_baudrate(int fd, unsigned int baud);
ssize_t serial_read_n( int fd, const uint8_t  *read_buffer, ssize_t read_size, uint32_t timeout);
ssize_t serial_write_n(int fd, const uint8_t  *write_buffer, ssize_t write_size);
esp_loader_error_t loader_port_serial_write(int fd, const uint8_t *data, uint16_t size);
esp_loader_error_t loader_port_serial_read(int fd, const uint8_t *data, uint16_t size, uint32_t timeout);
esp_loader_error_t loader_spi_set_params_cmd(int fd, uint32_t fl_id, uint32_t total_size, uint32_t block_size, uint32_t sector_size, uint32_t page_size, uint32_t status_mask);

void loader_port_delay_ms(uint32_t ms);
void loader_port_enter_bootloader(int fd);
void loader_port_reset_target(int fd);
void loader_port_start_timer(uint32_t ms);
uint32_t loader_port_remaining_time(void);

#endif