#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <gaio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gon_http_parser.h>
#include <tucube/tucube_Module.h>
#include <tucube/tucube_ClData.h>
#include <libgenc/genc_cast.h>
#include <libgenc/genc_list.h>
#include <libgenc/genc_ltostr.h>
#include <tucube/tucube_IBase.h>
#include <tucube/tucube_ICLocal.h>
#include <gaio.h>
#include "tucube_IHttp.h"

struct tucube_epoll_http_Module {
    TUCUBE_IBASE_FUNCTION_POINTERS;
    TUCUBE_ICLOCAL_FUNCTION_POINTERS;
    TUCUBE_IHTTP_FUNCTION_POINTERS;

    size_t parserHeaderBufferCapacity;
    size_t parserBodyBufferCapacity;
};

struct tucube_epoll_http_ClData {
    struct gaio_Io* clientIo;
    struct tucube_IHttp_Response* clientResponse;
    struct gon_http_parser* parser;
    bool isKeepAlive;
};

TUCUBE_IBASE_FUNCTIONS;
TUCUBE_ICLOCAL_FUNCTIONS;

int tucube_IBase_init(struct tucube_Module_Config* moduleConfig, struct tucube_Module_List* moduleList, void* args[]) {
#define TUCUBE_LOCAL_MODULE GENC_CAST(module->generic.pointer, struct tucube_epoll_http_Module*)
    warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    if(GENC_LIST_ELEMENT_NEXT(moduleConfig) == NULL)
        errx(EXIT_FAILURE, "tucube_epoll_http requires another module");

    struct tucube_Module* module = malloc(1 * sizeof(struct tucube_Module));
    GENC_LIST_ELEMENT_INIT(module);
    module->generic.pointer = malloc(1 * sizeof(struct tucube_epoll_http_Module));

    TUCUBE_MODULE_DLOPEN(module, moduleConfig);
    TUCUBE_IBASE_DLSYM(module, struct tucube_epoll_http_Module);
    TUCUBE_ICLOCAL_DLSYM(module, struct tucube_epoll_http_Module);
    TUCUBE_IHTTP_DLSYM(module, struct tucube_epoll_http_Module);

    TUCUBE_LOCAL_MODULE->parserHeaderBufferCapacity = 1024;

    if(json_object_get(json_array_get(moduleConfig->json, 1), "tucube_epoll_http.parserHeaderBufferCapacity") != NULL)
        TUCUBE_LOCAL_MODULE->parserHeaderBufferCapacity = json_integer_value(json_object_get(json_array_get(moduleConfig->json, 1), "tucube_epoll_http.parserHeaderBufferCapacity"));

    TUCUBE_LOCAL_MODULE->parserBodyBufferCapacity = 1048576;

    if(json_object_get(json_array_get(moduleConfig->json, 1), "tucube_epoll_http.parserBodyBufferCapacity") != NULL)
        TUCUBE_LOCAL_MODULE->parserBodyBufferCapacity = json_integer_value(json_object_get(json_array_get(moduleConfig->json, 1), "tucube_epoll_http.parserBodyBufferCapacity"));

    GENC_LIST_APPEND(moduleList, module);

    if(TUCUBE_LOCAL_MODULE->tucube_IBase_init(GENC_LIST_ELEMENT_NEXT(moduleConfig), moduleList, (void*[]){NULL}) == -1)
        errx(EXIT_FAILURE, "%s: %u: tucube_IBase_init() failed", __FILE__, __LINE__);

    return 0;
}

int tucube_IBase_tlInit(struct tucube_Module* module, struct tucube_Module_Config* moduleConfig, void* args[]) {
#define TUCUBE_LOCAL_MODULE GENC_CAST(module->generic.pointer, struct tucube_epoll_http_Module*)
    warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    TUCUBE_LOCAL_MODULE->tucube_IBase_tlInit(GENC_LIST_ELEMENT_NEXT(module), GENC_LIST_ELEMENT_NEXT(moduleConfig), (void*[]){NULL});
#undef TUCUBE_LOCAL_MODULE
    return 0;
}

static int tucube_epoll_http_writeBytes(struct tucube_IHttp_Response* response, char* bytes, size_t bytesSize) {
    response->io->write(response->io, bytes, bytesSize);
    return 0;
}

static int tucube_epoll_http_writeIo(struct tucube_IHttp_Response* response, struct gaio_Io* io, size_t bytesSize) {
    response->io->sendfile(response->io, io, NULL, bytesSize);
    return 0;
}

