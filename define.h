
//
// Created by dongbo01 on 5/8/18.
//

#ifndef TESTREDISPROTOCOL_DEFINE_H
#define TESTREDISPROTOCOL_DEFINE_H



#define PROTO_IOBUF_LEN         (1024*16)  /* Generic I/O buffer size */


#define PROTO_REQ_INLINE 1
#define PROTO_REQ_MULTIBULK 2

#define PROTO_MBULK_BIG_ARG     (1024*32)
#define PROTO_INLINE_MAX_SIZE   (1024*64) /* Max size of inline reads */
#define C_OK                    0
#define C_ERR                   -1



#endif //TESTREDISPROTOCOL_DEFINE_H
