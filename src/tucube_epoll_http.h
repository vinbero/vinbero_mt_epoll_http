#ifndef _TUCUBE_EPOLL_HTTP_H
#define _TUCUBE_EPOLL_HTTP_H

#include <stdbool.h>
#include <tucube/tucube_Module.h>
#include <tucube/tucube_ClData.h>
#include "tucube_epoll_http_Parser.h"
#include "tucube_epoll_http_ResponseBody.h"

struct tucube_epoll_http_Module {
    int (*tucube_epoll_http_Module_init)(struct tucube_Module_Config*, struct tucube_Module_List*);
    int (*tucube_epoll_http_Module_tlInit)(struct tucube_Module*, struct tucube_Module_Config*);
    int (*tucube_epoll_http_Module_clInit)(struct tucube_Module*, struct tucube_ClData_List*, int*);

    int (*tucube_epoll_http_Module_onRequestStart)(struct tucube_Module*, struct tucube_ClData*);
    int (*tucube_epoll_http_Module_onRequestMethod)(struct tucube_Module*, struct tucube_ClData*, char*, ssize_t);
    int (*tucube_epoll_http_Module_onRequestUri)(struct tucube_Module*, struct tucube_ClData*, char*, ssize_t);
    int (*tucube_epoll_http_Module_onRequestProtocol)(struct tucube_Module*, struct tucube_ClData*, char*, ssize_t);
    int (*tucube_epoll_http_Module_onRequestScriptPath)(struct tucube_Module*, struct tucube_ClData*, char*, ssize_t);
    int (*tucube_epoll_http_Module_onRequestContentType)(struct tucube_Module*, struct tucube_ClData*, char*, ssize_t);
    int (*tucube_epoll_http_Module_onRequestContentLength)(struct tucube_Module*, struct tucube_ClData*, char*, ssize_t);
    

    int (*tucube_epoll_http_Module_onRequestHeaderField)(struct tucube_Module*, struct tucube_ClData*, char*, ssize_t);
    int (*tucube_epoll_http_Module_onRequestHeaderValue)(struct tucube_Module*, struct tucube_ClData*, char*, ssize_t);
    int (*tucube_epoll_http_Module_onRequestHeadersFinish)(struct tucube_Module*, struct tucube_ClData*);
    int (*tucube_epoll_http_Module_onRequestBodyStart)(struct tucube_Module*, struct tucube_ClData*);
    int (*tucube_epoll_http_Module_onRequestBody)(struct tucube_Module*, struct tucube_ClData*, char*, ssize_t);
    int (*tucube_epoll_http_Module_onRequestBodyFinish)(struct tucube_Module*, struct tucube_ClData*);
    int (*tucube_epoll_http_Module_onRequestFinish)(struct tucube_Module*, struct tucube_ClData*);
    
    int (*tucube_epoll_http_Module_onGetRequestContentLength)(struct tucube_Module*, struct tucube_ClData*, ssize_t*);
    int (*tucube_epoll_http_Module_onGetRequestIntHeader)(struct tucube_Module* module, struct tucube_ClData* clData, const char* headerField, int* headerValue);
    int (*tucube_epoll_http_Module_onGetRequestDoubleHeader)(struct tucube_Module* module, struct tucube_ClData* clData, const char* headerField, double* headerValue);
    int (*tucube_epoll_http_Module_onGetRequestStringHeader)(struct tucube_Module* module, struct tucube_ClData* clData, const char* headerField, const char** headerValue);

    int (*tucube_epoll_http_Module_onResponseStatusCode)(struct tucube_Module*, struct tucube_ClData*, int*);
    int (*tucube_epoll_http_Module_onResponseHeaderStart)(struct tucube_Module*, struct tucube_ClData*);
    int (*tucube_epoll_http_Module_onResponseHeader)(struct tucube_Module*, struct tucube_ClData*, const char**, size_t*, const char**, size_t*);
    int (*tucube_epoll_http_Module_onResponseBodyStart)(struct tucube_Module*, struct tucube_ClData*);
    int (*tucube_epoll_http_Module_onResponseBody)(struct tucube_Module*, struct tucube_ClData*, struct tucube_epoll_http_ResponseBody*);

    int (*tucube_epoll_http_Module_clDestroy)(struct tucube_Module*, struct tucube_ClData*);
    int (*tucube_epoll_http_Module_tlDestroy)(struct tucube_Module*);
    int (*tucube_epoll_http_Module_destroy)(struct tucube_Module*);

    size_t parserHeaderBufferCapacity;
    size_t parserBodyBufferCapacity;
};

struct tucube_epoll_http_ClData {
    int* clientSocket;
    struct tucube_epoll_http_Parser* parser;
    bool isKeepAlive;
};

int tucube_tcp_epoll_module_init(struct tucube_Module_Config* moduleConfig, struct tucube_Module_List* moduleList);
int tucube_tcp_epoll_module_tlInit(struct tucube_Module* module, struct tucube_Module_Config* moduleConfig);
int tucube_tcp_epoll_module_clInit(struct tucube_Module* module, struct tucube_ClData_List* clDataList, int* clientSocket);
int tucube_tcp_epoll_module_service(struct tucube_Module* module, struct tucube_ClData*);
int tucube_tcp_epoll_module_clDestroy(struct tucube_Module* module, struct tucube_ClData* cldata);
int tucube_tcp_epoll_module_tlDestroy(struct tucube_Module* module);
int tucube_tcp_epoll_module_destroy(struct tucube_Module* module);

#endif
