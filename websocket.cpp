#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "websocket.h"
#include "ws_parser.h"

#define print_info() printf("-> ptr=0x%p.(%s,%d)\n", ptr, __FILE__, __LINE__)

static char str_client_req[] = "\
GET / HTTP/1.1\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Version: 13\r\n\
Sec-WebSocket-Key: qPDDeYhyaIhjCtefxxR4/Q==\r\n\
Host: 192.168.8.129:3457\r\n\
Sec-WebSocket-Protocol: zyx_protocol\r\n\
\r\n\
";


static void cb_pre_malloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
static void cb_read_over(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
static void cb_write(uv_write_t* req, int status);


int ws_init(ws_connect_t *ptr, uv_loop_t *loop)
{
	memset(ptr, 0, sizeof(ws_connect_t));
	uv_tcp_init(loop, &ptr->connect);

	ptr->connect.data = ptr;
	ptr->connect_req.data = ptr;
	ptr->write_req[0].data = ptr;
    ptr->write_req[1].data = ptr;
    ptr->bWrited[0] = 0;
    ptr->bWrited[1] = 0;

	ptr->write_len_max = 256;
	ptr->write_length = 0;
    ptr->read_len_max = 256;
    ptr->read_length = 0;
    
    memset(&(ptr->read_header), 0, sizeof(WsHeader));

	//print_info();

	return 0;
}

int ws_quit(ws_connect_t *ptr)
{
    ptr->write_len_max = 0;
    ptr->write_length = 0;
    ptr->read_len_max = 0;
    ptr->read_length = 0;

	return 0;
}

static void on_connect(ws_connect_t *ptr)
{
	//print_info();

	ptr->state = WS_CONNECT;

	uv_read_start((uv_stream_t *)&ptr->connect, cb_pre_malloc, cb_read_over);

	if(WS_CLIENT == ptr->type)
	{
		// step1 websocket握手

		ptr->write_buf[0] = uv_buf_init(str_client_req, strlen(str_client_req));

		uv_write(&(ptr->write_req[0]), (uv_stream_t*)&ptr->connect, ptr->write_buf, 1, cb_write);
	}
    else
    {
        
    }
}

static void cb_connect(uv_connect_t* req, int status)
{
	ws_connect_t *ptr = (ws_connect_t*)req->data;

	//print_info();

	on_connect(ptr);
}

int ws_connect(ws_connect_t *ptr, char *ip, int port, ws_cb_connect cb, ws_cb_error cb_error, void *cb_obj)
{
	ptr->type = WS_CLIENT;

	strncpy(ptr->ip, ip, sizeof(ptr->ip));
	ptr->port = port;

	struct sockaddr_in addr;
	uv_ip4_addr(ptr->ip, ptr->port, &addr);

	ptr->cb.cb_connect = cb;
    ptr->cb.cb_error = cb_error;
	ptr->cb.obj_connect = cb_obj;

	ptr->state = WS_CONNECTING;

	uv_tcp_connect( &ptr->connect_req,
					&ptr->connect,
					(const struct sockaddr*) &addr,
					cb_connect);

	return 0;
}

static void cb_close(uv_handle_t* handle)
{
	ws_connect_t *ptr = (ws_connect_t*)handle->data;

	//print_info();

	if(NULL != ptr->cb.cb_close)
	{
		ptr->cb.cb_close(ptr, ptr->cb.obj_close);
	}

	// 清空其他状态
    ptr->write_length = 0;
    ptr->read_length = 0;
	ptr->type = WS_SERVER;
	
	memset(&(ptr->cb), 0, sizeof(ptr->cb));
}

int ws_close(ws_connect_t *ptr, ws_cb_close cb, void *cb_obj)
{
	ptr->state = WS_CLOSE;
	ptr->cb.cb_close = cb;
	ptr->cb.obj_close = cb_obj;

	uv_close((uv_handle_t*)&ptr->connect, cb_close);

	return 0;
}

int ws_state(ws_connect_t *ptr)
{
	return ptr->state;
}

static void cb_write(uv_write_t* req, int status)
{
	ws_connect_t *ptr = (ws_connect_t*)req->data;

	//print_info();

	if(0 != status)
	{
		//uv_strerror(status);
	}
    
    if(WS_SERVER == ptr->type && WS_ONLINE != ptr->state)
    {
        ptr->state = WS_ONLINE;
        ptr->cb.cb_connect(ptr, ptr->cb.obj_connect, WS_OK);
    }
    else if(NULL != ptr->cb.cb_write)
    {
        ptr->cb.cb_write(ptr, ptr->cb.obj_write);
        ptr->bWrited[0] = 0;
    }

	//uv_close((uv_handle_t*)req->handle, close_cb);
}

int ws_write(ws_connect_t *ptr, OpcodeType type, uv_buf_t *bufs, int nbufs, ws_cb_write cb, void *cb_obj)
{
    if (WS_ONLINE == ptr->state)
    {
        if (0 != ptr->bWrited)
        {
            return 2;
        }
        
        memcpy(&(ptr->write_buf[1]), bufs, nbufs * sizeof(uv_buf_t));

        ptr->cb.cb_write = cb;
        ptr->cb.obj_write = cb_obj;

        uint64_t dataSize = 0;
        for (int i = 0; i < nbufs; ++i)
        {
            dataSize += bufs[i].len;
        }
        
        ptr->write_length = ptr->write_len_max;
        Pack(type, ptr->write_buff, ptr->write_length, dataSize);
        
        ptr->write_buf[0].base = ptr->write_buff;
        ptr->write_buf[0].len = ptr->write_length;
        
        uv_write(&(ptr->write_req[0]), (uv_stream_t*)&ptr->connect, ptr->write_buf, nbufs + 1, cb_write);
        
        ptr->bWrited[0] = 1;
    }
    else
    {
        return 1;
    }

	return 0;
}

int ws_write_multi(ws_connect_t *ptr, uv_write_t *req, uv_buf_t *bufs, int nbufs, uv_write_cb cb)
{
	uv_write(req, (uv_stream_t*)&ptr->connect, bufs, nbufs, cb);

	return 0;
}

static void cb_pre_malloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	ws_connect_t *ptr = (ws_connect_t*)(handle->data);

	buf->base = (char*)(ptr->read_buff + ptr->read_length);
	buf->len = ptr->read_len_max - ptr->read_length;
}

