
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define GPIO_BASE_ADDRESS       0x42000000
#define GPIO_MAP_SIZE           0x1000

#define XAXIGPIO_DATA_OFFSET    0x00000000

typedef unsigned long uint32;


int main(int argc, char *argv[]){
    unsigned long ii;
    int memfd;
    void *gpioMapAddr = NULL;

    if ( (memfd = open("/dev/mem", O_RDWR | O_DSYNC)) == -1 ){
        printf("Can't open /dev/mem.\n");
        exit(-1);
    }

    gpioMapAddr = mmap(0, GPIO_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, GPIO_BASE_ADDRESS);
    if (gpioMapAddr == (void *) -1){
        printf("Can't map the CDMA-Control-Port to user space.\n");
        exit(-1);
    }
    
    printf("GPIO mapped at %p\n", gpioMapAddr);

    for(ii=0; ii<256; ii++){
        *((volatile unsigned long*) (gpioMapAddr + XAXIGPIO_DATA_OFFSET)) = ii;
        printf("led = 0x%02x\n", ii);
        usleep(20000); // sleep 20ms
    }
}
