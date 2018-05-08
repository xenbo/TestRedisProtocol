//
// Created by dongbo01 on 5/8/18.
//

#ifndef TESTREDISPROTOCOL_SERVER_H
#define TESTREDISPROTOCOL_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "sds.h"



#define OBJ_ENCODING_RAW 0     /* Raw representation */
#define LRU_BITS 24
#define OBJ_STRING 0
#define OBJ_ENCODING_EMBSTR 8  /* Embedded sds string encoding */


typedef struct redisObject {
    unsigned type:4;
    unsigned encoding:4;
    unsigned lru:LRU_BITS; /* LRU time (relative to global lru_clock) or
                            * LFU data (least significant 8 bits frequency
                            * and most significant 16 bits access time). */
    int refcount;
    void *ptr;
} robj;

robj *createObject(int type, void *ptr) ;

robj *createRawStringObject(const char *ptr, size_t len) ;

robj *createEmbeddedStringObject(const char *ptr, size_t len) ;

#define OBJ_ENCODING_EMBSTR_SIZE_LIMIT 44
robj *createStringObject(const char *ptr, size_t len) ;

#endif //TESTREDISPROTOCOL_SERVER_H
