#include "../include/serial.h"

static int64_t s_time_end;

int serial_open(char *serial_path)
{
    int fd = -1;
    fd = open(serial_path, O_RDWR); // | O_NOCTTY | O_NONBLOCK 
    if (fd == -1) {
        perror( "open_port: Unable to open /dev/ttyUSB0" );
    }

    if (serial_set_block_flag(fd, BLOCK_IO) == -1) {
        printf( "IO set error\n" );
    }

    return(fd);
}


int serial_close(int fd)
{
    if ( fd < 0 ) {
        return(-1);
    }

    if (close( fd ) == -1) {
        return(-1);
    }

    printf( "close uart\n\n" );

    return(0);
}


int serial_set_block_flag(int fd, int value)
{
    int oldflags;
    if (fd == -1) {
        return(-1);
    }

    oldflags = fcntl(fd, F_GETFL, 0);

    if (oldflags == -1) {
        printf( "get IO flags error\n" );
        return(-1);
    }

    if (value == BLOCK_IO) {
        oldflags &= ~O_NONBLOCK;     //BLOCK
    }else {
        oldflags |= O_NONBLOCK;      //NONBLOCK
    }

    return(fcntl( fd, F_GETFL, oldflags ) );
}


int serial_get_in_queue_byte(int fd, int *byte_counts)
{
    int bytes = 0;
    if (fd == -1) {
        return(-1);
    }

    if (ioctl( fd, FIONREAD, &bytes ) != -1)
    {
        *byte_counts = bytes;
        return(0);
    }

    return(-1);
}


/*
*
*  set_serial_para()
*
*  baud: 921600、115200
*
*  databit: 5、6、7、8
*
*  stopbit: 1、2
*
*  parity:0:null 1:odd 2:even
*
*  flow：0：null 1：softwareflow 2:hardwareflow 
*
*  return：fail:-1, sccess:0
*
*/

int set_serial_para(int fd, unsigned int baud, int databit, int stopbit, int parity, int flow)
{
    struct termios options;

    bzero(&options, sizeof(options));

    if (fd == -1) {
        return(-1);
    }

	if (tcgetattr( fd, &options ) == -1) {
        return(-1);
    }

    switch(baud) /* get baudrate */
    {
    case 921600:
        options.c_cflag = B921600;
        break;

    case 115200:
        options.c_cflag = B115200;
        break;

    default:
        options.c_cflag = B115200;
        break;
    }

    switch(databit) 
    {
    case 7:
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS7;
        break;

    case 8:
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        break;

    default:
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        break;
    }

    switch(parity)                                      
	{
    case 0:
        options.c_cflag &= ~PARENB;                  
        options.c_iflag &= ~(INPCK | ISTRIP);          
        options.c_iflag |= IGNPAR;                      
        break;

	case 1:
        options.c_cflag |= (PARENB | PARODD);        
        options.c_iflag |= (INPCK | ISTRIP);   
        options.c_iflag &= ~IGNPAR;                
        break;

    case 2:
        options.c_cflag |= PARENB;                 
        options.c_cflag &= ~PARODD;                 
        options.c_iflag |= (INPCK | ISTRIP);            
        options.c_iflag &= ~IGNPAR;                   
        break;

	default:
        options.c_cflag &= ~PARENB;               
        options.c_iflag &= ~(INPCK | ISTRIP);          
        options.c_iflag |= IGNPAR;                  
        break;
    }

    switch ( stopbit )                                      
    {
    case 1:
        options.c_cflag &= ~CSTOPB;         
        break;

    case 2:
        options.c_cflag |= CSTOPB;               
        break;

    default:
        options.c_cflag &= ~CSTOPB;                     
        break;
    }

    switch(flow)                       
    {
    case 0:
        options.c_cflag &= ~CRTSCTS;                  
        options.c_iflag &= ~(IXON | IXOFF | IXANY);  
        options.c_cflag |= CLOCAL;                      

    case 1:
        options.c_cflag &= ~CRTSCTS;                   
        options.c_cflag &= ~CLOCAL;                    
        options.c_iflag |= (IXON | IXOFF | IXANY);   
        break;

    case 2:
        options.c_cflag &= ~CLOCAL;                   
        options.c_iflag &= ~(IXON | IXOFF | IXANY);     
        options.c_cflag |= CRTSCTS;                    
        break;

    default:
        options.c_cflag &= ~CRTSCTS;             
        options.c_iflag &= ~(IXON | IXOFF | IXANY);     
        options.c_cflag |= CLOCAL;                   
        break;
    }

    options.c_cflag |= CREAD;                              
    options.c_iflag |= IGNBRK;                            
    options.c_oflag = 0;                                  
    options.c_lflag = 0;                              

    options.c_iflag &= ~(ICRNL | IXON);
    // options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); 
    // options.c_lflag &= ~(ICANON | ECHO | ECHOE ); 
    // options.c_oflag &= ~OPOST; 
	
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &options) == -1){
        return(-1);
    }

    tcflush( fd, TCOFLUSH );
    tcflush( fd, TCIFLUSH );
    return(0);
}


