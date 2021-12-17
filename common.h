#ifndef __COMMMON_h__
#define __COMMON_h__

#include "mfs.h"

#define MFS_BLOCK_SIZE   (4096)

#define BUFFER_SIZE      (1000)

#define MAX_INODE        (4096)
#define NUM_INODES       (16)
#define DIRECT_POINTERS  (14)

#define BLOCK_SIZE       (4096)

#define MAX_DIR_NAME     (28)


#define ROOT_INODE_NUM   (0)

enum MFS_REQ {
    REQ_INIT,
    REQ_LOOKUP,
    REQ_STAT,
    REQ_WRITE,
    REQ_READ,
    REQ_CREAT,
    REQ_UNLINK,
    REQ_SHUTDOWN
};

typedef struct DTO {
    enum MFS_REQ request;

    int inum;
    int block;
    int type;
    int ret;

    char name[MAX_DIR_NAME + 5]; // extra 5 chars to catch error
    char buffer[BLOCK_SIZE];
    struct __MFS_Stat_t stat;
} DTO;

#endif // __MFS_h__