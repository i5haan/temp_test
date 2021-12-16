#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "udp.h"
#include "common.h"

struct checkpoint_region {
    int currentEnd;
    int imapLocs[MAX_INODE/NUM_INODES];
    int currentInodeNum;
} cr;

struct inode {
    int size;
    int type;
    int blocks[DIRECT_POINTERS];
};

struct inode_map {
    int inodeLoc[NUM_INODES];
};

struct empty_dir_entry {
    int index;
    int block;
    int inum;
};

struct MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
};

// struct __MFS_DirEnt_t {
//     char name[MAX_DIR_NAME];  // up to 28 bytes of name in directory (including \0)
//     int  inum;      // inode number of entry (-1 means entry not used)
// };

struct directory {
    struct __MFS_DirEnt_t inums[(BLOCK_SIZE - MAX_DIR_NAME) / 32];
};

struct inode_map imaps[MAX_INODE/NUM_INODES];

char *filename;

int getNextAvailableInode() {
    int i;
    for(i = 0; i < MAX_INODE/NUM_INODES; i++) {
        int j;
        for(j = 0; j < NUM_INODES; j++) {
            if(imaps[i].inodeLoc[j] == -1) {
                return (i * NUM_INODES) + j;
            }
        }
    }

    return -1;
}

void writeDataAt(int fd, char* data, int pos, int size) {
    lseek(fd, pos, SEEK_SET); // Checkpoint resides at start so, that is where we will start writing
    write(fd, data, size);
}

void writeInodeAt(int fd, struct inode i, int pos) {
    char* data = malloc((sizeof(struct inode)));
    *((struct inode*)data) = i;


    writeDataAt(fd, data, pos, sizeof(struct inode));
}

// void writeDirectoryDataBlockAt(int fd, struct directory d, int pos) {
//     char* data = malloc((sizeof(struct directory)));
//     *((struct directory*)data) = d;


//     writeDataAt(fd, data, pos, sizeof(struct directory));
// }

void writeDirectoryDataBlockAt(int fd, struct directory *d, int pos) {
    char* data = malloc((sizeof(struct directory)));
    *((struct directory*)data) = *d;


    writeDataAt(fd, data, pos, sizeof(struct directory));
}

void writeImapAt(int fd, struct inode_map imap, int pos) {
    char* data = malloc((sizeof(struct inode_map)));
    *((struct inode_map*)data) = imap;


    writeDataAt(fd, data, pos, sizeof(struct inode_map));
}

void readDataAt(int fd, char* output, int pos, int size) {
    lseek(fd, pos, SEEK_SET);

    read(fd, output, size);
}

struct inode_map readImapAt(int fd, int pos) {
    char* output = (char*) malloc(sizeof(struct inode_map));

    readDataAt(fd, output, pos, sizeof(struct inode_map));

    return *(struct inode_map*) (output);
}

void readImaps(int fd) {
    int i;

    for(i = 0; i < MAX_INODE/NUM_INODES; i++) {
        imaps[i] = readImapAt(fd, cr.imapLocs[i]);
    }
}

struct inode readInodeAt(int fd, int pos) {
    char* output = (char*) malloc(sizeof(struct inode));
    readDataAt(fd, output, pos, sizeof(struct inode));

    return *(struct inode*) (output);
}

struct directory readDirAt(int fd, int pos) {
    char* output = (char*) malloc(sizeof(struct directory));
    readDataAt(fd, output, pos, sizeof(struct directory));

    return *(struct directory*) (output);
}

void readDirAtPtr(int fd, int pos, struct directory *dir) {
    char* output = (char*) malloc(sizeof(struct directory));
    readDataAt(fd, output, pos, sizeof(struct directory));
    dir = (struct directory*) malloc(sizeof(struct directory));
    *dir = *(struct directory*) (output);
}

// If file is present, load from file into memory, or create a new
void initCheckpoint() {
    cr.currentEnd = sizeof(struct checkpoint_region);
    cr.currentInodeNum = 0;
    int i = 0;

    for(i = 0;i < MAX_INODE/NUM_INODES; i++) {
        cr.imapLocs[i] = -1;
    }
}

void initImaps() {
    int i;
    int j;
    for(i = 0; i < MAX_INODE/NUM_INODES; i++) {
        for(j = 0;j < NUM_INODES; j++) {
            imaps[i].inodeLoc[j] = -1;
        }
    }
}