static int tucube_epoll_http_writeCrLf(struct tucube_IHttp_Response* response) {
    response->io->write(response->io, "\r\n", sizeof("\r\n") - 1);
    return 0;
}
static int tucube_epoll_http_writeVersion(struct tucube_IHttp_Response* response, int major, int minor) {
    response->io->write(response->io, "HTTP/", sizeof("HTTP/") - 1);

    char* majorString;
    size_t majorStringSize;
    majorStringSize = genc_ltostr(major, 10, &majorString);
    response->io->write(response->io, majorString, majorStringSize);
    free(majorString);

    response->io->write(response->io, ".", sizeof(".") - 1);

    char* minorString;
    size_t minorStringSize;
    minorStringSize = genc_ltostr(minor, 10, &minorString);
    response->io->write(response->io, minorString, minorStringSize);
    free(minorString);

    response->io->write(response->io, " ", sizeof(" ") - 1);

    return 0;
}
static int tucube_epoll_http_writeStatusCode(struct tucube_IHttp_Response* response, int statusCode) {
    char* statusCodeString;
    size_t statusCodeStringSize;
    statusCodeStringSize = genc_ltostr(statusCode, 10, &statusCodeString);
    response->io->write(response->io, statusCodeString, statusCodeStringSize);
    free(statusCodeString);

    response->io->write(response->io, " \r\n", sizeof(" \r\n") - 1);

    return 0;
}
static int tucube_epoll_http_writeIntHeader(struct tucube_IHttp_Response* response, char* headerField, size_t headerFieldSize, int headerValue) {
    response->io->write(response->io, headerField, headerFieldSize);

    response->io->write(response->io, ": ", sizeof(": ") - 1);

    char* headerValueString;
    size_t headerValueStringSize;
    headerValueStringSize = genc_ltostr(headerValue, 10, &headerValueString);
    response->io->write(response->io, headerValueString, headerValueStringSize);
    free(headerValueString);

    response->io->write(response->io, "\r\n", sizeof("\r\n") - 1);

    return 0;
}
static int tucube_epoll_http_writeDoubleHeader(struct tucube_IHttp_Response* response, char* headerField, size_t headerFieldSize, double headerValue) {
    // not implemented yet
    return 0;
}
static int tucube_epoll_http_writeStringHeader(struct tucube_IHttp_Response* response, char* headerField, size_t headerFieldSize, char* headerValue, size_t headerValueSize) {
    response->io->write(response->io, headerField, headerFieldSize);
    response->io->write(response->io, ": ", sizeof(": ") - 1);
    response->io->write(response->io, headerValue, headerValueSize);
    response->io->write(response->io, "\r\n", sizeof("\r\n") - 1);

    return 0;
}

