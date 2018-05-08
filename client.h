//
// Created by dongbo01 on 5/8/18.
//

#ifndef TESTREDISPROTOCOL_CLIENT_H
#define TESTREDISPROTOCOL_CLIENT_H


#include "sds.h"
#include "server.h"


typedef struct client {
    uint64_t id;            /* Client incremental unique ID. */
    int fd;                 /* Client socket. */

    robj *name;             /* As set by CLIENT SETNAME. */
    sds querybuf;           /* Buffer we use to accumulate client queries. */
    sds pending_querybuf;   /* If this is a master, this buffer represents the
                               yet not applied replication stream that we
                               are receiving from the master. */
    size_t querybuf_peak;   /* Recent (100ms or more) peak of querybuf size. */
    int argc;               /* Num of arguments of current command. */
    robj **argv;            /* Arguments of current command. */
    int multibulklen;       /* Number of multi bulk arguments left to read. */
    int flags;              /* Client flags: CLIENT_* macros. */
    int reqtype;            /* Request protocol type: PROTO_REQ_* */
    long bulklen;           /* Length of bulk argument in multi bulk request. */
    char buf[100000];
} client;


client *createClient(int fd) ;


void freeClient(client *c);

void resetClient(client *c);

#endif //TESTREDISPROTOCOL_CLIENT_H