/*
*
*  int serial_set_baudrate(int fd, unsigned int baud)
*  baud:115200,921600
*/

int serial_set_baudrate(int fd, unsigned int baud)
{
    struct termios options;
    struct termios old_options;
    unsigned int baudrate = B19200;
    bzero(&options, sizeof(options));

    if (fd == -1) {
        return(-1);
    }

	if (tcgetattr( fd, &old_options ) == -1) {
        return(-1);
    }

	if (tcgetattr( fd, &options ) == -1) {
        return(-1);
    }

    switch(baud)
    {
    case 921600:
        baudrate = B921600;
        break;
    case 115200:
        baudrate = B115200;
        break;

    default:
        baudrate = B19200;
        break;
    }


    if (cfsetispeed(&options, baudrate) == -1) {
        return(-1);
    }

    if (cfsetospeed(&options, baudrate) == -1) {
        tcsetattr( fd, TCSANOW, &old_options );
        return(-1);
    }

    while ( tcdrain( fd ) == -1 );        

    tcflush( fd, TCOFLUSH );
    tcflush( fd, TCIFLUSH );

    options.c_cflag |= CREAD;                              
    options.c_iflag |= IGNBRK;                           
    options.c_oflag = 0;                                 
    options.c_lflag = 0;                                  

    //options.c_lflag &= ~(ICANON | ECHO | ECHOE); 
    // options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(ICRNL | IXON);
    // options.c_oflag &= ~OPOST; 

    if (tcsetattr( fd, TCSANOW, &options ) == -1){
        tcsetattr( fd, TCSANOW, &old_options );
        return(-1);
    }

    return(0);
}


/*
*
*  serial_read_n()
*
*
*/
ssize_t serial_read_n( int fd, const uint8_t *read_buffer, ssize_t read_size, uint32_t timeout)
{
    int nfds;
    ssize_t nread = 0 ;
    fd_set readfds;
    struct timeval tv;

    tv.tv_sec = 3;
    tv.tv_usec = 0;
    
    FD_ZERO(&readfds);
    FD_SET(fd,&readfds);
    nfds = select(fd+1, &readfds, NULL, NULL, &tv);
    if(nfds == 0) {
        printf("serial_read_n-timeout!\r\n");
    } else {
        nread = read(fd, (void *)read_buffer,read_size);
    }

    return nread;
}



/**
*  serial_write_n()
*/

ssize_t serial_write_n(int fd, const uint8_t *write_buffer, ssize_t write_size)
{
    int nfds;
    ssize_t real_write_count = 0 ;
    fd_set writefds;
    struct timeval tv;

    tv.tv_sec = 3;
    tv.tv_usec = 0;
    
    FD_ZERO(&writefds);
    FD_SET(fd,&writefds);
    nfds = select(fd+1,  NULL, &writefds, NULL, &tv);
    if(nfds == 0) {
        printf("serial_read_n-timeout!\r\n");
    } else {
        real_write_count = write(fd, write_buffer,write_size);
    }

    if (real_write_count < 0) {
        perror( "write errot\n" );
        return(-1);
    }

    return real_write_count;
}

esp_loader_error_t loader_port_serial_write(int fd, const uint8_t *data, uint16_t size)
{
    int write_len = serial_write_n(fd, data, size);
    
    if (write_len < 0) {
        return ESP_LOADER_ERROR_FAIL;
    } else if (write_len < size) {
        return ESP_LOADER_ERROR_TIMEOUT;
    } else {
        return ESP_LOADER_SUCCESS;
    }
}

esp_loader_error_t loader_port_serial_read(int fd, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    ssize_t read_len = serial_read_n(fd, data, size, timeout);

    if (read_len < 0) {
        return ESP_LOADER_ERROR_FAIL;
    } else if (read_len < size) {
        return ESP_LOADER_ERROR_TIMEOUT;
    } else {
        return ESP_LOADER_SUCCESS;
    }
}

void loader_port_delay_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

void loader_port_enter_bootloader(int fd)
{
    int status;
    ioctl(fd, TIOCMGET, &status);
    status &= ~TIOCM_DTR;
    status &= ~TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);

    loader_port_delay_ms(50);

    ioctl(fd, TIOCMGET, &status);
    status &= ~TIOCM_DTR;
    status |= TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);

    // gpio_set_level(s_reset_trigger_pin, 0);
    // gpio_set_level(s_reset_trigger_pin, 1);
    // loader_port_delay_ms(50);
    // gpio_set_level(s_gpio0_trigger_pin, 1);
}

void loader_port_reset_target(int fd)
{
    int status;
    ioctl(fd, TIOCMGET, &status);
    status &= ~TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);

    sleep(5);

    ioctl(fd, TIOCMGET, &status);
    status |= TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);
}


void loader_port_start_timer(uint32_t ms)
{
    time_t now;
    s_time_end = time(&now) + ms * 1000;
}


uint32_t loader_port_remaining_time(void)
{
    time_t now;
    int64_t remaining = (s_time_end - time(&now)) / 1000;
    return (remaining > 0) ? (uint32_t)remaining : 0;
}


