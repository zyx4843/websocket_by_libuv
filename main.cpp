//
//  main.cpp
//  nvr
//
//  Created by zhengyixiong on 15/8/12.
//  Copyright (c) 2015年 zhengyixiong. All rights reserved.
//

#include <stdio.h>
#include "websocket.h"

int cb_malloc(ws_connect_t *ptr, void *obj, int suggested_size, uv_buf_t* buf)
{
    buf->base = new char[suggested_size];
    buf->len = suggested_size;
    return 0;
}

int cb_read(ws_connect_t *ptr, void *obj, int nread,  OpcodeType type, const uv_buf_t* buf)
{
    printf(buf->base);
    return 0;
}

int cb_write(ws_connect_t *ptr, void *obj)
{
    printf("write ok\n");
    return 0;
}

static int connect_cb(ws_connect_t *ptr, void *obj, int status)
{
    printf("connect back\n");
    ws_read_start(ptr, cb_malloc, cb_read, NULL);
    
    uv_buf_t bufs2;
    bufs2.base = "hello websocket";
    bufs2.len = strlen(bufs2.base);
    ws_write(ptr, OPCODE_TEXT, &bufs2, 1, cb_write, NULL);
    
    return 0;
}

int cb_error(ws_connect_t *ptr, void *obj)
{
    printf("error\n");
    return 0;
}

static void ping_cb(uv_timer_t* handle)
{
    ws_connect_t *ptr = (ws_connect_t*)(handle->data);
    
    ws_ping(ptr);
}

static int con_cb(ws_connect_t *ptr, void *obj, int status)
{
    printf("accept back\n");
    ws_read_start(ptr, cb_malloc, cb_read, NULL);
    
    uv_buf_t bufs2;
    bufs2.base = "server send";
    bufs2.len = strlen(bufs2.base);
    ws_write(ptr, OPCODE_TEXT, &bufs2, 1, cb_write, NULL);
    
    uv_timer_t *handle = new uv_timer_t;
    handle->data = ptr;
    
    uv_timer_init(uv_default_loop(), handle);
    
    uv_timer_start(handle, ping_cb, 100, 100);
    
    return 0;
}

int malloc_connect(ws_connect_t *&ptr, void *obj)
{
    ptr = new ws_connect_t;
    
    printf("malloc back\n");

	ws_init(ptr, uv_default_loop());
    
    ws_set_connect(ptr, con_cb, cb_error, NULL);
    
    return 0;
}

int main()
{
    ws_server_t server;
    
    ws_listen(&server, uv_default_loop(), 8090, malloc_connect, NULL);
    
    //uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    
    ws_connect_t connect;
    
    ws_init(&connect, uv_default_loop());
    
    ws_connect(&connect, "127.0.0.1", 8090, connect_cb, cb_error, NULL);
    
    printf("adfd");
    
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    
    //ws_quit(&connect);
    
    return 0;
}
