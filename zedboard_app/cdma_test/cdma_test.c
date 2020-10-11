
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

#define CDMA_BASE_ADDRESS       0x43000000
#define CDMA_MAP_SIZE           0x1000

#define DMA_CHUNK_SIZE         (0x01000000)                      // bytes per DMA transfer
#define DMA_CHUNK_COUNT        (4)                               // repeat transfer times
#define DMA_TOTAL_SIZE         (DMA_CHUNK_SIZE*DMA_CHUNK_COUNT)  // DMA_TOTAL_SIZE*2 could not larger than the spare DDR space, 128MiB(0x8000000)
#define SRC_BASE_ADDR           0x10000000
#define DST_BASE_ADDR           SRC_BASE_ADDR + DMA_TOTAL_SIZE

#define XAXICDMA_CR_OFFSET      0x00000000  /**< Control register */
#define XAXICDMA_SR_OFFSET      0x00000004  /**< Status register */
#define XAXICDMA_CDESC_OFFSET   0x00000008  /**< Current descriptor pointer */
#define XAXICDMA_TDESC_OFFSET   0x00000010  /**< Tail descriptor pointer */
#define XAXICDMA_SRCADDR_OFFSET 0x00000018  /**< Source address register */
#define XAXICDMA_DSTADDR_OFFSET 0x00000020  /**< Destination address register */
#define XAXICDMA_BTT_OFFSET     0x00000028  /**< Bytes to transfer */
#define XAXICDMA_CR_RESET_MASK  0x00000004  /**< Reset DMA engine */

typedef unsigned long uint32;
typedef unsigned long long uint64;

int CDMA_reset(void *dma_ctrl_base, int sg_mode_enabled){
    uint32 ResetMask;
    uint32 TimeOut = 100;
    do{
        ResetMask = (uint32)XAXICDMA_CR_RESET_MASK;
        *((volatile uint32 *) (dma_ctrl_base + XAXICDMA_CR_OFFSET)) = (uint32)ResetMask;
        ResetMask = *((volatile uint32 *) (dma_ctrl_base + XAXICDMA_CR_OFFSET));
        if(!(ResetMask & XAXICDMA_CR_RESET_MASK))   // If the reset bit is still high, then reset is not done
            break;
        TimeOut -= 1;
    }while (TimeOut);
    if(TimeOut){
        if(sg_mode_enabled)
            *((volatile uint32 *) (dma_ctrl_base + XAXICDMA_CR_OFFSET)) |=  0x00000008;
        else
            *((volatile uint32 *) (dma_ctrl_base + XAXICDMA_CR_OFFSET)) &= ~0x00000008;
        return 0;
    }else{
        return -1;
    }
}

int CDMA_busy(void *dma_ctrl_base){    // return: 1:busy   0:idle
    volatile uint32 status_reg = *((volatile uint32 *) (dma_ctrl_base + XAXICDMA_SR_OFFSET));
    if ( (status_reg & 0x00000002) == 0 ) {
        //printf("    CDMA is busy...\n");
        return 1;
    }
    return 0;
}

// use Real address for src_addr and dst_addr here, size unit is byte
void CDMA_start(void *dma_ctrl_base, uint64 src_addr, uint64 dst_addr, uint32 size){
    while( CDMA_busy(dma_ctrl_base) );                                              // Wait until CDMA is not busy
    //*((volatile uint32 *) (dma_ctrl_base + XAXICDMA_CR_OFFSET)) |=  0x00007000;       // enable interrupt
    *((volatile uint64*) (dma_ctrl_base + XAXICDMA_SRCADDR_OFFSET)) = src_addr;     // Set the Source Address
    *((volatile uint64*) (dma_ctrl_base + XAXICDMA_DSTADDR_OFFSET)) = dst_addr;     //Set the Destination Address
    *((volatile uint32 *) (dma_ctrl_base + XAXICDMA_BTT_OFFSET)) = size;              // write Length to Start Transfer
}

uint32 time_diff_us(struct timeval timeStart, struct timeval timeEnd){
    return 1000000UL*(timeEnd.tv_sec - timeStart.tv_sec) + timeEnd.tv_usec - timeStart.tv_usec;
}




int main(int argc, char *argv[]){
    uint32 ii, errorcnt = 0;
    int memfd;
    void  *cdmaMapAddr = NULL;
    uint32 *srcMapAddr  = NULL;
    uint32 *dstMapAddr  = NULL;

    if ( (memfd = open("/dev/mem", O_RDWR | O_DSYNC)) == -1 ){
        printf("Can't open /dev/mem.\n");
        exit(-1);
    }

    cdmaMapAddr = mmap(0, CDMA_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, CDMA_BASE_ADDRESS);
    if (cdmaMapAddr == (void *) -1){
        printf("Can't map the CDMA-Control-Port to user space.\n");
        exit(-1);
    }
    
    srcMapAddr  = (uint32*)mmap(0, DMA_TOTAL_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, SRC_BASE_ADDR);
    if (srcMapAddr == (void *) -1){
        printf("Can't map the src memory to user space.\n");
        exit(-1);
    }
    
    dstMapAddr  = (uint32*)mmap(0, DMA_TOTAL_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, DST_BASE_ADDR);
    if (srcMapAddr == (void *) -1){
        printf("Can't map the dst memory to user space.\n");
        exit(-1);
    }
    
    printf("CDMA ctrl mapped at %p, src memory mapped at %p, dst memory mapped at %p\n", cdmaMapAddr, srcMapAddr, dstMapAddr);
    
    if( CDMA_reset(cdmaMapAddr, 0) ){  // Reset CDMA and disable SG-DMA mode
        printf("CDMA reset failed!\n");
        exit(-1);
    }
    
    
    // assign initialization data for src memory
    srand((uint32)time(0));
    printf("initializing data in src memory..."); fflush(stdout);
    for(ii=0; ii<DMA_TOTAL_SIZE/sizeof(*srcMapAddr); ii++)
        srcMapAddr[ii] = (uint32)rand();
    printf("done\n");
    
    
    for(ii=0; ii<DMA_CHUNK_COUNT; ii++){
        struct timeval time_start, time_end; 
        
        gettimeofday(&time_start, NULL);
        
        CDMA_start(cdmaMapAddr, SRC_BASE_ADDR+DMA_CHUNK_SIZE*ii, DST_BASE_ADDR+DMA_CHUNK_SIZE*ii, DMA_CHUNK_SIZE);
        
        while( CDMA_busy(cdmaMapAddr) ); 
        
        gettimeofday(&time_end, NULL);
        
        printf("transfered %d Byte in %u us, speed %.2lfMBps\n" , DMA_CHUNK_SIZE, time_diff_us(time_start, time_end) , ((double)DMA_CHUNK_SIZE) / time_diff_us(time_start, time_end) );
    }

    printf("checking......"); fflush(stdout);
    for(ii=0; ii<DMA_TOTAL_SIZE/sizeof(*srcMapAddr); ii++)
        if(srcMapAddr[ii] != dstMapAddr[ii])
            errorcnt++;
    
    if(errorcnt)
        printf("found %d error bytes\n\n", errorcnt);
    else
        printf("passed\n\n", errorcnt);
}
