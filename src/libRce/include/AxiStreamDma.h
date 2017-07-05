#ifndef __AXI_STREAM_DMA_H__
#define __AXI_STREAM_DMA_H__

#ifdef __linux__
#include <linux/types.h>
#else
#include <stdint.h>
typedef int32_t __s32;
typedef uint32_t __u32;
typedef uint8_t __u8;
typedef uint16_t __u16;
#endif

#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

// Return Errors
#define ERR_DRIVER     -1
#define ERR_BUFF_OFLOW -2
#define ERR_DMA_OFLOW  -3
#define ERR_AXI_WRITE  -4

// Get Write Buffer Count
// inline void axisGetWriteBufferCount(__s32 fd)

// Get Write Buffer Size 
// inline void axisGetWriteBufferSize(__s32 fd)

// Write Data
// inline ssize_t axisWrite(__s32 fd, const void *buf, size_t size, __u32 fUser, __u32 lUser, __u32 dest )

// Read Data
// inline ssize_t axisRead(__s32 fd, void *buf, size_t size, __u32 *fUser, __u32 *lUser, __u32 *dest )

// Read Index
// inline ssize_t axisReadUser(__s32 fd, __u32 *index, __u32 *fUser, __u32 *lUser, __u32 *dest )

// Post Index
// inline ssize_t axisPostUser(__s32 fd, __u32 index )

// Get Read Buffer Count
// inline void axisGetReadBufferCount(__s32 fd)

// Get Read Buffer Size 
// inline void axisGetReadBufferSize(__s32 fd)

// Read ACK
// inline void axisReadAck (__s32 fd)

// Read Ready
// inline void axisReadReady (__s32 fd)

// Return user space mapping to dma buffers
// inline __u8 ** axisMapUser(__s32 fd, __u32 *count, __u32 *size)

// Free space mapping to dma buffers
// inline void axisUnMapUser(__s32 fd, __u8 ** buffer)

// Assign interrupt handler
// inline void axisAssignHandler (__s32 fd, void (*handler)(int))

struct AxiStreamDmaWrite {
   __u32        command;
   const void * buffer;
   size_t       size;
   __u32        fUser;
   __u32        lUser;
   __u32        dest;
};

struct AxiStreamDmaRead {
   __u32  command;
   void * buffer;
   __s32  index;
   __s32  size;
   __u32  fUser;
   __u32  lUser;
   __u32  dest;
};

// Commands
#define CMD_GET_BSIZE  0x01
#define CMD_GET_BCOUNT 0x02
#define CMD_WRITE_DATA 0x03
#define CMD_READ_COPY  0x04
#define CMD_READ_USER  0x05
#define CMD_POST_USER  0x06
#define CMD_READ_ACK   0x07
#define CMD_READ_READY 0x08

// Everything below is hidden during kernel module compile
#ifndef AXIS_IN_KERNEL
#include <stdlib.h>

// Get Write Buffer Count
inline ssize_t axisGetWriteBufferCount(__s32 fd) {
   struct AxiStreamDmaWrite dmaWr;

   dmaWr.command = CMD_GET_BCOUNT;

   return(write(fd,&dmaWr,sizeof(struct AxiStreamDmaWrite)));
}

// Get Write Buffer Size 
inline ssize_t axisGetWriteBufferSize(__s32 fd) {
   struct AxiStreamDmaWrite dmaWr;

   dmaWr.command = CMD_GET_BSIZE;

   return(write(fd,&dmaWr,sizeof(struct AxiStreamDmaWrite)));
}

// Write Data
inline ssize_t axisWrite(__s32 fd, const void *buf, size_t size, __u32 fUser, __u32 lUser, __u32 dest ) {
   struct AxiStreamDmaWrite dmaWr;

   dmaWr.command = CMD_WRITE_DATA;
   dmaWr.buffer  = buf;
   dmaWr.size    = size;
   dmaWr.fUser   = fUser;
   dmaWr.lUser   = lUser;
   dmaWr.dest    = dest;

   return(write(fd,&dmaWr,sizeof(struct AxiStreamDmaWrite)));
}

