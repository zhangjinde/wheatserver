#include "worker/worker.h"

/* Sync worker Area */
void setupSync();
void syncWorkerCron();
int syncSendData(struct client *c, struct slice *data); // pass `data` ownership to
int syncRegisterRead(struct client *c);

/* Async worker Area */
void setupAsync();
void asyncWorkerCron();
int asyncSendData(struct client *c, struct slice *data); // pass `data` ownership to
int asyncRegisterRead(struct client *c);

struct worker WorkerTable[] = {
    {"SyncWorker", setupSync, syncWorkerCron, syncSendData, syncRegisterRead},
    {"AsyncWorker", setupAsync, asyncWorkerCron, asyncSendData, asyncRegisterRead}
};

int httpSpot(struct conn*);
int parseHttp(struct conn *, struct slice *);
void *initHttpData();
void freeHttpData(void *data);
int initHttp();
void deallocHttp();

int spotRedis(struct conn *);
int parseRedis(struct conn *, struct slice *);
void *initRedisData();
void freeRedisData(void *data);
int initRedis();
void deallocRedis();

int redisSpot(struct conn *c);
int parseRedis(struct conn *c, struct slice *slice);
void *initRedisData();
void freeRedisData(void *d);
int initRedis();
void deallocRedis();
struct protocol ProtocolTable[] = {
    {"Http", httpSpot, parseHttp, initHttpData, freeHttpData,
        initHttp, deallocHttp},
    {"Redis", redisSpot, parseRedis, initRedisData, freeRedisData,
        initRedis, deallocRedis},
};

int staticFileCall(struct conn *, void *);
int initStaticFile(struct protocol *);
void deallocStaticFile();
void *initStaticFileData(struct conn *);
void freeStaticFileData(void *app_data);

int wsgiCall(struct conn *, void *);
int initWsgi(struct protocol *);
void deallocWsgi();
void *initWsgiAppData(struct conn *);
void freeWsgiAppData(void *app_data);

int redisCall(struct conn *c, void *arg);
int redisAppInit(struct protocol *);
void redisAppDeinit();
void *redisAppDataInit(struct conn *c);
void redisAppDataDeinit(void *data);
struct app appTable[] = {
    {"Http", "wsgi", wsgiCall, initWsgi, deallocWsgi,
        initWsgiAppData, freeWsgiAppData, 0},
    {"Http", "static-file", staticFileCall, initStaticFile, deallocStaticFile,
        initStaticFileData, freeStaticFileData, 0},
    {"Redis", "redis", redisCall, redisAppInit, redisAppDataDeinit,
        redisAppDataInit, redisAppDataDeinit, 0},
};
