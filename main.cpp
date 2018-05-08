#include <iostream>
#include <cstdio>
#include <cstring>

#include <unistd.h>

extern "C" {
#include "sds.h"
#include "util.h"

#include "define.h"
#include "server.h"
#include "client.h"
}

#include <thread>


int processMultibulkBuffer(client *c) {
    char *newline = NULL;
    int pos = 0, ok;
    long long ll;

    if (c->multibulklen == 0) {
        /* The client should have been reset */


        /* Multi bulk length cannot be read without a \r\n */
        newline = strchr(c->querybuf, '\r');
        if (newline == NULL) {
            if (sdslen(c->querybuf) > PROTO_INLINE_MAX_SIZE) {

                printf("too big mbulk count string\n");
            }
            return C_ERR;
        }

        /* Buffer should also contain \n */
        if (newline - (c->querybuf) > ((signed) sdslen(c->querybuf) - 2))
            return C_ERR;

        /* We know for sure there is a whole line since newline != NULL,
         * so go ahead and find out the multi bulk length. */

        ok = string2ll(c->querybuf + 1, newline - (c->querybuf + 1), &ll);
        if (!ok || ll > 1024 * 1024) {
            printf("invalid mbulk count");
            return C_ERR;
        }

        pos = (newline - c->querybuf) + 2;
        if (ll <= 0) {
            sdsrange(c->querybuf, pos, -1);
            return C_OK;
        }

        c->multibulklen = ll;

        /* Setup argv array on client structure */
        if (c->argv) free(c->argv);
        c->argv = (robj **) malloc(sizeof(robj *) * c->multibulklen);
    }


    while (c->multibulklen) {
        /* Read bulk length if unknown */
        if (c->bulklen == -1) {
            newline = strchr(c->querybuf + pos, '\r');
            if (newline == NULL) {
                if (sdslen(c->querybuf) > PROTO_INLINE_MAX_SIZE) {

                    printf("too big bulk count string");
                    return C_ERR;
                }
                break;
            }

            /* Buffer should also contain \n */
            if (newline - (c->querybuf) > ((signed) sdslen(c->querybuf) - 2))
                break;

            if (c->querybuf[pos] != '$') {

                printf("expected $ but got something else");
                return C_ERR;
            }

            ok = string2ll(c->querybuf + pos + 1, newline - (c->querybuf + pos + 1), &ll);
            if (!ok || ll < 0 || ll > 512 * 1024 * 1024) {

                printf("invalid bulk length");
                return C_ERR;
            }

            pos += newline - (c->querybuf + pos) + 2;
            if (ll >= PROTO_MBULK_BIG_ARG) {
                size_t qblen;

                /* If we are going to read a large object from network
                 * try to make it likely that it will start at c->querybuf
                 * boundary so that we can optimize object creation
                 * avoiding a large copy of data. */
                sdsrange(c->querybuf, pos, -1);
                pos = 0;
                qblen = sdslen(c->querybuf);
                /* Hint the sds library about the amount of bytes this string is
                 * going to contain. */
                if (qblen < (size_t) ll + 2)
                    c->querybuf = sdsMakeRoomFor(c->querybuf, ll + 2 - qblen);
            }
            c->bulklen = ll;
        }

        /* Read bulk argument */
        if (sdslen(c->querybuf) - pos < (unsigned) (c->bulklen + 2)) {
            /* Not enough data (+2 == trailing \r\n) */
            break;
        } else {
            /* Optimization: if the buffer contains JUST our bulk element
             * instead of creating a new object by *copying* the sds we
             * just use the current sds string. */
            if (pos == 0 &&
                c->bulklen >= PROTO_MBULK_BIG_ARG &&
                (signed) sdslen(c->querybuf) == c->bulklen + 2) {
                c->argv[c->argc++] = createObject(OBJ_STRING, c->querybuf);
                sdsIncrLen(c->querybuf, -2); /* remove CRLF */
                /* Assume that if we saw a fat argument we'll see another one
                 * likely... */
                c->querybuf = sdsnewlen(NULL, c->bulklen + 2);
                sdsclear(c->querybuf);
                pos = 0;
            } else {
                c->argv[c->argc++] = createStringObject(c->querybuf + pos, c->bulklen);
                pos += c->bulklen + 2;
            }
            c->bulklen = -1;
            c->multibulklen--;
        }
    }

    /* Trim to pos */
    if (pos) sdsrange(c->querybuf, pos, -1);

    /* We're done when c->multibulk == 0 */
    if (c->multibulklen == 0) return C_OK;

    /* Still not ready to process the command */
    return C_ERR;
}

