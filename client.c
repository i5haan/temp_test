#include <stdio.h>
#include "udp.h"
#include "mfs.h"

enum MFS_REQ {
    REQ_INIT,
    REQ_LOOKUP,
    REQ_STAT,
    REQ_WRITE,
    REQ_READ,
    REQ_CREAT,
    REQ_UNLINK,
    REQ_RESPONSE,
    REQ_SHUTDOWN
  };

#define BUFFER_SIZE (1000)

// client code
int main(int argc, char *argv[]) {
    struct sockaddr_in addrSnd, addrRcv;

    int sd = UDP_Open(20000);
    int rc = UDP_FillSockAddr(&addrSnd, "localhost", 10000);

    char message[BUFFER_SIZE];
    sprintf(message, "hello world");

    printf("client:: send message [%s]\n", message);
    rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    if (rc < 0) {
	printf("client:: failed to send\n");
	exit(1);
    }

    printf("client:: wait for reply...\n");
    rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    return 0;
}
