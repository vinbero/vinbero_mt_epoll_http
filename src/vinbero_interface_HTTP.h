#ifndef _VINBERO_INTERFACE_HTTP_H
#define _VINBERO_INTERFACE_HTTP_H

#include <vinbero_common/vinbero_common_Module.h>
#include <vinbero_common/vinbero_common_ClData.h>

struct vinbero_interface_HTTP_Response {
    struct gaio_Io* io;
    struct vinbero_interface_HTTP_Response_Methods* methods;
};

struct vinbero_interface_HTTP_Response_Methods {
    int (*writeBytes)(struct vinbero_interface_HTTP_Response* response, char* buffer, size_t bufferSize);
    int (*writeIo)(struct vinbero_interface_HTTP_Response* response, struct gaio_Io* io, size_t writeSize);
    int (*writeCrLf)(struct vinbero_interface_HTTP_Response* response);
    int (*writeVersion)(struct vinbero_interface_HTTP_Response* response, int major, int minor);
    int (*writeStatusCode)(struct vinbero_interface_HTTP_Response* response, int statusCode);
    int (*writeIntHeader)(struct vinbero_interface_HTTP_Response* response, char* headerField, size_t headerFieldSize, int headerValue);
    int (*writeDoubleHeader)(struct vinbero_interface_HTTP_Response* response, char* headerField, size_t headerFieldSize, double headerValue);
    int (*writeStringHeader)(struct vinbero_interface_HTTP_Response* response, char* headerField, size_t headerFieldSize, char* headerValue, size_t headerValueSize);
    int (*writeStringBody)(struct vinbero_interface_HTTP_Response* response, char* stringBody, size_t stringBodySize);
    int (*writeIoBody)(struct vinbero_interface_HTTP_Response* response, struct gaio_Io* ioBody, size_t ioBodySize);
    int (*writeChunkedBodyStart)(struct vinbero_interface_HTTP_Response* response);
    int (*writeChunkedBody)(struct vinbero_interface_HTTP_Response* response, char* stringBody, size_t stringBodySize);
    int (*writeChunkedBodyEnd)(struct vinbero_interface_HTTP_Response* response);
};

#define VINBERO_INTERFACE_HTTP_FUNCTIONS                                                                                                                            \
int vinbero_interface_HTTP_onRequestStart(void* args[]);                                                                                                            \
int vinbero_interface_HTTP_onRequestMethod(char* token, ssize_t tokenSize, void* args[]);                                                                           \
int vinbero_interface_HTTP_onRequestUri(char* token, ssize_t tokenSize, void* args[]);                                                                              \
int vinbero_interface_HTTP_onRequestVersionMajor(int major, void* args[]);                                                                                          \
int vinbero_interface_HTTP_onRequestVersionMinor(int minor, void* args[]);                                                                                          \
int vinbero_interface_HTTP_onRequestScriptPath(char* token, ssize_t tokenSize, void* args[]);                                                                       \
int vinbero_interface_HTTP_onRequestContentType(char* token, ssize_t tokenSize, void* args[]);                                                                      \
int vinbero_interface_HTTP_onRequestContentLength(char* token, ssize_t tokenSize, void* args[]);                                                                    \
int vinbero_interface_HTTP_onRequestHeaderField(char* token, ssize_t tokenSize, void* args[]);                                                                      \
int vinbero_interface_HTTP_onRequestHeaderValue(char* token, ssize_t tokenSize, void* args[]);                                                                      \
int vinbero_interface_HTTP_onRequestHeadersFinish(void* args[]);                                                                                                    \
int vinbero_interface_HTTP_onRequestBodyStart(void* args[]);                                                                                                        \
int vinbero_interface_HTTP_onRequestBody(char* token, ssize_t tokenSize, void* args[]);                                                                             \
int vinbero_interface_HTTP_onRequestBodyFinish(void* args[]);                                                                                                       \
int vinbero_interface_HTTP_onGetRequestContentLength(struct vinbero_common_Module* module, struct vinbero_common_ClData* clData, ssize_t* contentLength);                           \
int vinbero_interface_HTTP_onGetRequestIntHeader(struct vinbero_common_Module* module, struct vinbero_common_ClData* clData, const char* headerField, int* headerValue);            \
int vinbero_interface_HTTP_onGetRequestDoubleHeader(struct vinbero_common_Module* module, struct vinbero_common_ClData* clData, const char* headerField, double* headerValue);      \
int vinbero_interface_HTTP_onGetRequestStringHeader(struct vinbero_common_Module* module, struct vinbero_common_ClData* clData, const char* headerField, const char** headerValue); \
int vinbero_interface_HTTP_onRequestFinish(struct vinbero_common_Module*, struct vinbero_common_ClData*, void* args[])

