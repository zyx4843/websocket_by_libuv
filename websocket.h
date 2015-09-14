//
//  websocket.h
//
//  Created by zhengyixiong on 15/8/12.
//  Copyright (c) 2015年 zhengyixiong. All rights reserved.
//
#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__


#include "uv.h"
#include "ws_parser.h"

typedef enum ws_type_enum
{
	WS_SERVER		= 0,		///< websocket服务端
	WS_CLIENT					///< websocket客户端
};

typedef enum ws_state_enum
{
	WS_CLOSE		= 0,		///< 连接关闭
	WS_CONNECTING,				///< 连接中
	WS_CONNECT,					///< 连接上了,还在http握手
	WS_ONLINE,					///< 连接上了,http握手完成,可以收发数据
}ws_state_enum;

typedef enum ws_error
{
    WS_OK = 0,
    WS_ERROR_BUF_FULL = -1000,
    WS_ERROR_BAD,
    WS_ERROR_REONLIN,
    WS_ERROR_NO_ERROR_CB
}WS_ERROR;

typedef struct ws_connect_t ws_connect_t;
typedef int (*ws_cb_connect)(ws_connect_t *ptr, void *obj, int status);
typedef int (*ws_cb_close)(ws_connect_t *ptr, void *obj);
typedef int (*ws_cb_error)(ws_connect_t *ptr, void *obj);
typedef int (*ws_cb_write)(ws_connect_t *ptr, void *obj);
typedef int (*ws_cb_malloc)(ws_connect_t *ptr, void *obj, int suggested_size, uv_buf_t* buf);
typedef int (*ws_cb_read)(ws_connect_t *ptr, void *obj, int nread, OpcodeType type, const uv_buf_t* buf);

typedef struct ws_connect_t
{
	uv_tcp_t			connect;			///< 连接对象
	uv_connect_t		connect_req;		///< 连接请求对象

	uv_write_t			write_req[2];			///< 写请求对象
	uv_buf_t			write_buf[32];		///< 写请求的内存描述
    char                bWrited[2];            ///< 是否写完了

	char				ip[32];				///< 对端ip
	int					port;				///< 对端port

	char                write_buff[256];	///< 缓存
	int					write_len_max;		///< 缓存长度
	int					write_length;		///< 当前有效长度
    
    char                read_buff[256];		///< 缓存
    int					read_len_max;		///< 缓存长度
    int					read_length;		///< 当前有效长度
    WsHeader            read_header;        ///< 读到ws帧的头
    int                 read_dataPos;       ///< 数据起始位置

	int					type;				///< 连接类型, 0.服务端, 1.客户端
	int					state;				///< 连接状态 WS_CLOSE

	struct
	{
		ws_cb_connect	cb_connect;
        ws_cb_error     cb_error;
		void			*obj_connect;

		ws_cb_close		cb_close;
		void			*obj_close;
        

		ws_cb_write		cb_write;
		void			*obj_write;

		ws_cb_malloc	cb_malloc;
		ws_cb_read		cb_read;
		void			*obj_read;
	}cb;
}ws_connect_t;


int ws_init(ws_connect_t *ptr, uv_loop_t *loop);
int ws_quit(ws_connect_t *ptr);

//客户端主动连接及关闭
int ws_connect(ws_connect_t *ptr, char *ip, int port, ws_cb_connect cb, ws_cb_error cb_error, void *cb_obj);
int ws_close(ws_connect_t *ptr, ws_cb_close cb, void *cb_obj);

//连接状态
int ws_state(ws_connect_t *ptr);

//nbufs个数不能超过30,websocket在接口里处理
int ws_write(ws_connect_t *ptr, OpcodeType type, uv_buf_t *bufs, int nbufs, ws_cb_write cb, void *cb_obj);

int ws_read_start(ws_connect_t *ptr, ws_cb_malloc cb_malloc, ws_cb_read cb_read, void *cb_read_obj);
int ws_read_stop(ws_connect_t *ptr);


//////////////////////////////////////////////////////////////////////////
typedef struct ws_server_t ws_server_t;
typedef int (*ws_malloc_connect)(ws_connect_t *&ptr, void *obj);

typedef struct ws_server_t
{
	uv_loop_t			*loop;
	uv_tcp_t			server;
	int					port;

	struct{
        ws_malloc_connect cb_malloc;
        void            *obj_malloc;
        
		ws_cb_connect	cb_connection;
        ws_cb_error     cb_error;
		void			*obj_connection;
	}cb;
}ws_server_t;

//服务端监听,监听到一个要回调上层分配个连接，并设置ws_set_connect
int ws_listen(ws_server_t *ptr, uv_loop_t *loop, int port, ws_malloc_connect cb, void *cb_obj);
int ws_set_connect(ws_connect_t *ptr, ws_cb_connect cb, ws_cb_error cb_error, void *cb_obj);
//ping作为心跳要定时发送
int ws_ping(ws_connect_t *ptr);
uv_loop_t* ws_loop(ws_server_t *ptr);


#endif // __WEBSOCKET_H__
//end
