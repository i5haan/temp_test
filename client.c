#include <stdio.h>
#include "udp.h"
// #include "mfs.h"
#include "common.h"

// client code

// def run(self):
//     self.loadlib()
//     self.start_server()
//     self.mfs_init("localhost", self.port)

//     self.creat(ROOT, MFS_DIRECTORY, "testdir")
//     inum = self.lookup(ROOT, "testdir")

//     self.creat(inum, MFS_REGULAR_FILE, "testfile")
//     self.lookup(inum, "testfile")

//     r = self.libmfs.MFS_Unlink(ROOT, "testdir")
//     if r != -1:
//        raise Failure("MFS_Unlink should fail on non-empty dir")

//     self.unlink(inum, "testfile")
//     self.unlink(ROOT, "testdir")

//     self.shutdown()
//     self.server.wait()
//     self.done()


int main(int argc, char *argv[]) {
    MFS_Init("localhost", 5000);
    printf("Create: %d\n", MFS_Creat(0, MFS_DIRECTORY, "testdir"));
    int inum = MFS_Lookup(0, "testdir");
    printf("Lookup: %d\n", inum);
    // printf("Create2: %d\n", MFS_Creat(inum, MFS_REGULAR_FILE, "testfile"));
    // printf("Lookup2: %d\n", MFS_Lookup(inum, "testfile"));

    // printf("UNLINK: %d\n", MFS_Unlink(0, "testdir"));
    // printf("UNLINK2: %d\n", MFS_Unlink(inum, "testfile"));
    // printf("UNLINK3: %d\n", MFS_Unlink(inum, "testdir"));

    char str[20];

    // int i;
    // for(i = 0; i < 1794; i++) {
    //     sprintf(str, "%d", i);
    //     printf("Create %d: %d\n", i, MFS_Creat(inum, MFS_REGULAR_FILE, str));
    // }

    // for(i = 0; i < 1794; i++) {
    //     sprintf(str, "%d", i);
    //     printf("Lookup %d: %d\n", i, MFS_Lookup(inum, str));
    // }

    int i, j, k;
    for(i = 0;i < 4;i++) {
        sprintf(str, "testdir%d", i);
        printf("Create %d: %d\n", i, MFS_Creat(0, MFS_DIRECTORY, str));
        int ainum = MFS_Lookup(0, str);
        printf("Create %d: %d\n", i, MFS_Creat(ainum, MFS_REGULAR_FILE, "file1"));
        printf("Create %d: %d\n", i, MFS_Creat(ainum, MFS_REGULAR_FILE, "file2"));
        for(j = 0;j < 3;j++) {
            sprintf(str, "testdir%d", j);
            printf("Create %d: %d\n", i, MFS_Creat(ainum, MFS_DIRECTORY, str));
            int binum = MFS_Lookup(ainum, str);
            printf("Create %d: %d\n", i, MFS_Creat(binum, MFS_REGULAR_FILE, "file1"));
            printf("Create %d: %d\n", i, MFS_Creat(binum, MFS_REGULAR_FILE, "file2"));
            for(k = 0;k < 2;k++) {
                sprintf(str, "testdir%d", k);
                printf("Create %d: %d\n", i, MFS_Creat(binum, MFS_DIRECTORY, str));
                int cinum = MFS_Lookup(binum, str);
                printf("Create %d: %d\n", i, MFS_Creat(cinum, MFS_REGULAR_FILE, "file1"));
                printf("Create %d: %d\n", i, MFS_Creat(cinum, MFS_REGULAR_FILE, "file2"));
            }
        }
    }

    for(i = 0;i < 4;i++) {
        sprintf(str, "testdir%d", i);
        int ainum = MFS_Lookup(0, str);
        printf("Lookup Directory: %d\n", ainum);
        printf("Lookup %d: %d\n", i, MFS_Lookup(ainum, "file1"));
        printf("Lookup %d: %d\n", i, MFS_Lookup(ainum, "file2"));
        for(j = 0;j < 3;j++) {
            sprintf(str, "testdir%d", j);
            int binum = MFS_Lookup(ainum, str);
            printf("Lookup Directory: %d\n", binum);
            printf("Lookup %d: %d\n", i, MFS_Lookup(binum, "file1"));
            printf("Lookup %d: %d\n", i, MFS_Lookup(binum, "file2"));
            for(k = 0;k < 2;k++) {
                sprintf(str, "testdir%d", k);
                int cinum = MFS_Lookup(binum, str);
                printf("Lookup Directory: %d\n", cinum);
                printf("Lookup %d: %d\n", i, MFS_Lookup(cinum, "file1"));
                printf("Lookup %d: %d\n", i, MFS_Lookup(cinum, "file2"));
            }
        }
    }

    // printf("Create %d: %d\n", 1793, MFS_Creat(inum, MFS_REGULAR_FILE, "1793"));
    // printf("Lookup %d: %d\n", 1793, MFS_Lookup(inum, "1793"));


    MFS_Shutdown();
    return 0;
}
