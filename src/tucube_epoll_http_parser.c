#include <err.h>
#include <stdlib.h>
#include "tucube_epoll_http_parser.h"

int tucube_epoll_http_parser_parse_message_header(struct tucube_epoll_http_parser* parser, ssize_t buffer_size)
{
    parser->buffer_offset = 0;
    parser->token = parser->buffer + parser->buffer_offset;
    parser->token_offset = 0;

    while(parser->buffer_offset < buffer_size)
    {
        switch(parser->state)
        {
        case TUCUBE_EPOLL_HTTP_PARSER_METHOD_BEGIN:
            parser->token = parser->buffer + parser->buffer_offset;
            parser->token_offset = 0;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_METHOD;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_METHOD:
	    if(parser->buffer[parser->buffer_offset] == ' ')
            {
                ++parser->buffer_offset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_METHOD_END;
            }
            else if(parser->buffer[parser->buffer_offset] < 'A' || parser->buffer[parser->buffer_offset] > 'z')
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            else
            {
                ++parser->token_offset;
                ++parser->buffer_offset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_METHOD_END:
            parser->on_method(
                 parser->token,
                 parser->token_offset);
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_URI_BEGIN;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_URI_BEGIN:
            parser->token = parser->buffer + parser->buffer_offset;
            parser->token_offset = 0;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_URI;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_URI:
            if(parser->buffer[parser->buffer_offset] == ' ')
            {
                ++parser->buffer_offset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_URI_END;
            }
            else
            {
                ++parser->token_offset;
                ++parser->buffer_offset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_URI_END:
            parser->on_uri(
                 parser->token,
                 parser->token_offset);
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_VERSION_BEGIN;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_VERSION_BEGIN:
            parser->token = parser->buffer + parser->buffer_offset;
            parser->token_offset = 0;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_VERSION;
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_VERSION:
            if(parser->buffer[parser->buffer_offset] == '\r')
            {
                ++parser->buffer_offset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_VERSION_END;
            }
            else
            {
                ++parser->token_offset;
                ++parser->buffer_offset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_VERSION_END:
            if(parser->buffer[parser->buffer_offset] == '\n')
            {
                ++parser->buffer_offset;
                parser->on_version(
                     parser->token,
                     parser->token_offset);
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_BEGIN;
            }
            else
            {
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_BEGIN:
            if(parser->buffer[parser->buffer_offset] == '\r')
            {
                ++parser->buffer_offset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_END;
            }
            else
            {
                parser->token = parser->buffer + parser->buffer_offset;
                parser->token_offset = 0;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD:
            if(parser->buffer[parser->buffer_offset] == ':')
            {
                ++parser->buffer_offset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_END;
            }
            else
            {
                ++parser->token_offset;
                ++parser->buffer_offset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_END:
            if(parser->buffer[parser->buffer_offset] == ' ')
            {
                ++parser->buffer_offset;
            }
            else
            {
                parser->on_header_field(
                     parser->token,
                     parser->token_offset);
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE_BEGIN;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE_BEGIN:
            parser->token = parser->buffer + parser->buffer_offset;
            parser->token_offset = 0;
            parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE;            
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE:
            if(parser->buffer[parser->buffer_offset] == '\r')
            {
                ++parser->buffer_offset;
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE_END;
            }
            else
            {
                ++parser->token_offset;
                ++parser->buffer_offset;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_VALUE_END:
            if(parser->buffer[parser->buffer_offset] == '\n')
            {
                ++parser->buffer_offset;
                parser->on_header_value(
                     parser->token,
                     parser->token_offset);
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_HEADER_FIELD_BEGIN;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_HEADER_END:
            if(parser->buffer[parser->buffer_offset] == '\n')
            {
                ++parser->buffer_offset;
                return 0;
            }
            else
            {
                parser->state = TUCUBE_EPOLL_HTTP_PARSER_ERROR;
            }
            break;
        case TUCUBE_EPOLL_HTTP_PARSER_ERROR:
            return -1;
            break;
        }
    }
    warnx("%s: %u: Parser needs more characters", __FILE__, __LINE__);
    return 1;
}

int tucube_epoll_http_parser_parse_message_body(struct tucube_epoll_http_parser* parser, ssize_t buffer_size)
{
    return 0;
}