void writeCheckpoint(int fd) {
    lseek(fd, 0, SEEK_SET); // Checkpoint resides at start so, that is where we will start writing
    char* finalOutput = malloc((sizeof(struct checkpoint_region)));
    *((struct checkpoint_region*)finalOutput) = cr;


    write(fd, finalOutput, sizeof(struct checkpoint_region));
}

void readCheckpoint(int fd) {
    lseek(fd, 0, SEEK_SET);
    char* finalOutput = malloc((sizeof(struct checkpoint_region)));


    read(fd, finalOutput, sizeof(struct checkpoint_region));
    cr = *(struct checkpoint_region*) (finalOutput);

}

// Creates an inode struct and returns
struct inode createInode(int type) {
    struct inode newInode;

    newInode.size = 0;
    newInode.type = type;

    int i;
    for(i = 0; i < DIRECT_POINTERS; i++) {
        newInode.blocks[i] = -1;
    }

    return newInode;
}

void createDirData(struct directory *dir) {
    int i;
    for(i = 0; i < (BLOCK_SIZE - MAX_DIR_NAME - 4) / 32; i++) {
        dir->inums[i].inum = -1;
        strcpy(dir->inums[i].name, "");
    }
}

struct empty_dir_entry checkNameInInode(int fd, struct inode in, char *name) {
    int i;
    for(i = 0; i < DIRECT_POINTERS; i++) {
        if(in.blocks[i] == -1) {
            break;
        }
        struct directory d = readDirAt(fd, in.blocks[i]);
        int j;
        for(j = 0; j < (BLOCK_SIZE - MAX_DIR_NAME - 4) / 32; j++) {
            if(strcmp(d.inums[j].name, name) == 0) {
                return (struct empty_dir_entry) {.block = i, .index = j, .inum = d.inums[j].inum};
            }
        }

    }
    return (struct empty_dir_entry) {.block = -1, .index = -1, .inum = -1};;
}
 

struct empty_dir_entry getNextEmptyDirEntry(int fd, struct inode in) {
    int i;
    for(i = 0; i < DIRECT_POINTERS; i++) {
        if(in.blocks[i] == -1) {
            // If we reached here, it means previous all block are filled
            // Just return the block number with -1 index
            return (struct empty_dir_entry) { .index = -1, .block = i};
        }
        struct directory d = readDirAt(fd, in.blocks[i]);
        int j;
        for(j = 0; j < (BLOCK_SIZE - MAX_DIR_NAME - 4) / 32; j++) {
            // If we found an entry in an already present block
            if(d.inums[j].inum -1) {
                return (struct empty_dir_entry) { .index = j, .block = i};
            }
        }

    }
    // -1 block indicates, a new block is not free
    return (struct empty_dir_entry) { .index = -1, .block = -1};
}



// Warning! Does not call checkpoint in this function!
int addEntryToDirectory(int fd, char name[MAX_DIR_NAME], int inum,  int pinum) {
    int imapNum = pinum / (MAX_INODE/NUM_INODES);
    int inodeLoc = imaps[imapNum].inodeLoc[pinum % (MAX_INODE/NUM_INODES)];

    struct inode pInode = readInodeAt(fd, inodeLoc);

    struct empty_dir_entry entry = getNextEmptyDirEntry(fd, pInode);
    struct directory *dir;
    dir = (struct directory*) malloc(sizeof(struct directory));
    if(entry.block == -1) {
        return -1;
    }

    if(entry.block != -1 && entry.index == -1) {
        createDirData(dir);
        entry.index = 0;
    } else {
        readDirAtPtr(fd, pInode.blocks[entry.block], dir);
    }

    int currPos = cr.currentEnd;
    // int j;
    // printf("LOOKUP: %d\n", pInode.blocks[entry.block]);
    strcpy(dir->inums[entry.index].name, name);
    dir->inums[entry.index].inum = inum;
    // for(j = 0; j < dir.numDir; j++) {
    //     printf("%s\n", dir.inums[j].name);
    // }

    writeDirectoryDataBlockAt(fd, dir, currPos);
    pInode.blocks[entry.block] = currPos;
    currPos += sizeof(struct directory);



    writeInodeAt(fd, pInode, currPos);
    imaps[imapNum].inodeLoc[pinum % (MAX_INODE/NUM_INODES)] = currPos;
    currPos += sizeof(struct inode);

    writeImapAt(fd, imaps[imapNum], currPos);
    cr.imapLocs[imapNum] = currPos;
    
    currPos += sizeof(struct inode_map);

    cr.currentEnd = currPos;
    
    return 0;
}