int tucube_ICLocal_init(struct tucube_Module* module, struct tucube_ClData_List* clDataList, void* args[]) {
#define TUCUBE_LOCAL_MODULE GENC_CAST(module->generic.pointer, struct tucube_epoll_http_Module*)
#define TUCUBE_LOCAL_CLDATA GENC_CAST(clData->generic.pointer, struct tucube_epoll_http_ClData*)
#define TUCUBE_LOCAL_PARSER GENC_CAST(clData->generic.pointer, struct tucube_epoll_http_ClData*)->parser
    warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    struct tucube_ClData* clData = malloc(1 * sizeof(struct tucube_ClData));
    GENC_LIST_ELEMENT_INIT(clData);
    clData->generic.pointer = malloc(1 * sizeof(struct tucube_epoll_http_ClData));

    TUCUBE_LOCAL_CLDATA->clientIo = ((struct gaio_Io*)args[0]);

    TUCUBE_LOCAL_CLDATA->isKeepAlive = false;

    TUCUBE_LOCAL_PARSER = malloc(1 * sizeof(struct gon_http_parser));
    gon_http_parser_init(
        TUCUBE_LOCAL_PARSER,
        TUCUBE_LOCAL_MODULE->parserHeaderBufferCapacity,
        TUCUBE_LOCAL_MODULE->parserBodyBufferCapacity
    );
    TUCUBE_LOCAL_PARSER->onRequestStart = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestStart;
    TUCUBE_LOCAL_PARSER->onRequestMethod = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestMethod;
    TUCUBE_LOCAL_PARSER->onRequestUri = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestUri;
    TUCUBE_LOCAL_PARSER->onRequestProtocol = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestProtocol;
    TUCUBE_LOCAL_PARSER->onRequestScriptPath = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestScriptPath;
    TUCUBE_LOCAL_PARSER->onRequestContentType = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestContentType;
    TUCUBE_LOCAL_PARSER->onRequestContentLength = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestContentLength;

    TUCUBE_LOCAL_PARSER->onRequestHeaderField = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestHeaderField;
    TUCUBE_LOCAL_PARSER->onRequestHeaderValue = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestHeaderValue;
    TUCUBE_LOCAL_PARSER->onRequestHeadersFinish = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestHeadersFinish;
    TUCUBE_LOCAL_PARSER->onRequestBodyStart = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestBodyStart;
    TUCUBE_LOCAL_PARSER->onRequestBody = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestBody;
    TUCUBE_LOCAL_PARSER->onRequestBodyFinish = TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestBodyFinish;

    GENC_LIST_APPEND(clDataList, clData);

    TUCUBE_LOCAL_CLDATA->clientResponse = malloc(sizeof(struct tucube_IHttp_Response));
    TUCUBE_LOCAL_CLDATA->clientResponse->callbacks = malloc(sizeof(struct tucube_IHttp_Response_Callbacks));
    TUCUBE_LOCAL_CLDATA->clientResponse->io = TUCUBE_LOCAL_CLDATA->clientIo;
    TUCUBE_LOCAL_CLDATA->clientResponse->callbacks->writeBytes = tucube_epoll_http_writeBytes;
    TUCUBE_LOCAL_CLDATA->clientResponse->callbacks->writeIo = tucube_epoll_http_writeIo;
    TUCUBE_LOCAL_CLDATA->clientResponse->callbacks->writeCrLf = tucube_epoll_http_writeCrLf;
    TUCUBE_LOCAL_CLDATA->clientResponse->callbacks->writeVersion = tucube_epoll_http_writeVersion;
    TUCUBE_LOCAL_CLDATA->clientResponse->callbacks->writeStatusCode = tucube_epoll_http_writeStatusCode;
    TUCUBE_LOCAL_CLDATA->clientResponse->callbacks->writeIntHeader = tucube_epoll_http_writeIntHeader;
    TUCUBE_LOCAL_CLDATA->clientResponse->callbacks->writeDoubleHeader = tucube_epoll_http_writeDoubleHeader;
    TUCUBE_LOCAL_CLDATA->clientResponse->callbacks->writeStringHeader = tucube_epoll_http_writeStringHeader;

    TUCUBE_LOCAL_MODULE->tucube_ICLocal_init(GENC_LIST_ELEMENT_NEXT(module), clDataList, (void*[]){TUCUBE_LOCAL_CLDATA->clientResponse, NULL});
    return 0;
#undef TUCUBE_LOCAL_MODULE
#undef TUCUBE_LOCAL_CLDATA
#undef TUCUBE_LOCAL_PARSER
}
static inline int tucube_epoll_http_readRequest(struct tucube_Module* module, struct tucube_ClData* clData) {
#define TUCUBE_LOCAL_MODULE GENC_CAST(module->generic.pointer, struct tucube_epoll_http_Module*)
#define TUCUBE_LOCAL_CLDATA GENC_CAST(clData->generic.pointer, struct tucube_epoll_http_ClData*)
#define TUCUBE_LOCAL_PARSER GENC_CAST(clData->generic.pointer, struct tucube_epoll_http_ClData*)->parser
#define TUCUBE_LOCAL_CLIENT_IO GENC_CAST(clData->generic.pointer, struct tucube_epoll_http_ClData*)->clientIo
    warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    ssize_t readSize;
    while((readSize = TUCUBE_LOCAL_CLIENT_IO->read(
              TUCUBE_LOCAL_CLIENT_IO,
              gon_http_parser_getBufferPosition(TUCUBE_LOCAL_PARSER),
              gon_http_parser_getAvailableBufferSize(TUCUBE_LOCAL_PARSER)
           )) > 0) {
        int result;
        if((result = gon_http_parser_parse(TUCUBE_LOCAL_PARSER, readSize, (void*[]){GENC_LIST_ELEMENT_NEXT(module), GENC_LIST_ELEMENT_NEXT(clData)})) == -1) {
            warnx("%s: %u: Parser error", __FILE__, __LINE__);
            return -1;
        } else if(result == 0)
            break;
    }
    if(readSize == -1) {
        if(errno == EAGAIN) {
//            warnx("%s: %u: Client socket EAGAIN", __FILE__, __LINE__);
            return 1;
        } else if(errno == EWOULDBLOCK) {
            warnx("%s: %u: Client socket EWOULDBLOCK", __FILE__, __LINE__);
            return 1;
        } else
            warn("%s: %u", __FILE__, __LINE__);
        return -1;
    }
    else if(readSize == 0) {
        warnx("%s: %u: Client socket has been closed", __FILE__, __LINE__);
        return -1;
    }
    
    const char* connectionHeaderValue;
    if(TUCUBE_LOCAL_MODULE->tucube_IHttp_onGetRequestStringHeader(
              GENC_LIST_ELEMENT_NEXT(module), GENC_LIST_ELEMENT_NEXT(clData), "Connection", &connectionHeaderValue
      ) != -1) {
        if(strncasecmp(connectionHeaderValue, "Keep-Alive", sizeof("Keep-Alive")) == 0) {
            warnx("%s: %u: Keep-Alive Connection", __FILE__, __LINE__);
            TUCUBE_LOCAL_CLDATA->isKeepAlive = false;
            gon_http_parser_reset(TUCUBE_LOCAL_PARSER);
            return 2; // request finished but this is keep-alive
        }
        TUCUBE_LOCAL_CLDATA->isKeepAlive = false;
    }
    return 0; // request finsihed
#undef TUCUBE_LOCAL_MODULE
#undef TUCUBE_LOCAL_CLDATA
#undef TUCUBE_LOCAL_PARSER
}

