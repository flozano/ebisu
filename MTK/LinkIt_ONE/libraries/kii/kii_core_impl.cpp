#include <string.h>
#include <stdarg.h>
#include "kii_core_impl.h"

#include <LTask.h>
#define BUFF_SIZE 256

/* HTTP Callback functions */
kii_http_client_code_t
    request_line_cb(
        kii_http_context_t* http_context,
        const char* method,
        const char* host,
        const char* path)
{
    char* reqBuff;
    context_t* app_context;
    
    app_context = (context_t*)http_context->app_context;
    memset(app_context, 0x00, sizeof(context_t));
    reqBuff = http_context->buffer;
    strncpy(app_context->host, host, strlen(host));
    sprintf(reqBuff, "%s https://%s/%s HTTP/1.1\r\n", method, host, path);

    return KII_HTTPC_OK;
}

kii_http_client_code_t
    header_cb(
        kii_http_context_t* http_context,
        const char* key,
        const char* value)
{
    char* reqBuff = http_context->buffer;
    strcat(reqBuff, key);
    strcat(reqBuff, ":");
    strcat(reqBuff, value);
    strcat(reqBuff, "\r\n");
    return KII_HTTPC_OK;
}

kii_http_client_code_t
    body_cb(
        kii_http_context_t* http_context,
        const char* body_data,
        size_t body_size)
{
    char* reqBuff = http_context->buffer;

    if ((strlen(reqBuff) + 3 + body_size) > http_context->buffer_size)
    {
        return KII_HTTPC_FAIL;
    }

    strcat(reqBuff, "\r\n");
    if (body_data != NULL) {
        strncat(reqBuff, body_data, body_size);
    }
    return KII_HTTPC_OK;
}