void createAndWriteDir(int fd, char name[MAX_DIR_NAME], int pinum) {
    // get a new inode
    struct inode newInode = createInode(MFS_DIRECTORY);
    newInode.size = BLOCK_SIZE;
    printf("%s\n", name);

    int newSelfInum = getNextAvailableInode();
    cr.currentInodeNum++; // Next Inode
    int currPos = cr.currentEnd;
    // base data for the directory
    struct directory *dir;
    dir = (struct directory*) malloc(sizeof(struct directory));
    createDirData(dir);

    strcpy(dir->inums[0].name, ".");
    dir->inums[0].inum = newSelfInum;

    strcpy(dir->inums[1].name, "..");
    dir->inums[1].inum = pinum == -1 ? newSelfInum : pinum; // For root dir

    // All the writing starts now
    // Write directory data
    writeDirectoryDataBlockAt(fd, dir, currPos);
    newInode.blocks[0] = currPos; // First block for a new directory
    currPos += sizeof(struct directory);

    // Write Inode Data
    writeInodeAt(fd, newInode, currPos);
    int imapNum = newSelfInum / (MAX_INODE/NUM_INODES);
    // update imap
    imaps[imapNum].inodeLoc[newSelfInum % (MAX_INODE/NUM_INODES)] = currPos; // Update location

    currPos += sizeof(struct inode);


    // Write Imap
    writeImapAt(fd, imaps[imapNum], currPos);
    cr.imapLocs[imapNum] = currPos;

    currPos += sizeof(struct inode_map);

    cr.currentEnd = currPos;

    // If not root, then we will also update the directory strucutre of the parent inum
    if(pinum != -1) {
        addEntryToDirectory(fd, name, newSelfInum, pinum);
    }


    writeCheckpoint(fd);

}


void createAndWriteFile(int fd, char name[MAX_DIR_NAME], int pinum) {
    // get a new inode
    struct inode newInode = createInode(MFS_REGULAR_FILE);
    printf("%s\n", name);

    int newSelfInum = getNextAvailableInode();
    cr.currentInodeNum++; // Next Inode
    int currPos = cr.currentEnd;


    // Write Inode Data
    writeInodeAt(fd, newInode, currPos);
    int imapNum = newSelfInum / (MAX_INODE/NUM_INODES);
    // update imap
    imaps[imapNum].inodeLoc[newSelfInum % (MAX_INODE/NUM_INODES)] = currPos; // Update location

    currPos += sizeof(struct inode);


    // Write Imap
    writeImapAt(fd, imaps[imapNum], currPos);
    cr.imapLocs[imapNum] = currPos;

    currPos += sizeof(struct inode_map);

    cr.currentEnd = currPos;

    // If not root, then we will also update the directory strucutre of the parent inum
    if(pinum != -1) {
        addEntryToDirectory(fd, name, newSelfInum, pinum);
    }


    writeCheckpoint(fd);

}

void createRootDir(int fd) {
    createAndWriteDir(fd, "root", -1);
}

void writeToBlockAtInode(int fd, int inum, char* buffer, int block) {
    int imapNum = inum / (MAX_INODE/NUM_INODES);
    struct inode in = readInodeAt(fd, imaps[imapNum].inodeLoc[inum % (MAX_INODE/NUM_INODES)]);

    int currPos = cr.currentEnd;

    int prevFilled = 1;
    if(in.blocks[block] == -1) {
        prevFilled = 0;
    }    
    printf("Writing at %d\n", currPos);
    writeDataAt(fd, buffer, currPos, BLOCK_SIZE);
    in.blocks[block] = currPos;

    currPos += BLOCK_SIZE;
    if(prevFilled == 0) {
        in.size += BLOCK_SIZE;
    }
    writeInodeAt(fd, in, currPos);
    imaps[imapNum].inodeLoc[inum % (MAX_INODE/NUM_INODES)] = currPos; // Update location
    
    currPos += sizeof(struct inode);
    // Write Imap
    writeImapAt(fd, imaps[imapNum], currPos);
    cr.imapLocs[imapNum] = currPos;

    currPos += sizeof(struct inode_map);

    cr.currentEnd = currPos;

    writeCheckpoint(fd);
}

int readFromBlockAtInode(int fd, int inum, char* buffer, int block) {
    int imapNum = inum / (MAX_INODE/NUM_INODES);
    struct inode in = readInodeAt(fd, imaps[imapNum].inodeLoc[inum % (MAX_INODE/NUM_INODES)]);

    if(in.blocks[block] == -1) {
        return -1;
    }    
    printf("Reading at %d\n", in.blocks[block]);
    readDataAt(fd, buffer, in.blocks[block], BLOCK_SIZE);
    return 0;
}

