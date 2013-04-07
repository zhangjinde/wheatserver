// Copyright (c) 2013 The Wheatserver Author. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef WHEATSERVER_WORKER_PROCESS_H
#define WHEATSERVER_WORKER_PROCESS_H

#include <unistd.h>
#include <setjmp.h>

#include "../slice.h"
#include "../wstr.h"
#include "mbuf.h"
#include "../slab.h"

#define WORKER_BOOT_ERROR 3
#define WHEATSERVER_REQ_MAXLEN 8*1024

struct workerProcess {
    struct protocol *protocol;
    struct array *apps;
    pid_t pid;
    pid_t ppid;
    int alive;
    struct timeval start_time;

    char *worker_name;
    struct worker *worker;

    // Used by master process
    struct array *stats;
    struct evcenter *center;
    int master_stat_fd;
    // In worker process side, last statistic packet sended time, use
    // `refresh_time` to decide this cron should send statistic packet
    // In master process side, `refresh_time` is used to indicate the last
    // send time of this worker process
    time_t refresh_time;
};

struct client;
struct conn;

struct protocol {
    struct moduleAttr *attr;
    int (*spotAppAndCall)(struct conn *);
    /* `parser` parse `data` and assign protocol data to client
     * if return 0 imply parser success,
     * return 1 means data continued and return -1 means parse
     * error. `nparsed` is value-argument and indicate parsed bytes*/
    int (*parser)(struct conn *, struct slice *, size_t *nparsed);
    void *(*initProtocolData)();
    void (*freeProtocolData)(void *ptcol_data);
    int (*initProtocol)();
    void (*deallocProtocol)();
};

struct worker {
    struct moduleAttr *attr;
    void (*setup)();
    void (*cron)();
    /* Send data in buffer which `buf` points
     * return -1 imply send data failed and the remaining length
     * of bufmeans the length of data not sended.
     * return others means the length of data.
     *
     * Caller will lose ownership of `data` in order to avoid big copy,
     * sendData will free `data`
     */
    int (*sendData)(struct conn *);
    int (*recvData)(struct client *);
};

struct app {
    struct moduleAttr *attr;
    char *proto_belong;
    void (*appCron)();
    /* Return WHEAT_OK means all is ok and return WHEAT_WRONG means something
     * wrong inside and worker will deinit this app. It's important to know
     * requests failed not to return WHEAT_WRONG, request failed your app
     * decision and it should not affetc app itself */
    int (*appCall)(struct conn *, void *arg);
    int (*initApp)(struct protocol *);
    // deallocApp must clean up all data alloced
    void (*deallocApp)();
    void *(*initAppData)(struct conn *);
    void (*freeAppData)(void *app_data);
    int is_init;
};

struct conn {
    struct client *client;
    void *protocol_data;
    struct app *app;
    void *app_private_data;
    struct list *send_queue;
    int ready_send;
    struct array *cleanup;
    struct conn *next;
};

struct client {
    int clifd;
    wstr ip;
    int port;
    struct timeval last_io;
    wstr name;
    struct protocol *protocol;
    struct conn *pending;
    struct list *conns;
    struct msghdr *req_buf;
    void *client_data;       // Only used by app
    void (*notify)(struct client*);
    void *notify_data;

    unsigned is_outer:1;
    unsigned should_close:1; // Used to indicate whether closing client
    unsigned valid:1;        // Intern: used to indicate client fd is unused and
                             // need closing, only used by worker IO methods when
                             // error happended
};

#define WHEAT_WORKERS    2
extern struct workerProcess *WorkerProcess;
extern struct worker *WorkerTable[];
extern struct protocol *ProtocolTable[];
extern struct app *AppTable[];

/* modify attention. Worker, Protocol, Applicantion interface */
void initWorkerProcess(struct workerProcess *worker, char *worker_name);
void freeWorkerProcess(void *worker);
void workerProcessCron(void (*fake_func)(void *data), void *data);

struct client *createClient(int fd, char *ip, int port, struct protocol *p);
void freeClient(struct client *);
void finishConn(struct conn *c);
void tryFreeClient(struct client *c);
void clientSendPacketList(struct client *c);
int sendClientFile(struct conn *c, int fd, off_t len);
struct client *buildConn(char *ip, int port, struct protocol *p);
int sendClientData(struct conn *c, struct slice *s);
struct conn *connGet(struct client *client);
int isClientNeedSend(struct client *);
void registerConnFree(struct conn*, void (*)(void*), void *data);

#define isClientValid(c)                   ((c)->valid)
#define setClientUnvalid(c)                ((c)->valid = 0)
#define isClientNeedParse(c)               (msgCanRead(c)->req_buf))
#define refreshClient(c)                   ((c)->last_io = (Server.cron_time))
#define getConnIP(c)                       ((c)->client->ip)
#define getConnPort(c)                     ((c)->client->port)
#define setClientClose(c)                  ((c)->client->should_close = 1)
#define isOuterClient(c)                   ((c)->is_outer == 1)
#define setClientName(c, n)                ((c)->name = wstrCat(c->name, (n)))
#define setClientFreeNotify(c, func)       ((c)->notify = (func))

/* workerprocess's flow:
 * 0. setup filling workerProcess members
 * 1. accept connection and init client
 * 2. recognize protocol(http or pop3 etc...)
 * 3. read big bulk data from socket (sync or async)
 * 4. call protocol parser
 * 5. call app constructor and pass the protocol parsed data
 * 6. construct response and send to
 *
 * worker's duty:
 * provide with send and receive api and refresh client's last_io field
 * */
struct protocol *spotProtocol(char *ip, int port, int fd);
struct app *spotAppInterface();

#endif
