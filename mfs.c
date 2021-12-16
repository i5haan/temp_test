#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "udp.h"

#include "mfs.h"
#include "common.h"
/******************** MFS start ********************/

char *server_host;
int *server_port;
int online = 0;

int genericRequest(struct DTO req, struct DTO *res) {

    int sd = UDP_Open(0);
    if(sd < -1)
    {
        perror("udp_send: failed to open socket.");
        return -1;
    }

	struct sockaddr_in addr, addr2;
	// printf("Hostname: %s, port: %d\n", server_host, *server_port);
    int rc = UDP_FillSockAddr(&addr, server_host, *server_port);
    if(rc < 0)
    {
        perror("upd_send: failed to find host");
        return -1;
    }

    fd_set rfds;
    struct timeval tv;
    tv.tv_sec=3;
    tv.tv_usec=0;

    int trial_limit = 5;	/* trial = 5 */
    do {
        FD_ZERO(&rfds);
        FD_SET(sd,&rfds);
        // printf("client:: read payload [size:%d contents:(Req: %d, inum: %d, block: %d, ret: %d, name: %s, buf: %s, stat.size: %d, stat.type: %d)]\n", rc, req.request, req.inum, req.block, req.ret, req.name, req.buffer, req.stat.size, req.stat.type);
		
		UDP_Write(sd, &addr, (char*)&req, sizeof(struct DTO));
        if(select(sd+1, &rfds, NULL, NULL, &tv))
        {
            // printf("client:: wait for reply...\n");
			rc = UDP_Read(sd, &addr2, (char*)res, sizeof(struct DTO));
			// printf("client:: got reply [size:%d contents:(%d)\n", rc, res->ret);
            if(rc > 0)
            {
                UDP_Close(sd);
                return 0;
            }
        }else {
            trial_limit --;
        }
    }while(1);
}



int MFS_Init(char *hostname, int port) {
	server_host = (char*) malloc(sizeof(char)*4096);
	strcpy(server_host, hostname);
	server_port = (int*) malloc(sizeof(int));
	*server_port = port;
	return 0;
}


int MFS_Lookup(int pinum, char *name){
	printf("Looking up!!\n");
	struct DTO req, *res;
	res = (struct DTO*) malloc(sizeof(struct DTO));

	req.inum = pinum;
	req.request = REQ_LOOKUP;
	strcpy(req.name, name);


	genericRequest(req, res);

	return res->ret;
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
	struct DTO req, *res;
	res = (struct DTO*) malloc(sizeof(struct DTO));

	req.inum = inum;
	req.request = REQ_STAT;

	genericRequest(req, res);

	m->type = res->stat.type;
	m->size = res->stat.size;

	return res->ret;
}

int MFS_Write(int inum, char buffer[MFS_BLOCK_SIZE], int block){
	struct DTO req, *res;
	res = (struct DTO*) malloc(sizeof(struct DTO));
	printf("Client: buffer %s\n", buffer);

	int i;
	for(i=0; i < MFS_BLOCK_SIZE; i++)
		req.buffer[i] = buffer[i];

	for(i=0; i < MFS_BLOCK_SIZE; i++)
		printf("%c", buffer[i]);
	printf("\n");

	req.inum = inum;
	req.request = REQ_WRITE;
	req.block = block;
	// strcpy(req.buffer, buffer);

	genericRequest(req, res);


	return res->ret;
}

int MFS_Read(int inum, char *buffer, int block){
	struct DTO req, *res;
	res = (struct DTO*) malloc(sizeof(struct DTO));

	req.inum = inum;
	req.request = REQ_READ;
	req.block = block;

	genericRequest(req, res);

	strcpy(buffer, res->buffer);


	return res->ret;
}

int MFS_Creat(int pinum, int type, char *name){
	struct DTO req, *res;
	res = (struct DTO*) malloc(sizeof(struct DTO));

	req.inum = pinum;
	req.request = REQ_CREAT;
	strcpy(req.name, name);
	req.stat.type = type;


	genericRequest(req, res);

	return res->ret;
}

int MFS_Unlink(int pinum, char *name){
	struct DTO req, *res;
	res = (struct DTO*) malloc(sizeof(struct DTO));

	req.inum = pinum;
	req.request = REQ_UNLINK;
	strcpy(req.name, name);

	genericRequest(req, res);

	return res->ret;
}

int MFS_Shutdown(){
	struct DTO req, *res;
	res = (struct DTO*) malloc(sizeof(struct DTO));

	req.request = REQ_SHUTDOWN;


	genericRequest(req, res);

	return 0;
}