void removeEntryFromDirectory(int fd, struct inode pin, int pinum, int inum, struct empty_dir_entry entry) {
    int block = entry.block;
    int index = entry.index;

    struct directory *dir;
    dir = (struct directory*) malloc(sizeof(struct directory));
    readDirAtPtr(fd, pin.blocks[block], dir);

    dir->inums[index].inum = -1;
    strcpy(dir->inums[index].name, "");


    int currPos = cr.currentEnd;

    writeDirectoryDataBlockAt(fd, dir, currPos);

    pin.blocks[block] = -1;

    currPos += sizeof(struct directory);

    writeInodeAt(fd, pin, currPos);
    imaps[inum / (MAX_INODE/NUM_INODES)].inodeLoc[inum % (MAX_INODE/NUM_INODES)] = -1; // Update location
    
    currPos += sizeof(struct inode);
    // Write Imap
    writeImapAt(fd, imaps[pinum / (MAX_INODE/NUM_INODES)], currPos);
    cr.imapLocs[pinum / (MAX_INODE/NUM_INODES)] = currPos;

    currPos += sizeof(struct inode_map);

    cr.currentEnd = currPos;

    writeCheckpoint(fd);
}

int isDirectoryEmpty(int fd, struct inode in) {
    int count = 0;
    int i;
    for(i = 0; i < DIRECT_POINTERS; i++) {
        if(in.blocks[i] == -1) {
            break;
        }
        struct directory d = readDirAt(fd, in.blocks[i]);
        int j;
        for(j = 0; j < (BLOCK_SIZE - MAX_DIR_NAME - 4) / 32; j++) {
            if(d.inums[j].inum != -1) {
                count++;
            }
        }
    }

    if(count == 2) {
        return 0;
    } else {
        return -1;
    }
}

int _Lookup(int pinum, char *name) {
    printf("server:: LOOKUP\n");
    int fd = open(filename, O_RDWR);
    int imapNum = pinum / (MAX_INODE/NUM_INODES);
    // check if inode exists 
    if(imaps[imapNum].inodeLoc[pinum % (MAX_INODE/NUM_INODES)] == -1) {
        printf("server:: LOOKUP :: Err-> pinum not found\n");
        close(fd);
        return -1;
    }

    struct inode retInode = readInodeAt(fd, imaps[imapNum].inodeLoc[pinum % (MAX_INODE/NUM_INODES)]);

    if(retInode.type != MFS_DIRECTORY) {
        printf("server:: LOOKUP :: Err-> Not a directory\n");
        close(fd);
        return -1;
    }

    struct empty_dir_entry entry = checkNameInInode(fd, retInode, name);

    close(fd);
    return entry.inum;
}

int _Stat(int inum, struct __MFS_Stat_t *m) {
    printf("server:: STAT\n");
    // m = (struct MFS_Stat_t*) malloc(sizeof(struct MFS_Stat_t));
    int fd = open(filename, O_RDWR);
    int imapNum = inum / (MAX_INODE/NUM_INODES);
    // check if inode exists 
    if(imaps[imapNum].inodeLoc[inum % (MAX_INODE/NUM_INODES)] == -1) {
        printf("server:: STAT :: Err-> inum not found\n");
        close(fd);
        return -1;
    }

    struct inode retInode = readInodeAt(fd, imaps[imapNum].inodeLoc[inum % (MAX_INODE/NUM_INODES)]);

    m->type = retInode.type;
    m->size = retInode.size;

    close(fd);
    return 0;
}
int _Write(int inum, char *buffer, int block) {
    printf("server:: WRITE\n");
    int fd = open(filename, O_RDWR);
    int imapNum = inum / (MAX_INODE/NUM_INODES);
    // check if inode exists 
    if(imaps[imapNum].inodeLoc[inum % (MAX_INODE/NUM_INODES)] == -1) {
        printf("server:: WRITE :: Err-> inum not found\n");
        close(fd);
        return -1;
    }

    struct inode in = readInodeAt(fd, imaps[imapNum].inodeLoc[inum % (MAX_INODE/NUM_INODES)]);

    if(in.type != MFS_REGULAR_FILE) {
        printf("server:: WRITE :: Err-> Not a file\n");
        close(fd);
        return -1;
    }

    if(block >= DIRECT_POINTERS || block < 0) {
        printf("server:: WRITE :: Err-> inum not found\n");
        close(fd);
        return -1;
    }

    writeToBlockAtInode(fd, inum, buffer, block);

    close(fd);
    return 0;
}