kii_http_client_code_t
    execute_cb(
        kii_http_context_t* http_context,
        int* response_code,
        char** response_body)
{
    context_t* ctx;
    kii_socket_code_t sock_res;

    ctx = (context_t *)http_context->app_context;

    switch (ctx->state) {
        case PRV_STATE_IDLE:
            ctx->sent_size = 0;
            ctx->last_chunk = 0;
            ctx->received_size = 0;
            ctx->state = PRV_STATE_CONNECT;
            return KII_HTTPC_AGAIN;
        case PRV_STATE_CONNECT:
            sock_res = http_context->connect_cb(&http_context->socket_context, ctx->host, 80);
            if (sock_res == KII_SOCKETC_OK) {
                ctx->state = PRV_STATE_SEND;
                return KII_HTTPC_AGAIN;
            } else if (sock_res == KII_SOCKETC_AGAIN) {
                return KII_HTTPC_AGAIN;
            } else {
                ctx->state = PRV_STATE_IDLE;
                return KII_HTTPC_FAIL;
            }
        case PRV_STATE_SEND:
        {
            char* sendBuff = NULL;
            int size = BUFF_SIZE;
            int remain = http_context->total_send_size- ctx->sent_size;
            if (remain <= size) {
                size = remain;
                ctx->last_chunk = 1;
            }
            sendBuff = http_context->buffer + ctx->sent_size;
            sock_res = http_context->send_cb(
                    &http_context->socket_context,
                    sendBuff,
                    size);
            if (sock_res == KII_SOCKETC_OK) {
                ctx->sent_size += size;
                if (ctx->last_chunk > 0) {
                    ctx->state = PRV_STATE_RECV;
                }
                return KII_HTTPC_AGAIN;
            } else if(sock_res == KII_SOCKETC_AGAIN) {
                return KII_HTTPC_AGAIN;
            } else {
                ctx->state = PRV_STATE_IDLE;
                return KII_HTTPC_FAIL;
            }
        }
        case PRV_STATE_RECV:
        {
            size_t actualLength = 0;
            char* buffPtr = http_context->buffer + ctx->received_size;
            if (ctx->received_size == 0) {
                memset(http_context->buffer, 0x00, http_context->buffer_size);
            }
            sock_res = http_context->recv_cb(&http_context->socket_context, buffPtr, BUFF_SIZE, &actualLength);
            if (sock_res == KII_SOCKETC_OK) {
                ctx->received_size += actualLength;
                if (actualLength < BUFF_SIZE) {
                    ctx->state = PRV_STATE_CLOSE;
                }
                return KII_HTTPC_AGAIN;
            } else if (sock_res == KII_SOCKETC_AGAIN) {
                return KII_HTTPC_AGAIN;
            } else {
                ctx->state = PRV_STATE_IDLE;
                return KII_HTTPC_FAIL;
            }
        }
        case PRV_STATE_CLOSE:
        {
            sock_res = http_context->close_cb(&http_context->socket_context);
            if (sock_res == KII_SOCKETC_OK) {
                /* parse status code */
                char* statusPtr = strstr(http_context->buffer, "HTTP/1.1 ");
                int numCode = 0;
                char* bodyPtr = NULL;
                if (statusPtr != NULL) {
                    char c_status_code[4];
                    c_status_code[3] = '\0';
                    statusPtr += strlen("HTTP/1.1 ");
                    memcpy(c_status_code, statusPtr, 3);
                    numCode = atoi(c_status_code);
                    *response_code = numCode;
                }
                /* set body pointer */
                bodyPtr = strstr(http_context->buffer, "\r\n\r\n");
                if (bodyPtr != NULL) {
                    bodyPtr += 4;
                }
                *response_body = bodyPtr;
                ctx->state = PRV_STATE_IDLE;
                return KII_HTTPC_OK;
            } else if (sock_res == KII_SOCKETC_AGAIN) {
                return KII_HTTPC_AGAIN;
            } else {
                ctx->state = PRV_STATE_IDLE;
                return KII_HTTPC_FAIL;
            }
        }
    }
    return KII_HTTPC_OK;
}
#if 0
void logger_cb(const char* format, ...)
{
    va_list list;
    va_start(list, format);
    vprintf(format, list);
    va_end(list);
}
#endif
static void kiiHal_itoa(char **buf, int i, int base)
{
	char *s;
#define LEN	20
	int rem;
	static char rev[LEN+1];

	rev[LEN] = 0;
	if (i == 0)
		{
		(*buf)[0] = '0';
		++(*buf);
		return;
		}
	s = &rev[LEN];
	while (i)
		{
		rem = i % base;
		if (rem < 10)
			*--s = rem + '0';
		else if (base == 16)
			*--s = "abcdef"[rem - 10];
		i /= base;
		}
	while (*s)
		{
		(*buf)[0] = *s++;
		++(*buf);
		}
}

void logger_cb(const char *fmt,...)
{
  char print_buf[512*2];
   va_list ap;
   double dval;
   int ival;
   char *p, *sval;
   char *bp, cval;
   int fract;

	bp= print_buf;
	*bp= 0;
	
	va_start (ap, fmt);
	for (p= (char*)fmt; *p; p++)
	{
		if (*p != '%')
		{
			*bp++= *p;
			continue;
		}
		switch (*++p) {
		case 'd':
			ival= va_arg(ap, int);
			if (ival < 0){
				*bp++= '-';
			     ival= -ival;
			}
			kiiHal_itoa (&bp, ival, 10);
			break;
			
        	case 'o':
			ival= va_arg(ap, int);
			if (ival < 0){
				*bp++= '-';
			     ival= -ival;
			}
			*bp++= '0';
			kiiHal_itoa (&bp, ival, 8);
			break;
			
		case 'x':
			ival= va_arg(ap, int);
			if (ival < 0){
			     *bp++= '-';
			     ival= -ival;
			}
			*bp++= '0';
			*bp++= 'x';
			kiiHal_itoa (&bp, ival, 16);
			break;
			
		case 'c':
			cval= va_arg(ap, int);
			*bp++= cval;
			break;
			
		case 's':
			for (sval = va_arg(ap, char *) ; *sval ; sval++ )
			    *bp++= *sval;
			break;
		}
	}
	*bp= 0;
	va_end (ap);
	Serial.println(print_buf);
}

/* vim:set ts=4 sts=4 sw=4 et fenc=UTF-8 ff=unix: */