#define VINBERO_INTERFACE_HTTP_FUNCTION_POINTERS                                                                                 \
int (*vinbero_interface_HTTP_onRequestStart)(void*[]);                                                                           \
int (*vinbero_interface_HTTP_onRequestMethod)(char*, ssize_t, void*[]);                                                          \
int (*vinbero_interface_HTTP_onRequestUri)(char*, ssize_t, void*[]);                                                             \
int (*vinbero_interface_HTTP_onRequestVersionMajor)(int, void*[]);                                                               \
int (*vinbero_interface_HTTP_onRequestVersionMinor)(int, void*[]);                                                               \
int (*vinbero_interface_HTTP_onRequestScriptPath)(char*, ssize_t, void*[]);                                                      \
int (*vinbero_interface_HTTP_onRequestContentType)(char*, ssize_t, void*[]);                                                     \
int (*vinbero_interface_HTTP_onRequestContentLength)(char*, ssize_t, void*[]);                                                   \
int (*vinbero_interface_HTTP_onRequestHeaderField)(char*, ssize_t, void*[]);                                                     \
int (*vinbero_interface_HTTP_onRequestHeaderValue)(char*, ssize_t, void*[]);                                                     \
int (*vinbero_interface_HTTP_onRequestHeadersFinish)(void*[]);                                                                   \
int (*vinbero_interface_HTTP_onRequestBodyStart)(void*[]);                                                                       \
int (*vinbero_interface_HTTP_onRequestBody)(char*, ssize_t, void*[]);                                                            \
int (*vinbero_interface_HTTP_onRequestBodyFinish)(void*[]);                                                                      \
int (*vinbero_interface_HTTP_onGetRequestContentLength)(struct vinbero_common_Module*, struct vinbero_common_ClData*, ssize_t*);                 \
int (*vinbero_interface_HTTP_onGetRequestIntHeader)(struct vinbero_common_Module*, struct vinbero_common_ClData*, const char*, int*);            \
int (*vinbero_interface_HTTP_onGetRequestDoubleHeader)(struct vinbero_common_Module*, struct vinbero_common_ClData*, const char*, double*);      \
int (*vinbero_interface_HTTP_onGetRequestStringHeader)(struct vinbero_common_Module*, struct vinbero_common_ClData*, const char*, const char**); \
int (*vinbero_interface_HTTP_onRequestFinish)(struct vinbero_common_Module*, struct vinbero_common_ClData*, void*[])

struct vinbero_interface_HTTP {
    VINBERO_INTERFACE_HTTP_FUNCTION_POINTERS;
};

#define VINBERO_INTERFACE_HTTP_DLSYM(interface, dlHandle, ret) do { \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestStart, ret); \
    if(*ret < 0) break; \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestMethod, ret); \
    if(*ret < 0) break; \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestUri, ret); \
    if(*ret < 0) break; \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestVersionMajor, ret); \
    if(*ret < 0) break; \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestVersionMinor, ret); \
    if(*ret < 0) break; \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestScriptPath, ret); \
    if(*ret < 0) break; \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestContentType, ret); \
    if(*ret < 0) break; \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestContentLength, ret);  \
    if(*ret < 0) break; \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestHeaderField, ret); \
    if(*ret < 0) break; \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestHeaderValue, ret);      \
    if(*ret < 0) break;                                                                    \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestHeadersFinish, ret);    \
    if(*ret < 0) break;                                                                    \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestBodyStart, ret);        \
    if(*ret < 0) break;                                                                    \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestBody, ret);             \
    if(*ret < 0) break;                                                                    \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestBodyFinish, ret);       \
    if(*ret < 0) break;                                                                    \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onGetRequestContentLength, ret); \
    if(*ret < 0) break;                                                                    \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onGetRequestIntHeader, ret);     \
    if(*ret < 0) break;                                                                    \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onGetRequestDoubleHeader, ret);  \
    if(*ret < 0) break;                                                                    \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onGetRequestStringHeader, ret);  \
    if(*ret < 0) break;                                                                    \
    VINBERO_COMMON_MODULE_DLSYM(interface, dlHandle, vinbero_interface_HTTP_onRequestFinish, ret);           \
    if(*ret < 0) break;                                                                    \
} while(0)

#endif