int _Read(int inum, char *buffer, int block) {
    printf("server:: READ\n");
    int fd = open(filename, O_RDWR);
    int imapNum = inum / (MAX_INODE/NUM_INODES);
    // check if inode exists 
    if(imaps[imapNum].inodeLoc[inum % (MAX_INODE/NUM_INODES)] == -1) {
        printf("server:: READ :: Err-> inum not found\n");
        close(fd);
        return -1;
    }

    if(block >= DIRECT_POINTERS || block < 0) {
        printf("server:: WRITE :: Err-> Invalid Block\n");
        close(fd);
        return -1;
    }

    if(readFromBlockAtInode(fd, inum, buffer, block) == -1) {
        printf("server:: WRITE :: Err-> Block empty\n");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int _Creat(int pinum, int type, char *name) {
    printf("server:: CREAT\n");
    int fd = open(filename, O_RDWR);
    int imapNum = pinum / (MAX_INODE/NUM_INODES);
    // check if inode exists 
    if(imaps[imapNum].inodeLoc[pinum % (MAX_INODE/NUM_INODES)] == -1) {
        printf("server:: CREAT :: Err-> pinum not found\n");
        close(fd);
        return -1;
    }

    struct inode pInode = readInodeAt(fd, imaps[imapNum].inodeLoc[pinum % (MAX_INODE/NUM_INODES)]);

    if(pInode.type != MFS_DIRECTORY) {
        printf("server:: CREAT :: Err-> not a dir\n");
        close(fd);
        return -1;
    }
    struct empty_dir_entry entry = checkNameInInode(fd, pInode, name);
    if(entry.inum != -1) {
        printf("server:: CREAT :: Err-> Already in dir\n");
        close(fd);
        return 0;
    }

    if(type == MFS_DIRECTORY) {
        createAndWriteDir(fd, name, pinum);
    } else if(type == MFS_REGULAR_FILE) {
        createAndWriteFile(fd, name, pinum);
    }

    close(fd);
    return 0;
}

int _Unlink(int pinum, char *name) {
    printf("server:: UNLINK\n");
    int fd = open(filename, O_RDWR);
    int imapNum = pinum / (MAX_INODE/NUM_INODES);
    if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        printf("server:: UNLINK :: Err-> Can't remove . and ..!\n");
        close(fd);
        return -1;
    }
    // check if inode exists 
    if(imaps[imapNum].inodeLoc[pinum % (MAX_INODE/NUM_INODES)] == -1) {
        printf("server:: UNLINK :: Err-> pinum not found\n");
        close(fd);
        return -1;
    }

    struct inode pInode = readInodeAt(fd, imaps[imapNum].inodeLoc[pinum % (MAX_INODE/NUM_INODES)]);

    if(pInode.type != MFS_DIRECTORY) {
        printf("server:: CREAT :: Err-> not a dir\n");
        close(fd);
        return -1;
    }

    struct empty_dir_entry entry = checkNameInInode(fd, pInode, name);
    if(entry.inum == -1) {
        printf("server:: CREAT :: Err-> Not found\n");
        close(fd);
        return 0;
    }


    struct inode in = readInodeAt(fd, imaps[entry.inum / (MAX_INODE/NUM_INODES)].inodeLoc[entry.inum % (MAX_INODE/NUM_INODES)]);

    if(in.type == MFS_REGULAR_FILE) {
        removeEntryFromDirectory(fd, pInode, pinum, entry.inum, entry);
    } else if(in.type == MFS_DIRECTORY) {
        if(isDirectoryEmpty(fd, pInode) == 0) {
            removeEntryFromDirectory(fd, pInode, pinum, entry.inum, entry);
        } else {
            printf("server:: CREAT :: Err-> Not empty\n");
            close(fd);
            return -1;
        }
    }


    close(fd);
    return 0;
}

int _Shutdown() {
    printf("server:: SHUTDOWN\n");
    exit(0);
}

void initFS() {
    int fd = open(filename, O_RDWR);

    // file does not exist
    if(fd < 0) {
        printf("LOG: File system image not found, creating new!!\n");
        fd = open(filename, O_RDWR | O_CREAT);
        initCheckpoint();
        writeCheckpoint(fd);
    
        initImaps();
    
        int currPos = cr.currentEnd;
    
        int i;
        for(i = 0; i < MAX_INODE/NUM_INODES; i++) {
            cr.imapLocs[i] = currPos;
            writeImapAt(fd, imaps[i], currPos);
            currPos += sizeof(struct inode_map);
        }
    
        cr.currentEnd = currPos;
    
        writeCheckpoint(fd);
        createRootDir(fd);
        
    } else {
        printf("LOG: File system image found, loading!!\n");
        readCheckpoint(fd);

        readImaps(fd);
    }
    close(fd);
}

void performActionAndReturn(struct DTO req, struct DTO *res) {
    if(req.request == REQ_CREAT) {
        int r = _Creat(req.inum, req.stat.type, req.name);
        res->ret = r;
    } else if(req.request == REQ_LOOKUP){
        int r = _Lookup(req.inum, req.name);
        res->ret = r;
    } else if(req.request == REQ_STAT) {
        int r = _Stat(req.inum, &(res->stat));
        res->ret = r;
    } else if(req.request == REQ_WRITE) {
        int r = _Write(req.inum, req.buffer, req.block);
        res->ret = r;
    } else if(req.request == REQ_READ) {
        int r = _Read(req.inum, (res->buffer), req.block);
        res->ret = r;
    } else if(req.request == REQ_UNLINK) {
        int r = _Unlink(req.inum, req.name);
        res->ret = r;
    } else if(req.request == REQ_SHUTDOWN) {
        _Shutdown();
        res->ret = 0;
    } else {
        printf("server:: Request type not servable!");
    }
}

int server_init(int port, char* image_path) {
    filename = (char *) malloc(sizeof(char)*100);
    strcpy(filename, image_path);
    initFS();
    int sd = UDP_Open(port);
    assert(sd > -1);
    struct DTO req, res;
    while (1) {
        struct sockaddr_in addr;
        printf("server:: waiting...\n");
        int rc = UDP_Read(sd, &addr, (char*)&req, sizeof(DTO));
        printf("server:: request payload [size:%d contents:(Req: %d, inum: %d, block: %d, ret: %d, name: %s, buf: %s, stat.size: %d, stat.type: %d)]\n", rc, req.request, req.inum, req.block, req.ret, req.name, req.buffer, req.stat.size, req.stat.type);
        if (rc > 0) {
                performActionAndReturn(req, &res);

                printf("server:: response payload [size:%d contents:(Req: %d, inum: %d, block: %d, ret: %d, name: %s, buf: %s, stat.size: %d, stat.type: %d)]\n", rc, res.request, res.inum, res.block, res.ret, res.name, res.buffer, res.stat.size, res.stat.type);
                
                rc = UDP_Write(sd, &addr, (char*)&res, sizeof(DTO));
            printf("server:: reply\n");
        } 
    }

    return 0;
}
    
    
int main(int argc, char *argv[])
{
    if(argc != 3) {
        perror("Usage: server <portnum> <image>\n");
        exit(1);
    }

    server_init(atoi(argv[1]),argv[2] );

    return 0;
}

// int main(int argc, char *argv[]) {
//     filename = (char *) malloc(sizeof(char)*100);
//     strcpy(filename, "fs_image");
//     initFS();

//     // struct MFS_Stat_t m;

//     MFS_Creat(0, MFS_REGULAR_FILE, "test");

//     printf("%d\n", MFS_Lookup(0, "test"));

//     // printf("%d\n", MFS_Write(1, "ishaan", 0));

//     // char *buf = (char*) malloc(BLOCK_SIZE);

//     // printf("%d\n", MFS_Read(1, buf, 0));
//     // printf("%s\n", buf);

//     // MFS_Unlink(0, "lolol1");

//     // printf("%d\n", MFS_Lookup(0, "lolol1"));

//     // printf("%d\n", MFS_Creat(0, MFS_REGULAR_FILE, "lolol1"));
    
//     // printf("%d\n", MFS_Lookup(0, "lolol1"));
//     // printf("Size: %d, type: %d\n", m.size, m.type);

//     // int i;

//     // for(i = 0; i < MAX_INODE/NUM_INODES; i++) {
//     //     int j;
//     //     for(j = 0; j < NUM_INODES; j++) {
//     //         printf("%d\n", imaps[i].inodeLoc[j]);
//     //     }
//     // }
// }

// server code
// int main(int argc, char *argv[]) {
    
//     return 0; 
// }
    


 