/* This function is called every time, in the client structure 'c', there is
 * more query buffer to process, because we read more data from the socket
 * or because a client was blocked and later reactivated, so there could be
 * pending query buffer, already representing a full command, to process. */
void processInputBuffer(client *c) {

    /* Keep processing while there is something in the input buffer */
    while (sdslen(c->querybuf)) {

        /* Determine request type when unknown. */
        if (!c->reqtype) {
            if (c->querybuf[0] == '*') {
                c->reqtype = PROTO_REQ_MULTIBULK;
            } else {
                c->reqtype = PROTO_REQ_INLINE;
            }
        }

        if (c->reqtype == PROTO_REQ_INLINE) {
            //  if (processInlineBuffer(c) != C_OK) break;
        } else if (c->reqtype == PROTO_REQ_MULTIBULK) {
            if (processMultibulkBuffer(c) != C_OK) break;
        } else {
            //serverPanic("Unknown request type");
        }


        static long i1=0;
        for (int i = 0; i < c->argc; ++i) {   //fuck bugs by db
            printf("%d , %s \n", i, c->argv[i]->ptr);
                    printf("%d\n",i1++);
        }

        resetClient(c);

        /* Multibulk processing could see a <= 0 length. */
//        if (c->argc == 0) {
//            resetClient(c);
//        } else {
//            /* Only reset the client when the command was executed. */
//            if (processCommand(c) == C_OK) {
//
//            }
//        }
    }

}

void readQueryFromClient( client *c) {
    int nread, readlen;
    size_t qblen;


    readlen = PROTO_IOBUF_LEN;
    /* If this is a multi bulk request, and we are processing a bulk reply
     * that is large enough, try to maximize the probability that the query
     * buffer contains exactly the SDS string representing the object, even
     * at the risk of requiring more read(2) calls. This way the function
     * processMultiBulkBuffer() can avoid copying buffers to create the
     * Redis Object representing the argument. */
    if (c->reqtype == PROTO_REQ_MULTIBULK && c->multibulklen && c->bulklen != -1
        && c->bulklen >= PROTO_MBULK_BIG_ARG) {
        int remaining = (unsigned) (c->bulklen + 2) - sdslen(c->querybuf);

        if (remaining < readlen) readlen = remaining;
    }

    qblen = sdslen(c->querybuf);
    if (c->querybuf_peak < qblen) c->querybuf_peak = qblen;
    c->querybuf = sdsMakeRoomFor(c->querybuf, readlen);
    nread = read(c->fd, c->querybuf + qblen, readlen);
    if (nread == -1) {
        if (errno == EAGAIN) {
            return;
        } else {
            freeClient(c);
            return;
        }
    } else if (nread == 0) {
        freeClient(c);
        return;
    }

    sdsIncrLen(c->querybuf, nread);


    if (sdslen(c->querybuf) > 1024 * 100) {
//        sds ci = catClientInfoString(sdsempty(),c), bytes = sdsempty();
//
//        bytes = sdscatrepr(bytes,c->querybuf,64);
//        sdsfree(ci);
//        sdsfree(bytes);
//        freeClient(c);
        return;
    }

    /* Time to process the buffer. If the client is a master we need to
     * compute the difference between the applied offset before and after
     * processing the buffer, to understand how much of the replication stream
     * was actually applied to the master state: this quantity, and its
     * corresponding part of the replication stream, will be propagated to
     * the sub-slaves and to the replication backlog. */


    // printf("==:%s\n", c->querybuf);


    processInputBuffer(c);

}


int main() {
    std::string teststr = "*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n*3\r\n$3\r\nset\r\n$6\r\ndbtest\r\n$19\r\nasldjflkasjdfklasdf\r\n";

    int fd[2];
    pipe(fd);

    int sd = fd[1];



    new std::thread ([sd,&teststr]() {
        while (1) {
            write(sd, teststr.c_str(), teststr.length());
            usleep(10);
        }
    });


    client *c = createClient(fd[0]);


    while (1){
        readQueryFromClient(c);

        usleep(1);
    }


    return 0;
}