// Read Data
inline ssize_t axisRead(__s32 fd, void *buf, size_t size, __u32 *fUser, __u32 *lUser, __u32 *dest ) {
   struct AxiStreamDmaRead dmaRd;
   ssize_t ret;

   dmaRd.command = CMD_READ_COPY;
   dmaRd.buffer  = buf;
   dmaRd.size    = size;

   ret = read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead));

   if ( fUser  != NULL ) *fUser  = dmaRd.fUser;
   if ( lUser  != NULL ) *lUser  = dmaRd.lUser;
   if ( dest   != NULL ) *dest   = dmaRd.dest;

   return(ret);
}

// Read Index
inline ssize_t axisReadUser(__s32 fd, __u32 *index,  __u32 *fUser, __u32 *lUser, __u32 *dest ) {
   struct AxiStreamDmaRead dmaRd;
   ssize_t ret;

   dmaRd.command = CMD_READ_USER;

   ret = read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead));
   
   *index = dmaRd.index;

   if ( fUser  != NULL ) *fUser  = dmaRd.fUser;
   if ( lUser  != NULL ) *lUser  = dmaRd.lUser;
   if ( dest   != NULL ) *dest   = dmaRd.dest;

   return(ret);
}

// Post Index
inline ssize_t axisPostUser(__s32 fd, __u32 index ) {
   struct AxiStreamDmaRead dmaRd;

   dmaRd.command = CMD_POST_USER;
   dmaRd.index   = index;

   return(read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead)));
}

// Get Read Buffer Count
inline ssize_t axisGetReadBufferCount(__s32 fd) {
   struct AxiStreamDmaRead dmaRd;

   dmaRd.command = CMD_GET_BCOUNT;

   return(read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead)));
}

// Get Read Buffer Size 
inline ssize_t axisGetReadBufferSize(__s32 fd) {
   struct AxiStreamDmaRead dmaRd;

   dmaRd.command = CMD_GET_BSIZE;

   return(read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead)));
}

// Read ACK
inline void axisReadAck (__s32 fd) {
   struct AxiStreamDmaRead dmaRd;

   dmaRd.command = CMD_READ_ACK;

   read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead));
}

// Read Ready
inline ssize_t axisReadReady (__s32 fd) {
   struct AxiStreamDmaRead dmaRd;

   dmaRd.command = CMD_READ_READY;

   return(read(fd,&dmaRd,sizeof(struct AxiStreamDmaRead)));
}

// Return user space mapping to dma buffers
inline __u8 ** axisMapUser(__s32 fd, __u32 *count, __u32 *size) {
   void *  temp;
   __u8 ** ret;
   __u32   bCount;
   __u32   bSize;
   __u32   x;;

   bCount = axisGetReadBufferCount(fd);
   bSize  = axisGetReadBufferSize(fd);

   if ( count != NULL ) *count = bCount;
   if ( size  != NULL ) *size  = bSize;

   if ( (ret = (__u8 **)malloc(sizeof(__u8 *) * bCount)) == 0 ) return(NULL);

   for (x=0; x < bCount; x++) {

      if ( (temp = mmap (0, bSize, PROT_READ, MAP_PRIVATE, fd, (bSize*x))) == MAP_FAILED) {
         free(ret);
         return(NULL);
      }

      ret[x] = (__u8 *)temp;
   }

   return(ret);
}

// Free space mapping to dma buffers
inline void axisUnMapUser(__s32 fd, __u8 ** buffer) {
   __u32   bCount;
   __u32   bSize;
   __u32   x;;

   bCount = axisGetReadBufferCount(fd);
   bSize  = axisGetReadBufferSize(fd);

   for (x=0; x < bCount; x++) munmap (buffer, bSize);

   free(buffer);
}

// Assign interrupt handler
inline void axisAssignHandler (__s32 fd, void (*handler)(int)) {
   struct sigaction act;
   __s32 oflags;

   act.sa_handler = handler;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;

   sigaction(SIGIO, &act, NULL);
   fcntl(fd, F_SETOWN, getpid());
   oflags = fcntl(fd, F_GETFL);
   fcntl(fd, F_SETFL, oflags | FASYNC);
}

#endif
#endif

