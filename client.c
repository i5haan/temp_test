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

    int i;
    for(i = 0; i < 1794; i++) {
        sprintf(str, "%d", i);
        printf("Create %d: %d\n", i, MFS_Creat(inum, MFS_REGULAR_FILE, str));
    }

    for(i = 0; i < 1794; i++) {
        sprintf(str, "%d", i);
        printf("Lookup %d: %d\n", i, MFS_Lookup(inum, str));
    }

    // printf("Lookup %d: %d\n", 1793, MFS_Lookup(inum, "1793"));


    MFS_Shutdown();
    return 0;
}
