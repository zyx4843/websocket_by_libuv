//
//  ws_parser.cpp
//  nvr
//
//  Created by zhengyixiong on 15/8/12.
//  Copyright (c) 2015年 zhengyixiong. All rights reserved.
//
#include "ws_parser.h"
#include "sha1.h"
#include <stdio.h>
#include <string.h>

void trimCpy(char *dst, char *src, int len)
{
    while(' ' == *src)
    {
        ++src;
        --len;
    }
    while(' ' == src[len - 1])
    {
        --len;
    }
    
    memcpy(dst, src, len);
    dst[len] = 0;
}

int ParseHandShake(const char* req, int reqSize, char* res, const int maxResSize, int &resSize)
{
    if ( (NULL == req) || (100 > reqSize) || (NULL == res) || (129 > maxResSize) ) {
        return 1;
    }
    
    //寻找http尾
    const char *pHeaderEnd = strstr(req, "\r\n\r\n");
    if (NULL == pHeaderEnd)
    {
        //没找到http尾
        return 1;
    }
    
    char host[64] = {0};
    char upgrade[20] = {0};
    char connection[20] = {0};
    char wskey[64] = {0};
    char origin[64] = {0};
    char protocol[64] = {0};
    char version[10] = {0};
    
    char line[84] = {0};
    char key[20] = {0};
    const char *pLineHead = req;
    const char *pLineEnd = req;
    char *pColon = NULL;
    while (pLineHead < pHeaderEnd)
    {
        pLineEnd = strstr(pLineHead, "\r\n");
        memcpy(line, pLineHead, pLineEnd - pLineHead);
        pColon = strstr(line, ":");
        if(NULL == pColon)
        {
            pLineHead = pLineEnd + 2;
            continue;
        }
        
        trimCpy(key, line, pColon - line);
        if (0 == strcmp(key, "Host"))
        {
            trimCpy(host, pColon + 1, pLineEnd - pLineHead - (pColon - line) - 1);
        }
        else if (0 == strcmp(key, "Upgrade"))
        {
            trimCpy(upgrade, pColon + 1, pLineEnd - pLineHead - (pColon - line) - 1);
        }
        else if (0 == strcmp(key, "Connection"))
        {
            trimCpy(connection, pColon + 1, pLineEnd - pLineHead - (pColon - line) - 1);
        }
        else if (0 == strcmp(key, "Sec-WebSocket-Key"))
        {
            trimCpy(wskey, pColon + 1, pLineEnd - pLineHead - (pColon - line) - 1);
        }
        else if (0 == strcmp(key, "Origin"))
        {
            trimCpy(origin, pColon + 1, pLineEnd - pLineHead - (pColon - line) - 1);
        }
        else if (0 == strcmp(key, "Sec-WebSocket-Protocol"))
        {
            trimCpy(protocol, pColon + 1, pLineEnd - pLineHead - (pColon - line) - 1);
        }
        else if (0 == strcmp(key, "Sec-WebSocket-Version"))
        {
            trimCpy(version, pColon + 1, pLineEnd - pLineHead - (pColon - line) - 1);
        }
        else
        {
            ;
        }
        
        pLineHead = pLineEnd + 2;
    }
    
    res[0] = 0;
    
    if ( (0 != strcmp(upgrade, "websocket")) || (0 != strcmp(connection, "Upgrade")) || (0 != strcmp(version, "13")) || (0 == strlen(wskey)) )
    {
        //http格式不对
        strncpy(res, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nContent-Type: text/plain\r\nConnection: Close\r\n\r\n", maxResSize);
        
        resSize = strlen(res);
        return 0;
    }
    
    strncpy(res, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ", maxResSize);
    strncat(wskey, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 64);
    shacalc(wskey, res + 97);
    strncat(res, "\r\n\r\n", maxResSize);
    resSize = strlen(res);
    
    return 0;
}

//返回值0表示成功，1表示参数错误，2表示长度不够
int Parse(const char* req, uint64_t reqSize, WsHeader &header, int &dataPos)
{
    if (NULL == req)
    {
        return 1;   //参数错误
    }
    
    if (2 > reqSize)
    {
        dataPos = 2;
        return 2;
    }
    
    header.fin = (req[0] >> 7) & 0x01;
    header.res[0] = (req[0] >> 6) & 0x01;
    header.res[1] = (req[0] >> 5) & 0x01;
    header.res[2] = (req[0] >> 4) & 0x01;
    header.type = req[0] & 0x0f;
    
    header.mask = (req[1] >> 7) & 0x01;
    header.length = req[1] & 0x7f;
    
    if (126 > header.length)
    {
        if (0 != header.mask)
        {
            if (6 < reqSize)
            {
                header.maskkey[0] = req[2];
                header.maskkey[1] = req[3];
                header.maskkey[2] = req[4];
                header.maskkey[3] = req[5];
                dataPos = 6;
            }
            else
            {
                dataPos = 6;
                return 2;   //长度不够
            }
        }
        else
        {
            dataPos = 2;
        }
    }
    else if (126 == header.length)
    {
        if (4 < reqSize)
        {
            dataPos = 4;
            return 2;   //长度不够
        }
        
        header.length += req[3] + (req[2] << 8);
        if (0 != header.mask)
        {
            if (8 < reqSize)
            {
                dataPos = 8;
                return 2;   //长度不够
            }
            
            header.maskkey[0] = req[4];
            header.maskkey[1] = req[5];
            header.maskkey[2] = req[6];
            header.maskkey[3] = req[7];
            dataPos = 8;
        }
        else
        {
            dataPos = 4;
        }
    }
    else if (127 == header.length)
    {
        if (10 < reqSize)
        {
            dataPos = 10;
            return 2;   //长度不够
        }
        
        header.length += ((uint64_t)req[3] << 48) + ((uint64_t)req[2] << 56);
        header.length += ((uint64_t)req[5] << 32) + ((uint64_t)req[4] << 40);
        header.length += ((uint64_t)req[7] << 16) + ((uint64_t)req[6] << 24);
        header.length += (uint64_t)req[9] + ((uint64_t)req[8] << 8);
        if (0 != header.mask)
        {
            if (14 < reqSize)
            {
                dataPos = 14;
                return 2;   //长度不够
            }
            
            header.maskkey[0] = req[10];
            header.maskkey[1] = req[11];
            header.maskkey[2] = req[12];
            header.maskkey[3] = req[13];
            dataPos = 14;
        }
        else
        {
            dataPos = 10;
        }
    }
    
    return 0;
}

int Pack(OpcodeType type, char* pHeader, int &headerSize, uint64_t dataSize)
{
    //res全为0，fin为1，mask为0
    pHeader[0] = type;
    pHeader[0] |= 0x80; //fin为1
    
    if (126 > dataSize)
    {
        if (2 > headerSize)
        {
            return 1;
        }
        pHeader[1] = dataSize;
        headerSize = 2;
    }
    else if (65661 >= dataSize)
    {
        if (4 > headerSize)
        {
            return 1;
        }
        pHeader[1] = 126;
        unsigned short *plen = reinterpret_cast<unsigned short *>(&pHeader[2]);
        *plen = dataSize - 126;
        //buf[2] = (dataSize >> 8) & 0xFF;
        //buf[3] = dataSize & 0xFF;
        headerSize = 4;
    }
    else
    {
        if (10 > headerSize)
        {
            return 1;
        }
        pHeader[1] = 127;
        uint64_t *plen = reinterpret_cast<uint64_t *>(&pHeader[2]);
        *plen = dataSize - 127;
        //buf[2] = (dataSize >> 56) & 0xFF;
        //buf[3] = dataSize & 0xFF;
        headerSize = 10;
    }
    
    return 0;
}