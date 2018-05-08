//
// Created by dongbo01 on 5/8/18.
//

#include "client.h"


void freeClient(client *c) {

    sdsfree(c->querybuf);
    sdsfree(c->pending_querybuf);
    c->querybuf = NULL;
    free(c->name);
}


void resetClient(client *c) {
    c->reqtype = 0;
    c->multibulklen = 0;
    c->bulklen = -1;


    for (int i = 0; i <  c->argc ; ++i) {
        free(c->argv[i]);
    }


    c->argc = 0;
}


client *createClient(int fd) {

    client *c = (client*)malloc(sizeof(client));
    c->fd = fd;
    c->name = NULL;
    c->querybuf = sdsempty();
    c->pending_querybuf = sdsempty();
    c->querybuf_peak = 0;
    c->reqtype = 0;
    c->argc = 0;
    c->argv = NULL;
    c->multibulklen = 0;
    c->bulklen = -1;
    c->flags = 0;

    return c;
}