static void cb_read_over(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
	ws_connect_t *ptr = (ws_connect_t*)(stream->data);

	if(nread >= 0)
	{
        ptr->read_length += nread;

		//print_info();
        
        if (WS_CONNECT == ptr->state)
        {
            char *pHeader = strstr(ptr->read_buff, "\r\n\r\n");
            if (NULL == pHeader)
            {
                //没找到http尾
                if (ptr->read_length >= ptr->read_len_max) {
                    ptr->read_length = 0;
                    //出错
                    /*if (WS_CLIENT == ptr->type)
                    {
                        ptr->cb.cb_connect(ptr, ptr->cb.obj_connect, WS_ERROR_BUF_FULL);
                    }
                    else*/
                    {
                        //服务器处理
                        ptr->cb.cb_connect(ptr, ptr->cb.obj_connect, WS_ERROR_BUF_FULL);
                    }
                }
                return;
            }
            
            //握手协议
            if (WS_CLIENT == ptr->type)
            {
                uv_read_stop((uv_stream_t *)&ptr->connect);
                //
                if(NULL != strstr(ptr->read_buff, "HTTP/1.1 101"))
                {
                    ptr->state = WS_ONLINE;
                    ptr->cb.cb_connect(ptr, ptr->cb.obj_connect, WS_OK);
                    
                    //发个心跳
                    /*uv_buf_t bufs;
                    ptr->write_buf[0].base = new char[2];
                    ptr->write_buf[0].len = 2;
                    ptr->write_buf[0].base[0] = 0x89;
                    ptr->write_buf[0].base[1] = 0;
                    uv_write(&ptr->write_req, (uv_stream_t*)&ptr->connect, ptr->write_buf, 1, cb_write);*/
                }
                else
                {
                    ptr->cb.cb_connect(ptr, ptr->cb.obj_connect, WS_ERROR_BAD);
                }
                //int len = pHeader - ptr->read_buff + 4;
                //memmove(ptr->read_buff, pHeader + 4, ptr->read_length - len);
                //ptr->read_length = ptr->read_length - len;
                ptr->read_length = 0;
            }
            else
            {
                uv_read_stop((uv_stream_t *)&ptr->connect);
                
                int reslen = ptr->write_len_max;
                
                ParseHandShake(ptr->read_buff, ptr->read_length, ptr->write_buff, ptr->write_len_max, reslen);
                
                ptr->write_buf[0] = uv_buf_init(ptr->write_buff, reslen);
                
                uv_write(&(ptr->write_req[0]), (uv_stream_t*)&ptr->connect, ptr->write_buf, 1, cb_write);
                
                //int len = pHeader - ptr->read_buff + 4;
                //memmove(ptr->read_buff, pHeader + 4, ptr->read_length - len);
                //ptr->read_length = ptr->read_length - len;
                ptr->read_length = 0;
            }
        }
        else
        {
            //状态不对
            ptr->cb.cb_connect(ptr, ptr->cb.obj_connect, WS_ERROR_REONLIN);
        }
	}
	else if(nread == UV_ENOBUFS)
	{
		//当缓存不足时, 会回调这个
	}
	else
	{
        ptr->cb.cb_connect(ptr, ptr->cb.obj_connect, WS_ERROR_BAD);
		// 关闭连接
		printf("->strerror:%s,(%s,%d)\n",uv_strerror(nread), __FILE__, __LINE__);
	}
}

