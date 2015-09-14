# websocket_by_libuv

基于libuv实现websocket解析，握手等

libuv在链接https://github.com/libuv/libuv

websocket是HTML5上的一个新协议，实现浏览器与服务器全双工通信。

握手时要借助HTTP协议后面就没它什么事了，
握手告诉服务器协议要升级为websocket还是子协议，是否支持，一般格式如下：


GET /chat HTTP/1.1

Host: server.example.com

Upgrade: websocket

Connection: Upgrade

Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==

Sec-WebSocket-Protocol: chat, superchat

Sec-WebSocket-Version: 13

Origin: http://example.com


通过返回码可以知道是否支持，成功是101，正确格式如下：
HTTP/1.1 101 Switching Protocols

Upgrade: websocket

Connection: Upgrade

Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=

Sec-WebSocket-Protocol: chat

代码里用的子协议是自定义的zyx_protocol,
Sec-WebSocket-Key是客户端随机生成的Base64 encode值，
服务端采用SHA-1处理返回Sec-WebSocket-Accept。

长链接一般都要保持心跳，协议里建议可以用ping,pong来完成
代码中也是这样用，根据nodejs里的websocket，服务端连上后要每隔一段时间发个ping，客户端有回复pong就保持连接，不然就超时断开。
客户端已实现自动回复pong，服务端要设置定时器发ping，还少了检测超时机制。

代码是基于libuv上来实现的，nodejs底层就是用libuv。简单实现了客户端及服务端的交流。