int tucube_IClService_call(struct tucube_Module* module, struct tucube_ClData* clData, void* args[]) {
#define TUCUBE_LOCAL_MODULE GENC_CAST(module->generic.pointer, struct tucube_epoll_http_Module*)
#define TUCUBE_LOCAL_CLIENT_IO GENC_CAST(clData->generic.pointer, struct tucube_epoll_http_ClData*)->clientIo
    warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    register int result = tucube_epoll_http_readRequest(module, clData);
    if(result != 0 && result != 2)
        return result;
    if(TUCUBE_LOCAL_CLIENT_IO->fcntl(TUCUBE_LOCAL_CLIENT_IO, F_SETFL, TUCUBE_LOCAL_CLIENT_IO->fcntl(TUCUBE_LOCAL_CLIENT_IO, F_GETFL, 0) & ~O_NONBLOCK) == -1)
        return -1;
    if(TUCUBE_LOCAL_MODULE->tucube_IHttp_onRequestFinish(GENC_LIST_ELEMENT_NEXT(module), GENC_LIST_ELEMENT_NEXT(clData), args) == -1)
        return -1;
    if(TUCUBE_LOCAL_CLIENT_IO->fcntl(TUCUBE_LOCAL_CLIENT_IO, F_SETFL, TUCUBE_LOCAL_CLIENT_IO->fcntl(TUCUBE_LOCAL_CLIENT_IO, F_GETFL, 0) | O_NONBLOCK) == -1)
        return -1;
    if(result == 2)
        return 1;

    return 0;
#undef TUCUBE_LOCAL_CLIENT_IO
#undef TUCUBE_LOCAL_MODULE
}

int tucube_ICLocal_destroy(struct tucube_Module* module, struct tucube_ClData* clData) {
#define TUCUBE_LOCAL_MODULE GENC_CAST(module->generic.pointer, struct tucube_epoll_http_Module*)
#define TUCUBE_LOCAL_CLDATA GENC_CAST(clData->generic.pointer, struct tucube_epoll_http_ClData*)
#define TUCUBE_LOCAL_PARSER GENC_CAST(clData->generic.pointer, struct tucube_epoll_http_ClData*)->parser
    warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    TUCUBE_LOCAL_MODULE->tucube_ICLocal_destroy(GENC_LIST_ELEMENT_NEXT(module), GENC_LIST_ELEMENT_NEXT(clData));
    free(TUCUBE_LOCAL_CLDATA->clientResponse->callbacks);
    free(TUCUBE_LOCAL_CLDATA->clientResponse);
    free(TUCUBE_LOCAL_PARSER->buffer);
    free(TUCUBE_LOCAL_PARSER);
    free(clData->generic.pointer);
    free(clData);
    return 0;
#undef TUCUBE_LOCAL_PARSER
#undef TUCUBE_LOCAL_CLDATA
#undef TUCUBE_LOCAL_MODULE
}

int tucube_IBase_tlDestroy(struct tucube_Module* module) {
#define TUCUBE_LOCAL_MODULE GENC_CAST(module->generic.pointer, struct tucube_epoll_http_Module*)
    warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    TUCUBE_LOCAL_MODULE->tucube_IBase_tlDestroy(GENC_LIST_ELEMENT_NEXT(module));
    return 0;
#undef TUCUBE_LOCAL_MODULE
}

int tucube_IBase_destroy(struct tucube_Module* module) {
#define TUCUBE_LOCAL_MODULE GENC_CAST(module->generic.pointer, struct tucube_epoll_http_Module*)
    warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    TUCUBE_LOCAL_MODULE->tucube_IBase_destroy(GENC_LIST_ELEMENT_NEXT(module));
//    dlclose(module->dl_handle);
    free(module->generic.pointer);
    free(module);
    return 0;
#undef TUCUBE_LOCAL_MODULE
}