static void cb_pong(uv_write_t* req, int status)
{
    ws_connect_t *ptr = (ws_connect_t*)req->data;
    ptr->bWrited[1] = 0;
}

static void cb_read_malloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    ws_connect_t *ptr = (ws_connect_t*)(handle->data);
    
    if (NULL == ptr->cb.cb_malloc)
    {
        printf("->strerror:read malloc is null,(%s,%d)\n", __FILE__, __LINE__);
        return;
    }
    
    if (0 < ptr->read_header.length)
    {
        uv_buf_t wsbuf;
        ptr->cb.cb_malloc(ptr, ptr->cb.obj_read, ptr->read_header.length, &wsbuf);
        memcpy(wsbuf.base, ptr->read_buff, ptr->read_length);
        buf->base = wsbuf.base + ptr->read_length;
        buf->len = wsbuf.len - ptr->read_length;
    }
    else if(0 < ptr->read_length)
    {
        buf->base = ptr->read_buff + ptr->read_length;
        buf->len = ptr->read_len_max - ptr->read_length;
    }
    else
    {
        buf->base = ptr->read_buff;
        buf->len = ptr->read_len_max;
    }
}

static void cb_read_operation(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    ws_connect_t *ptr = (ws_connect_t*)(stream->data);
    
    if (NULL == ptr->cb.cb_read)
    {
        printf("->strerror:read callback is null,(%s,%d)\n", __FILE__, __LINE__);
        return;
    }
    
    if(nread >= 0)
    {
        if (WS_ONLINE == ptr->state)
        {
            if (0 < ptr->read_header.length)
            {
                uv_buf_t wsbuf;
                ptr->cb.cb_malloc(ptr, ptr->cb.obj_read, ptr->read_header.length, &wsbuf);
                memcpy(wsbuf.base, ptr->read_buff, ptr->read_length);
                wsbuf.base = buf->base - ptr->read_length;
                wsbuf.len = nread + ptr->read_length;
                ptr->cb.cb_read(ptr, ptr->cb.obj_read, ptr->read_header.length, (OpcodeType)ptr->read_header.type, &wsbuf);
                ptr->read_header.length = 0;
                ptr->read_length = 0;
            }
            else
            {
                ptr->read_length += nread;
                int read_len = 0;
                int ret = 0;
                do
                {
                    ptr->read_dataPos = 0;
                    ret = Parse(ptr->read_buff + read_len, ptr->read_length - read_len, ptr->read_header, ptr->read_dataPos);
                    if(0 == ret)
                    {
                        if (OPCODE_PING == ptr->read_header.type)
                        {
                            //回复Pong
                            ptr->write_buf[31].base = ptr->write_buff;
                            ptr->write_buf[31].len = 2;
                            ptr->write_buf[31].base[0] = 0x8a;
                            ptr->write_buf[31].base[1] = 0;
                            uv_write(&(ptr->write_req[1]), (uv_stream_t*)&ptr->connect, &(ptr->write_buf[31]), 1, cb_pong);
                            ptr->bWrited[1] = 1;
                            
                            read_len += ptr->read_header.length + ptr->read_dataPos;
                            ptr->read_header.length = 0;
                        }
                        else if(OPCODE_PONG == ptr->read_header.type)
                        {
                            read_len += ptr->read_header.length + ptr->read_dataPos;
                            ptr->read_header.length = 0;
                        }
                        else if(read_len + ptr->read_header.length + ptr->read_dataPos <= ptr->read_length)
                        {
                            //有完整帧
                            uv_buf_t wsBuf;
                            wsBuf.base = ptr->read_buff - ptr->read_header.length;
                            wsBuf.len = ptr->read_header.length;
                            ptr->cb.cb_read(ptr, ptr->cb.obj_read, ptr->read_header.length, (OpcodeType)ptr->read_header.type, &wsBuf);
                            
                            read_len += ptr->read_header.length + ptr->read_dataPos;
                            ptr->read_header.length = 0;
                        }
                        else
                        {
                            //只够头
                            memmove(ptr->read_buff, ptr->read_buff + read_len + ptr->read_dataPos, ptr->read_length  - read_len - ptr->read_dataPos);
                            
                            read_len += ptr->read_dataPos;
                            break;
                        }
                    }
                    else if(2 == ret)
                    {
                        memmove(ptr->read_buff, ptr->read_buff + read_len, ptr->read_length - read_len);
                        ptr->read_header.length = 0;
                    }
                    else
                    {
                        printf("->strerror:parser ws header error,(%s,%d)\n", __FILE__, __LINE__);
                    }
                    
                }while( (0 == ret) && (read_len < ptr->read_length) );
                ptr->read_length -= read_len;
            }
        }
        else
        {
            printf("->strerror:status is error to read,(%s,%d)\n", __FILE__, __LINE__);
        }
    }
    else if(nread == UV_ENOBUFS)
    {
        //当缓存不足时, 会回调这个
        printf("->strerror:read malloc error,(%s,%d)\n", __FILE__, __LINE__);
    }
    else
    {
        if (NULL != ptr->cb.cb_error)
        {
            ptr->cb.cb_error(ptr, ptr->cb.obj_connect);
        }
        // 关闭连接
        printf("->strerror:%s,(%s,%d)\n",uv_strerror(nread), __FILE__, __LINE__);
    }
}

