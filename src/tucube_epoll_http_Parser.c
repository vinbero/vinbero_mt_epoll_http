#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <tucube/tucube_module.h>
#include <tucube/tucube_cldata.h>
#include <libgonc/gonc_cast.h>
#include <libgonc/gonc_nstrncasecmp.h>
#include "tucube_epoll_http.h"
#include "tucube_epoll_http_Parser.h"

char* tucube_epoll_http_Parser_get_buffer_position(struct tucube_epoll_http_Parser* parser)
{
    if(parser->state < TUCUBE_EPOLL_HTTP_PARSER_BODY_BEGIN)
        return parser->buffer + parser->tokenOffset;
    return parser->buffer;
}

size_t tucube_epoll_http_Parser_get_available_bufferSize(struct tucube_epoll_http_Parser* parser)
{
    if(parser->state < TUCUBE_EPOLL_HTTP_PARSER_BODY_BEGIN)
        return parser->headerBufferCapacity - parser->tokenOffset;
    return parser->bodyRemainder;
}

static inline int tucube_epoll_http_Parser_parse_headers(struct tucube_module* module, struct tucube_cldata* cldata, struct tucube_epoll_http_Parser* parser)
{
    parser->bufferOffset = parser->tokenOffset;
    parser->token = parser->buffer;

    while(parser->bufferOffset < parser->bufferSize)
    {
        switch(parser->state)
        {
        case TUCUBE_EPOLL_HTTP_PARSER_HEADERS_BEGIN:
            parser->onRequestStart(GONC_LIST_ELEMENT_NEXT(module), GONC_LIST_ELEMENT_NEXT(cldata));
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_METHOD_BEGIN;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_METHOD_BEGIN:
            parser->tokenOffset = 0;
            parser->token = parser->buffer + parser->bufferOffset;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_METHOD;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_METHOD:
	        if(parser->buffer[parser->bufferOffset] == ' ')
            {
                ++parser->bufferOffset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_METHOD_END;
            }
            else if(parser->buffer[parser->bufferOffset] < 'A' || parser->buffer[parser->bufferOffset] > 'z')
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            else
            {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_METHOD_END:
            if(parser->onRequestMethod(GONC_LIST_ELEMENT_NEXT(module),
                 GONC_LIST_ELEMENT_NEXT(cldata),
                 parser->token,
                 parser->tokenOffset) == -1) {
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            }
            else {
                parser->tokenOffset = 0;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_REQUEST_URI_BEGIN;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_REQUEST_URI_BEGIN:
            parser->token = parser->buffer + parser->bufferOffset;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_REQUEST_URI;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_REQUEST_URI:
            if(parser->buffer[parser->bufferOffset] == ' ')
            {
                ++parser->bufferOffset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_REQUEST_URI_END;
            }
            else
            {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_REQUEST_URI_END:
            if(parser->onRequestUri(GONC_LIST_ELEMENT_NEXT(module),
                 GONC_LIST_ELEMENT_NEXT(cldata),
                 parser->token,
                 parser->tokenOffset) == -1)
            {
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            }
            else
            {
                parser->tokenOffset = 0;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_PROTOCOL_BEGIN;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_PROTOCOL_BEGIN:
            parser->token = parser->buffer + parser->bufferOffset;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_PROTOCOL;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_PROTOCOL:
            if(parser->buffer[parser->bufferOffset] == '\r')
            {
                ++parser->bufferOffset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_PROTOCOL_END;
            }
            else
            {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_PROTOCOL_END:
            if(parser->buffer[parser->bufferOffset] == '\n')
            {
                ++parser->bufferOffset;
                if(parser->onRequestProtocol(GONC_LIST_ELEMENT_NEXT(module),
                      GONC_LIST_ELEMENT_NEXT(cldata),
                      parser->token,
                      parser->tokenOffset) == -1) {
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                }
                else {
                    parser->tokenOffset = 0;
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_BEGIN;
                }
            }
            else
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_BEGIN:
            if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADERS_END;
            }
            else {
                parser->token = parser->buffer + parser->bufferOffset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD:
            if(parser->buffer[parser->bufferOffset] == ':') {
                ++parser->bufferOffset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_END;
            }
            else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_END:
            if(parser->buffer[parser->bufferOffset] == ' ') {
                ++parser->bufferOffset;
            }
            else {
                if(gonc_nstrncasecmp(parser->token, parser->tokenOffset, "X-Script-Name", sizeof("X-Script-Name") - 1) == 0) {
                    parser->tokenOffset = 0;
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_SCRIPT_PATH_BEGIN;
                }
                else if(gonc_nstrncasecmp(parser->token, parser->tokenOffset, "Content-Type", sizeof("Content-Type") - 1) == 0) {
                    parser->tokenOffset = 0;
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_CONTENT_TYPE_BEGIN;
                }
                else if(gonc_nstrncasecmp(parser->token, parser->tokenOffset, "Content-Length", sizeof("Content-Length") - 1) == 0) {
                    parser->tokenOffset = 0;
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_CONTENT_LENGTH_BEGIN;
                }
                else if(parser->onRequestHeaderField(GONC_LIST_ELEMENT_NEXT(module),
                     GONC_LIST_ELEMENT_NEXT(cldata),
                     parser->token,
                     parser->tokenOffset) == -1) {
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                }
                else {
                    parser->tokenOffset = 0;
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE_BEGIN;
                }
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_SCRIPT_PATH_BEGIN:
            parser->token = parser->buffer + parser->bufferOffset;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_SCRIPT_PATH;            
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_SCRIPT_PATH:
            if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_SCRIPT_PATH_END;
            }
            else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_SCRIPT_PATH_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->onRequestScriptPath(GONC_LIST_ELEMENT_NEXT(module),
                     GONC_LIST_ELEMENT_NEXT(cldata),
                     parser->token,
                     parser->tokenOffset) == -1)
                {
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                }
                else
                {
                    parser->tokenOffset = 0;
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_BEGIN;
                }
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_CONTENT_TYPE_BEGIN:
            parser->token = parser->buffer + parser->bufferOffset;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_CONTENT_TYPE;            
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_CONTENT_TYPE:
            if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_CONTENT_TYPE_END;
            }
            else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_CONTENT_TYPE_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->onRequestContentType(GONC_LIST_ELEMENT_NEXT(module),
                     GONC_LIST_ELEMENT_NEXT(cldata),
                     parser->token,
                     parser->tokenOffset) == -1)
                {
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                }
                else
                {
                    parser->tokenOffset = 0;
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_BEGIN;
                }
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_CONTENT_LENGTH_BEGIN:
            parser->token = parser->buffer + parser->bufferOffset;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_CONTENT_LENGTH;            
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_CONTENT_LENGTH:
            if(parser->buffer[parser->bufferOffset] == '\r')
            {
                ++parser->bufferOffset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_CONTENT_LENGTH_END;
            }
            else
            {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
 
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_CONTENT_LENGTH_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->onRequestContentLength(GONC_LIST_ELEMENT_NEXT(module),
                     GONC_LIST_ELEMENT_NEXT(cldata),
                     parser->token,
                     parser->tokenOffset) == -1) {
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                }
                else {
                    parser->tokenOffset = 0;
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_BEGIN;
                }
            }
 
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE_BEGIN:
            parser->token = parser->buffer + parser->bufferOffset;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE;            
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE:
            if(parser->buffer[parser->bufferOffset] == '\r') {
                ++parser->bufferOffset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE_END;
            }
            else {
                ++parser->tokenOffset;
                ++parser->bufferOffset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE_END:
            if(parser->buffer[parser->bufferOffset] == '\n') {
                ++parser->bufferOffset;
                if(parser->onRequestHeaderValue(GONC_LIST_ELEMENT_NEXT(module),
                     GONC_LIST_ELEMENT_NEXT(cldata),
                     parser->token,
                     parser->tokenOffset) == -1) {
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                }
                else {
                    parser->tokenOffset = 0;
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_BEGIN;
                }
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADERS_END:
            if(parser->buffer[parser->bufferOffset] == '\n')
            {
                ++parser->bufferOffset;
                if(parser->onRequestHeadersFinish(GONC_LIST_ELEMENT_NEXT(module), GONC_LIST_ELEMENT_NEXT(cldata)) == -1)
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                else
                {
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_BODY_BEGIN;
                    return 0;
                }
            }
            else
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_ERROR:
            return -1;
            break;
        default:
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            break;
        }
    }

    if(parser->headerBufferCapacity - parser->tokenOffset == 0) {
        warnx("%s: %u: A token is bigger than http_buffer", __FILE__, __LINE__);
        parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
        return -1;
    }

    memmove(parser->buffer, parser->token, parser->tokenOffset * sizeof(char));
    return 1;
}

static inline int tucube_epoll_http_Parser_parse_body(struct tucube_module* module, struct tucube_cldata* cldata, struct tucube_epoll_http_Parser* parser)
{
    int result;
    if(parser->state == TUCUBE_EPOLL_HTTP_PARSER_BODY_BEGIN) {
        if((result = parser->onGetRequestContentLength(GONC_LIST_ELEMENT_NEXT(module), GONC_LIST_ELEMENT_NEXT(cldata), &parser->bodyRemainder)) == -1)
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
        else if(parser->bodyRemainder == 0) {
            if(parser->onRequestFinish(GONC_LIST_ELEMENT_NEXT(module), GONC_LIST_ELEMENT_NEXT(cldata)) == -1) {
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                return -1;
            }
            return 0;
        }
        else {
            if(parser->bufferSize - parser->bufferOffset > parser->bodyBufferCapacity)
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            else {
                memmove(parser->buffer, parser->buffer + parser->bufferOffset, (parser->bufferSize - parser->bufferOffset) * sizeof(char));
                parser->buffer = realloc(parser->buffer, parser->bodyBufferCapacity * sizeof(char));
                parser->bufferSize -= parser->bufferOffset;
                parser->bufferOffset = 0;
                if(parser->onRequestBodyStart(GONC_LIST_ELEMENT_NEXT(module), GONC_LIST_ELEMENT_NEXT(cldata)) == -1) {
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                    return -1;
                }
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_BODY;
            }
        }
    }

    while(parser->bodyRemainder > 0 && parser->bufferSize > 0) {
        switch(parser->state) {
        case TUCUBE_EPOLL_HTTP_PARSER_BODY:
            if(parser->bufferSize < parser->bodyRemainder) {
                if(parser->onRequestBody(GONC_LIST_ELEMENT_NEXT(module), GONC_LIST_ELEMENT_NEXT(cldata), parser->buffer, parser->bufferSize) == -1)
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                else {
                    parser->bodyRemainder -= parser->bufferSize;
                    parser->bufferSize = 0;
                }
            }
            else if(parser->bufferSize >= parser->bodyRemainder)
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_BODY_END;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_BODY_END:
            if(parser->onRequestBody(GONC_LIST_ELEMENT_NEXT(module), GONC_LIST_ELEMENT_NEXT(cldata), parser->buffer, parser->bufferSize) == -1)
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            else {
                parser->bufferSize = 0;
                if(parser->onRequestBodyFinish(GONC_LIST_ELEMENT_NEXT(module), GONC_LIST_ELEMENT_NEXT(cldata)) == -1)
                    parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                else {
                    if(parser->onRequestFinish(GONC_LIST_ELEMENT_NEXT(module), GONC_LIST_ELEMENT_NEXT(cldata)) == -1) {
                        parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
                        return -1;
                    }
                    return 0;
                }
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_ERROR:
            return -1;
        default:
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            break;
        }
    }
    return 1;
}

int tucube_epoll_http_Parser_parse(struct tucube_module* module, struct tucube_cldata* cldata, struct tucube_epoll_http_Parser* parser, ssize_t read_size) {
    parser->bufferSize = parser->tokenOffset + read_size;
    int result;
    if(parser->state < TUCUBE_EPOLL_HTTP_PARSER_BODY_BEGIN) {
        if((result = tucube_epoll_http_Parser_parse_headers(module, cldata, parser)) != 0)
            return result;
    }
    return tucube_epoll_http_Parser_parse_body(module, cldata, parser);
}