int ws_read_start(ws_connect_t *ptr, ws_cb_malloc cb_malloc, ws_cb_read cb_read, void *cb_read_obj)
{
    if (WS_ONLINE == ptr->state)
    {
        ptr->cb.cb_malloc = cb_malloc;
        
        ptr->cb.cb_read = cb_read;
        ptr->cb.obj_read = cb_read_obj;
        
        uv_read_start((uv_stream_t *)&ptr->connect, cb_read_malloc, cb_read_operation);
    }
	else
    {
        return 1;
    }
    
	return 0;
}

int ws_read_stop(ws_connect_t *ptr)
{
    uv_read_stop((uv_stream_t *)&ptr->connect);
    
	ptr->cb.cb_malloc = NULL;

	ptr->cb.cb_read = NULL;
	ptr->cb.obj_read = NULL;

	return 0;
}


//////////////////////////////////////////////////////////////////////////

static void cb_connection(uv_stream_t *stream, int status)
{
	ws_server_t *ptr = (ws_server_t*)stream->data;

	ws_connect_t *pConnect = NULL;

    ptr->cb.cb_malloc(pConnect, ptr->cb.obj_malloc);
    
    if (NULL == pConnect)
    {
        printf("->strerror:malloc connect error,(%s,%d)\n", __FILE__, __LINE__);
        return;
    }
    
	if(0 == uv_accept((uv_stream_t *)(&ptr->server), (uv_stream_t*)(&pConnect->connect)))
	{
        pConnect->state = WS_CONNECTING;
		on_connect(pConnect);
	}
    else
    {
        if (NULL != ptr->cb.cb_connection)
        {
            ptr->cb.cb_connection(pConnect, ptr->cb.obj_connection, WS_ERROR_BAD);
        }
        else
        {
            printf("->strerror:accept error,(%s,%d)\n", __FILE__, __LINE__);
        }
	}
}

int ws_listen(ws_server_t *ptr, uv_loop_t *loop, int port, ws_malloc_connect cb, void *cb_obj)
{
	ptr->loop = loop;
	ptr->port = port;

	ptr->cb.cb_malloc = cb;
	ptr->cb.obj_malloc = cb_obj;

	uv_tcp_init(ptr->loop, &ptr->server);
	ptr->server.data = ptr;

	struct sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", port, &addr);
	uv_tcp_bind(&ptr->server, (const struct sockaddr*)&addr, 0);

	uv_listen((uv_stream_t *)(&ptr->server), 1000, cb_connection);

	return 0;
}

int ws_set_connect(ws_connect_t *ptr, ws_cb_connect cb, ws_cb_error cb_error, void *cb_obj)
{
    ptr->cb.cb_connect = cb;
    ptr->cb.cb_error = cb_error;
    ptr->cb.obj_connect = cb_obj;
    
    return 0;
}

int ws_ping(ws_connect_t *ptr)
{
    if (0 != ptr->bWrited[1])
    {
        return 1;
    }
    else
    {
        ptr->write_buf[31].base = ptr->write_buff;
        ptr->write_buf[31].len = 2;
        ptr->write_buf[31].base[0] = 0x89;
        ptr->write_buf[31].base[1] = 0;
        uv_write(&(ptr->write_req[1]), (uv_stream_t*)&ptr->connect, &(ptr->write_buf[31]), 1, cb_pong);
        ptr->bWrited[1] = 1;
        return 0;
    }
    
    return 0;
}

uv_loop_t* ws_loop(ws_server_t *ptr)
{
	return ptr->loop;
}

/*
请求:

GET / HTTP/1.1
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Version: 13
Sec-WebSocket-Key: qPDDeYhyaIhjCtefxxR4/Q==
Host: 192.168.8.181:3455
Sec-WebSocket-Protocol: zyx_protocol

回复:

HTTP/1.1 400 Bad Request
Connection: close
X-WebSocket-Reject-Reason: Client must provide a Host header.

HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: xY9zZMQTFsuY8fI6UBpaOmQ4x0Y=
Sec-WebSocket-Protocol: zyx_protocol

*/
