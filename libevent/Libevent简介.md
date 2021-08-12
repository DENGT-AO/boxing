# libevent简介

## 简介

libevent API 提供了一种机制，可以在文件描述符上发生特定事件或达到超时后执行回调函数。此外，libevent 还支持由于信号或常规超时而引起的回调。 libevent 旨在替换事件驱动网络服务器中的事件循环。应用程序只需要调用 event_dispatch()，然后动态添加或删除事件，而无需更改事件循环。

libevent是一个事件通知库，适用于windows、linux、bsd等多种平台，内部使用select、epoll、kqueue、IOCP等系统调用管理事件机制。

主要特点：

1.  统一数据源， 统一I/O事件，信号和定时器这三种事件；
2. 可移植，跨平台支持多种I/O多路复用技术， epoll、poll、dev/poll、select 和kqueue 等；
3. 对并发编程支持，避免竞态条件；
4.  高性能，由事件驱动；
5.  轻量级，专注于网络；

组成：

1. 事件管理包括各种IO（socket）、定时器、信号等事件，也是libevent应用最广的模块；
2. 缓存管理是指evbuffer功能；
3. DNS是libevent提供的一个异步DNS查询功能；
4. HTTP是libevent的一个轻量级http实现，包括服务器和客户端。



其实libevent与socket网络编程类似，都打开一个通道，连接、监听这个通道上的资源，读入或者写出数据。

http://www.wangafu.net/~nickm/libevent-book

查看API接口，请使用最新外文文档，中文文档可能滞后导致最新版本的libevent已经丢弃部分API的使用

## 函数API

### event base

#### 创建事件base

在您可以使用任何有趣的 Libevent 函数之前，您需要分配一个或多个 event_base 结构。每个 event_base 结构都包含一组事件，并且可以轮询以确定哪些事件处于活动状态。 如果 event_base 设置为使用锁定，则在多个线程之间访问它是安全的。但是，它的循环只能在单个线程中运行。如果要让多个线程轮询 IO，则需要为每个线程设置一个 event_base。 小费 [Libevent 的未来版本可能支持跨多个线程运行事件的 event_bases。] 每个 event_base 都有一个“方法”或一个后端，用于确定哪些事件已准备就绪。公认的方法有：

- select
- poll
- epoll
- kqueue
- devpoll
- evport
- win32

event_base_new() 函数使用默认设置分配并返回一个新的事件库。它检查环境变量并返回一个指向新 event_base 的指针。如果有错误，则返回 NULL。 在方法中进行选择时，它会选择操作系统支持的最快方法（头文件：<event2/event.h>）。

```C
struct event_base *event_base_new(void);
```

#### 创建事件

```c
#define EV_TIMEOUT      0x01
#define EV_READ         0x02
#define EV_WRITE        0x04
#define EV_SIGNAL       0x08
#define EV_PERSIST      0x10
#define EV_ET           0x20

typedef void (*event_callback_fn)(evutil_socket_t, short, void *);

struct event *event_new(struct event_base *base, evutil_socket_t fd, short what, event_callback_fn cb,
void *arg);

void event_free(struct event *event);
```

通过event_new创建需要检测的事件，fd是待检测的文件描述符，what是事件标志（EV_TIMEOUT等），cd是回调函数， arg是传入回调的自定义参数（本身会传入fd，标志）

- EV_TIMEOUT：此标志表示在超时过去后变为活动的事件。
- EV_READ：此标志表示当提供的文件描述符准备好读取时变为活动的事件。
-  EV_WRITE 此标志表示当提供的文件描述符准备好写入时变为活动的事件。 
- EV_SIGNAL 用于实现信号检测。请参阅下面的“构建信号事件”。 
- EV_PERSIST 表示事件是持久的。该参数未设置，事件触发后不会再次触发；若设置则表示事件触发后，还是依然出于轮询挂起状态。例如超时事件，若设置EV_PERSIST ，则超时触发后，事件依然挂起，下次超时还可以触发。
- EV_ET 指示事件应该是边缘触发的，如果底层 event_base 后端支持边缘触发的事件。这会影响 EV_READ 和 EV_WRITE 的语义。

回调函数传入事件本身：

通常，您可能希望创建一个将自身作为回调参数接收的事件。但是，您不能将指向该事件的指针作为参数传递给 event_new()，因为它还不存在。为了解决这个问题，你可以使用 event_self_cbarg()。 

void *event_self_cbarg(); 

event_self_cbarg() 函数返回一个神奇的指针，当作为事件回调参数传递时，它告诉 event_new() 创建一个接收自身作为其回调参数的事件。

libevent还对事件函数进行了封装，分别有：超时事件、信号事件。

```c
#define evtimer_new(base, callback, arg) \
    event_new((base), -1, 0, (callback), (arg))
#define evtimer_add(ev, tv) \
    event_add((ev),(tv))
#define evtimer_del(ev) \
    event_del(ev)
#define evtimer_pending(ev, tv_out) \
    event_pending((ev), EV_TIMEOUT, (tv_out))
```

```c
#define evsignal_add(ev, tv) \
    event_add((ev),(tv))
#define evsignal_del(ev) \
    event_del(ev)
#define evsignal_pending(ev, what, tv_out) \
    event_pending((ev), (what), (tv_out))
```

挂起事件：

```c
int event_add(struct event *ev, const struct timeval *tv);
```

参数tv可以NULL，表示事件没有超时检测，会一直等到事件触发。

取消挂起事件：

```c
int event_del(struct event *ev);
```

<font color='red'>注意：如果在事件变为活动状态后但在其回调有机会执行之前删除事件，则不会执行回调。</font>

单独移出事件超时：

```c
int event_remove_timer(struct event *ev);
```

最后，您可以在不删除其 IO 或信号组件的情况下完全删除挂起事件的超时。如果事件没有超时挂起，则 event_remove_timer() 无效。如果事件只有超时但没有 IO 或信号组件，则 event_remove_timer() 与 event_del() 具有相同的效果。成功时返回值为 0，失败时返回 -1。

#### 事件库轮询

启动：

```C
int event_base_dispatch(struct event_base *base);
```

停止：

```c
int event_base_loopexit(struct event_base *base,
                        const struct timeval *tv);
int event_base_loopbreak(struct event_base *base);
```

### bufferevent

大多数时候，应用程序除了响应事件请求外，还需要对数据（缓存）进行处理。

通常情况下，我们写数据需要经过一下步骤：

1. 决定要向连接（例如socket）中写入什么数据，把这些数据放入缓存（buf）
2. 等待连接可写
3. 写入尽可能多的数据
4. 记住写入了多少数据，如果还有数据没写完，等待连接再次变为可写状态。

bufferevent由一个底层传输系统（例如socket），一个读缓冲区和一个写缓冲区（这是定义时自带的，但是也需要另外两个真正的缓冲buf进行数据的读写及操作）

对于普通的events， 当底层传输系统可读或者可写时，调用回调方式； 而bufferevent提供了一种替代方式：它在已经写入、或者读出数据的时候才调用回调函数。

通用接口：

1. 基于socket的bufferevent：

   在底层流式socket（TCP）发送和接收数据，使用event_*接口作为其后端。

2. 异步IO的bufferevent：

   使用`Windows IOCP`接口来想底层流式socket发送和接收数据。（Windows only， 实验性的）

3. 过滤型的bufferevent：

   在数据传送到底层bufferevent对象之前，对到来和外出的数据进行前期处理的bufferevent，比如对数据进行压缩或者转换。

4. 成对的bufferevent：

   两个缓冲区事件相互传输数据。

<font color=red>注意：Bufferevents 目前仅适用于面向流的协议，如 TCP。将来可能会支持面向数据报的协议，如 UDP。</font>

#### bufferevent和evbuffers

每一bufferevent都有一个输入缓冲区和输出缓冲区，这些缓冲区（buffer）都是struct evbuffer类型。当有数据写入时，先写入output缓冲区，然后output buffer会自动向底层传输系统写入；当有数据读入时，直接读input缓冲区。

#### 回调函数和水位数

每一个bufferevent都有两个数据相关的回到函数， 一个 **读回调**和一个**写回调**。 默认情况下，当有数据从底层传输读取时，读回调函数就会被调用； 当ouput buffer想底层传输写入足够多的数据时， 写回调函数就会被调用。

​		通过调整bufferevent的读取和写入“水位线”（watermarks），可以改变这些函数的默认行为。每个bufferevent都有4个水位线：

1. 读 低水位

   当bufferevent的输入缓冲区的数据量到达该水位线时，那么bufferevent的读回调函数就会被调用。该水位线默认为0，所以每一次读取操作都会导致读回调函数被调用。

2. 读 高水位

   如果bufferevent的输入缓冲的数据量到达该水位线时，那么bufferevent就会停止从底层系统读取数据，直到输入缓冲区中足够多的数据被抽走，从而数据量再次低于该水位线。默认情况下该水位线是无限制的，所以从来不会因为输入缓冲区的大小而停止读取操作。

3. 写 低水位

   当写入操作是的输出缓冲区的数据量达到或者低于该水位时，才调用写回调函数。默认情况下，该值为0，所以输出缓冲区被清空时才调用写回调函数。

4. 写 高水位

   并非由bufferevent直接使用，对于bufferevent作为其他bufferevent底层传输系统的时候，该水位线才有特殊意义。所以可以参考后面的过滤型bufferevent。

　bufferevent还提供了`error` 或者`event` 的回调函数，用来通知应用程序关于非数据相关的事件。比如：关闭连接或者发生错误。 为此，定义了以下 `event`标志：

- **BEV_EVENT_READING：**读操作期间发生了事件。具体哪个事件参见其他标志。
- **BEV_EVENT_WRITING：**写操作期间发生了事件。具体哪个事件参见其他标志。
- **BEV_EVENT_ERROR：**在bufferevent操作期间发生了错误，可以调用`EVUTIL_SOCKET_ERROR`函数来得到更多的错误信息。
- **BEV_EVENT_TIMEOUT：** bufferevent上发生了超时
- **BEV_EVENT_EOF：** bufferevent上遇到了EOF标志
- **BEV_EVENT_CONNECTED：**在bufferevent上请求的连接已经完成

<font color='red'>注意：为了保证安全，函数回调时可以设置延期回调，避免栈上数据出现溢出的情况</font>

#### 创建bufferevent的选项标志

这些标志是创建bufferevent时告诉底层传输系统或者内核的标志

- **BEV_OPT_CLOSE_ON_FREE：** 当释放bufferevent时，关闭底层的传输系统。 这将关闭底层套接字，释放底层bufferevent等。
- **BEV_OPT_THREADSAFE：** 自动为bufferevent分配锁， 从而在多线程中可以安全使用。
- **BEV_OPT_DEFER_CALLBACKS：** bufferevent会将其所有回调函数进行延迟调用设置。
- **BEV_OPT_UNLOCK_CALLBACKS：** 默认情况下， 当设置bufferevent为线程安全的时候，任何用户提供的回调函数调用时都会锁住bufferevent的锁， 设置改标志可以在提供的回调函数被调用时不锁住bufferevent的锁。

#### 基于socket的bufferevent

最简单的bufferevents就是基于socket类型的bufferevent。基于socket的bufferevent使用Libevent底层event机制探测底层网络socket何时准备好读和写，而且使用底层网络调用（比如`readv`，`writev`，`WSASend`或`WSARecv`）进行传送和接受数据。

1. 创建基于socket的bufferevent

   ```c
   struct bufferevent bufferevent_socket_new(struct event_base *base, evutil_socket_t fd, enum bufferevent_options options);
   ```

   - base: 表示事件base

   - option：是bufferevent选项的掩码（BEV_OPT_CLOSE_ON_FREE等）
   - fd：参数是一个可选的socket文件描述符。如果系统以后再设置socket文件描述符，可以将fd置为-1。

   <font color='red'>注意：要确保提供给`bufferevent_socket_new`的socket是非阻塞模式。Libevent提供了便于使用的`evutil_make_socket_nonblocking`来设置非阻塞模式。</font>

   `bufferevent_socket_new`成功时返回一个`bufferevent`，失败时返回`NULL`。

2. 基于socket的bufferevent上发送连接（应用程序与底层socket进行连接）

   ```c
   int bufferevent_socket_connect(struct bufferevent *bev, struct sockaddr *address, int addrlen);
   ```
   
   - bev：第一步创建bufferevent
   - address：连接地址（与connect函数参数类似）
   - addrlen：连接长度（与connect函数参数类似，直接sizeof）

   如果该bufferevent尚未设置socket，则调用该函数为该bufferevent会分配一个新的流类型的socket，并且置其为非阻塞的。
   
   该函数如果在建链成功时，返回0，如果发生错误，则返回-1.

<font color='red'>注意：在连接前需要调用bufferevent_setcb进行读写函数的设置</font>

#### 通用的bufferevent操作

1. 释放bufferevent

   ```c
   void  bufferevent_free(struct  bufferevent *bev);
   ```

   bev：之前创建bufferevent

   在释放时，若有回调函数未执行完，会等待回调完成后再删除；数据缓冲区的数据会丢弃，不会强制flush；若设置了BEV_OPT_CLOSE_ON_FREE，则会关闭底层传输系统。

2. 读写事件的使能

   ```C
   void bufferevent_enable(struct bufferevent *bufev, short events);
   ```

   events：EV_READ，EV_WRITE，或者EV_READ|EV_WRITE

   void bufferevent_disable(struct bufferevent *bufev, short events);

   禁用想用的读写缓冲区

   short bufferevent_get_enabled(struct bufferevent *bufev);

   获取当前bufferevent的使能事件。

   <font color='red'>**默认情况下，新创建的bufferevent会enable写操作，而禁止读操作。**</font>

3. bufferevent中的数据操作

   ```C
   struct evbuffer *bufferevent_get_input(struct bufferevent *bufev); 
   
   struct evbuffer *bufferevent_get_output(struct bufferevent *bufev);
   ```

   这两个函数是非常强大的基础：它们分别返回输入和输出缓冲区。

   请注意，应用程序只能删除（而不是添加）输入缓冲区上的数据，并且只能添加（而不是删除）输出缓冲区中的数据。 如果缓冲区事件上的写入由于数据太少而停止（或者如果读取因数据过多而停止），则将数据添加到输出缓冲区（或从输入缓冲区中删除数据）将自动重新启动它。

   
   
   ```C
   int bufferevent_write(struct bufferevent *bufev, const void *data, size_t size);
   int bufferevent_write_buffer(struct bufferevent *bufev, struct evbuffer *buf);
   ```

   这些函数将数据添加到 bufferevent 的输出缓冲区。调用 bufferevent_write() 将 size 字节从内存中的 data 添加到输出缓冲区的末尾。调用 bufferevent_write_buffer() 删除 buf 的全部内容并将它们放在输出缓冲区的末尾。如果成功，两者都返回 0，如果发生错误，则返回 -1。
   
   
   
   ```c
   size_t bufferevent_read(struct bufferevent *bufev, void *data, size_t size);
   int bufferevent_read_buffer(struct bufferevent *bufev, struct evbuffer *buf);
   ```

   这些函数从 bufferevent 的输入缓冲区中删除数据。 bufferevent_read() 函数从输入缓冲区中删除最多 size 个字节，将它们存储到内存中的 data 中。它返回实际删除的字节数。 bufferevent_read_buffer() 函数将输入缓冲区的全部内容排空并将它们放入 buf 中；成功时返回 0，失败时返回 -1。 请注意，使用 bufferevent_read() 时，data 处的内存块实际上必须有足够的空间来保存 size 字节的数据。

   

   ```c
   int bufferevent_flush(struct bufferevent *bufev, short iotype, enum bufferevent_flush_mode state);
   ```

   刷新一个bufferevent
   
   刷新 bufferevent 告诉 bufferevent 强制从底层传输读取或写入尽可能多的字节，忽略其他可能阻止写入的限制。它的详细功能取决于bufferevent的类型。 iotype 参数应为 EV_READ、EV_WRITE 或 EV_READ|EV_WRITE 以指示是否应处理读取、写入或两者的字节。状态参数可以是 BEV_NORMAL、BEV_FLUSH 或 BEV_FINISHED 之一。 BEV_FINISHED 表示应该告诉对方不再发送数据； BEV_NORMAL 和 BEV_FLUSH 之间的区别取决于缓冲事件的类型。 
   
   bufferevent_flush() 函数在失败时返回 -1，如果没有数据被刷新则返回 0，如果某些数据被刷新则返回 1。
   
   
   
   ```C
   void bufferevent_lock(struct bufferevent *bufev);
   void bufferevent_unlock(struct bufferevent *bufev);
   ```
   
   原子操作，锁定bufferevent。
   
   请注意，如果缓冲区事件在创建时没有被赋予 BEV_OPT_THREADSAFE 线程，或者 Libevent 的线程支持没有被激活，则锁定缓冲区事件没有任何影响。 使用此函数锁定 bufferevent 也将锁定其关联的 evbuffers。这些函数是递归的：锁定一个你已经持有锁的缓冲区事件是安全的。当然，您每次锁定 bufferevent 时都必须调用 unlock 一次。

### evbuffer

libevent 的 evbuffer 实现了为向后面添加数据和从前面移除数据而优化的字节队 

列。

evbuffer 用于处理缓冲网络 IO 的“缓冲”部分。<font color='red'>实际上就是两个buf</font>

#### 创建和释放evbuffer

```c
struct evbuffer *evbuffer_new(void); 

void evbuffer_free(struct evbuffer *buf); 
```

这两个函数的功能很简明: evbuffer_new() 分配和返回一个新的空 evbuffer ; 而 

evbuffer_free()释放 evbuffer 和其内容。

#### evbuffer与线程安全

```c
int evbuffer_enable_locking(struct evbuffer *buf, void *lock); 

void evbuffer_lock(struct evbuffer *buf); 

void evbuffer_unlock(struct evbuffer *buf); 
```

默认情况下,在多个线程中同时访问 evbuffer 是不安全的。如果需要这样的访问,可 

以调 用 evbuffer_enable_locking() 。如果 lock 参数为 NULL , libevent 会使用 

evthread_set_lock_creation_callback 提供的锁创建函数创建一个锁 。否 

则,libevent 将 lock 参数用作锁。 

evbuffer_lock()和 evbuffer_unlock()函数分别请求和释放 evbuffer 上的锁。可以使 

用这两个 函数让一系列操作是原子的。如果 evbuffer 没有启用锁,这两个函数不做 

任何操作。 

(注意:对于单个操作,不需要调用 evbuffer_lock()和evbuffer_unlock(): 如果 evbuffer 

启用了锁,单个操作就已经是原子的 。只有在需要多个操作连续执行 ,不让其他线程 

介入的 时候,才需要手动锁定 evbuffer) 

#### 向evbuffer中添加数据

```C
int evbuffer_add(struct evbuffer *buf, const void *data, size_t datlen);
```

此函数将 data 中的 datlen 字节附加到 buf 的末尾。成功时返回 0，失败时返回 -1。

```c
int evbuffer_add_printf(struct evbuffer *buf, const char *fmt, ...)
int evbuffer_add_vprintf(struct evbuffer *buf, const char *fmt, va_list ap);
```

这些函数将格式化数据附加到 buf 的末尾。格式参数和其他剩余参数分别由 C 库函数“printf”和“vprintf”处理。这些函数返回附加的字节数。

int evbuffer_expand(struct evbuffer *buf, size_t datlen);

此函数更改缓冲区中的最后一块内存，或添加一个新块，以便缓冲区现在足够大以包含 datlen 字节而无需任何进一步分配。

#### 从一个evbuffer移动数据至另一个

```c
int evbuffer_add_buffer(struct evbuffer *dst, struct evbuffer *src);
int evbuffer_remove_buffer(struct evbuffer *src, struct evbuffer *dst, size_t datlen);
```

evbuffer_add_buffer() 函数将所有数据从 src 移动到 dst 的末尾。成功时返回 0，失败时返回 -1。 evbuffer_remove_buffer() 函数精确地将 datlen 字节从 src 移动到 dst 的末尾，尽可能少地复制。如果要移动的字节少于 datlen，它会移动所有字节。它返回移动的字节数。

#### 添加数据至evbuffer首部

```c
int evbuffer_prepend(struct evbuffer *buf, const void *data, size_t size);
int evbuffer_prepend_buffer(struct evbuffer *dst, struct evbuffer* src);
```

这些函数的行为分别与 evbuffer_add() 和 evbuffer_add_buffer() 相同，只是它们将数据移动到目标缓冲区的前面。

#### 从一个evbuffer移出数据

```c
int evbuffer_drain(struct evbuffer *buf, size_t len);
int evbuffer_remove(struct evbuffer *buf, void *data, size_t datlen);
```

evbuffer_remove() 函数将 buf 前面的第一个 datlen 字节复制并删除到内存中的 data 中。如果可用字节数少于 datlen，则该函数将复制所有字节。失败时返回值为 -1，否则为复制的字节数。 evbuffer_drain() 函数的行为与 evbuffer_remove() 一样，不同之处在于它不复制数据：它只是从缓冲区的前面删除它。成功时返回 0，失败时返回 -1。

#### 使用 evbuffers 的网络 IO

```c
int evbuffer_write(struct evbuffer *buffer, evutil_socket_t fd);
int evbuffer_write_atmost(struct evbuffer *buffer, evutil_socket_t fd,
        ev_ssize_t howmuch);
int evbuffer_read(struct evbuffer *buffer, evutil_socket_t fd, int howmuch);
```

evbuffer_read() 函数从套接字 fd 读取最多howmuch 字节到缓冲区的末尾。它在成功时返回读取的字节数，在 EOF 时返回 0，在错误时返回 -1。请注意，该错误可能表明非阻塞操作不会成功；您需要检查 EAGAIN（或 Windows 上的 WSAEWOULDBLOCK）的错误代码。如果 howmuch 是负数，evbuffer_read() 会尝试猜测自己读取多少。 

evbuffer_write_atmost() 函数尝试将缓冲区前端的多少字节写入套接字 fd。它返回写入成功的字节数，失败时返回 -1。和evbuffer_read()一样，你需要检查错误代码，看错误是否真实，或者只是表明非阻塞IO无法立即完成。如果你给 howmuch 一个负值，我们尝试写入缓冲区的全部内容。 

调用 evbuffer_write() 与使用负的 howmuch 参数调用 evbuffer_write_atmost() 相同：它尝试尽可能多地刷新缓冲区。 在 Unix 上，这些函数应该适用于任何支持读写的文件描述符。在 Windows 上，仅支持套接字。 注意，在使用bufferevents时，不需要调用这些IO函数； bufferevents 代码为您完成。

#### 添加文件至一个evbuffer

```c
int evbuffer_add_file(struct evbuffer *output, int fd, ev_off_t offset, size_t length);
```

evbuffer_add_file() 函数假设它有一个可供读取的打开文件描述符（不是套接字，一次！）。它从文件中添加长度字节，从位置偏移开始，到输出的结尾。成功时返回 0，失败时返回 -1。



demo1：

简单定时器，每10秒打印一次时间。

```c
#include <stdio.h>
#include <event2/event.h>
#include <stdlib.h>
#include <sys/time.h>


//编译方式：gcc -o timer timer.c -levent

struct event * time_event = NULL;
struct event_base *base = NULL;

void PrintTime()
{
    struct timeval timer;
    gettimeofday(&timer, NULL);
    volatile uint current_time = (uint)(timer.tv_sec);
    printf("current_time:%d\n", current_time);
}

void timeout_cb(evutil_socket_t fd, short event_flag, void *arg)
{
    PrintTime();
    printf("flag = %d \n", (int)event_flag);

    //重新设置timer为未决状态
    /*
    struct timeval timeout = {0, 0};
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    event_add(time_event, &timeout);
    */
}

int main(int argc, char **argv)
{

    struct timeval timeout = {0, 0};
    int ret = 0;
    struct event_base* base = NULL;


    //创建base结构体
    base = event_base_new();
    if (base == NULL)
    {
        printf("create event base error!\n");
        exit(1);
    }
    
    //设置超时事件
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    //EV_PERSIST可使事件持续为非未决状态
    time_event = event_new(base, -1, EV_TIMEOUT | EV_PERSIST, timeout_cb, (void *)&time_event);
    //time_event = event_new(base, -1, EV_PERSIST, cb_func, event_self_cbarg());  //持久超时
    //event_self_cbarg()将事件自己传入回调函数
    //time_event = evtimer_new(base, timeout_cb, &time_event);  //一次超时

    //添加事件为未决状态
    event_add(time_event, &timeout);
    event_base_dispatch(base);

    event_base_free(base);
    
    return 0;
}

```

代码中有两种方式让事件成为循环超时事件，一种是在event_new的时候进行EV_PERSIST设置；另一种是在callback函数中再次设置.tv_sec与.tv_usec参数并挂起事件。



demo2：

串口收发

思路：利用多线程，主线程进行/dev/ttyUSB0的读监控，子线程进行循环写入数据，串口的初始化

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <event2/event.h>

//串口操作的头文件
#include     <stdio.h>      /*标准输入输出定义*/
#include     <stdlib.h>     /*标准函数库定义*/
#include     <unistd.h>     /*Unix 标准函数定义*/
#include     <sys/types.h>  
#include     <sys/stat.h>   
#include     <fcntl.h>      /*文件控制定义*/
#include     <termios.h>    /*PPSIX 终端控制定义*/
#include     <errno.h>      /*错误号定义*/

#include    <pthread.h>

#include <event2/event.h>

//串口名称
#define UART_DEV "/dev/ttyUSB0"

//编译方法：gcc -o test test.c -levent -lpthread
/*
        #include <termios.h>
        struct termios{
            tcflag_t  c_iflag;  //输入模式标志
            tcflag_t  c_oflag;  //输出模式标志
            tcflag_t  c_cflag;  //控制选项
            tcflag_t  c_lflag;  //行选项
            cc_t      c_cc[NCCS]; //控制字符
        }
        每个选项都是16位数,每一位都有其含义
        .c_iflag：输入模式标志，控制终端输入方式
                键 值             说 明
                IGNBRK          忽略BREAK键输入
                BRKINT          如果设置了IGNBRK，BREAK键的输入将被忽略，如果设置了BRKINT ，将产生SIGINT中断
                IGNPAR          忽略奇偶校验错误
                PARMRK          标识奇偶校验错误
                INPCK           允许输入奇偶校验
                ISTRIP          去除字符的第8个比特
                INLCR           将输入的NL（换行）转换成CR（回车）
                IGNCR           忽略输入的回车
                ICRNL           将输入的回车转化成换行（如果IGNCR未设置的情况下）
                IUCLC           将输入的大写字符转换成小写字符（非POSIX）
                IXON            允许输入时对XON/XOFF流进行控制
                IXANY           输入任何字符将重启停止的输出
                IXOFF           允许输入时对XON/XOFF流进行控制
                IMAXBEL         当输入队列满的时候开始响铃，Linux在使用该参数而是认为该参数总是已经设置
        .c_oflag：输出模式标志，控制终端输出方式
                键 值             说 明
                OPOST           处理后输出
                OLCUC           将输入的小写字符转换成大写字符（非POSIX）
                ONLCR           将输入的NL（换行）转换成CR（回车）及NL（换行）
                OCRNL           将输入的CR（回车）转换成NL（换行）
                ONOCR           第一行不输出回车符
                ONLRET          不输出回车符
                OFILL           发送填充字符以延迟终端输出
                OFDEL           以ASCII码的DEL作为填充字符，如果未设置该参数，填充字符将是NUL（'\0'）（非POSIX）
                NLDLY           换行输出延时，可以取NL0（不延迟）或NL1（延迟0.1s）
                CRDLY           回车延迟，取值范围为：CR0、CR1、CR2和 CR3
                TABDLY          水平制表符输出延迟，取值范围为：TAB0、TAB1、TAB2和TAB3
                BSDLY           空格输出延迟，可以取BS0或BS1
                VTDLY           垂直制表符输出延迟，可以取VT0或VT1
                FFDLY           换页延迟，可以取FF0或FF1
        .c_cflag：控制模式标志，指定终端硬件控制信息
                键 值             说 明
                CBAUD           波特率（4+1位）（非POSIX）
                CBAUDEX         附加波特率（1位）（非POSIX）
                CSIZE           字符长度，取值范围为CS5、CS6、CS7或CS8
                CSTOPB          设置两个停止位
                CREAD           使用接收器
                PARENB          使用奇偶校验
                PARODD          对输入使用奇偶校验，对输出使用偶校验
                HUPCL           关闭设备时挂起
                CLOCAL          忽略调制解调器线路状态
                CRTSCTS         使用RTS/CTS流控制
        .c_lflag：本地模式标志，控制终端编辑功能
                 键 值             说 明
                ISIG            当输入INTR、QUIT、SUSP或DSUSP时，产生相应的信号
                ICANON          使用标准输入模式
                XCASE           在ICANON和XCASE同时设置的情况下，终端只使用大写。如果只设置了XCASE，则输入字符将被转换为小写字符，除非字符使用了转义字符（非POSIX，且Linux不支持该参数）
                ECHO            显示输入字符
                ECHOE           如果ICANON同时设置，ERASE将删除输入的字符，WERASE将删除输入的单词
                ECHOK           如果ICANON同时设置，KILL将删除当前行
                ECHONL          如果ICANON同时设置，即使ECHO没有设置依然显示换行符
                ECHOPRT         如果ECHO和ICANON同时设置，将删除打印出的字符（非POSIX）
                TOSTOP          向后台输出发送SIGTTOU信号
        .c_cc[NCCS]：控制字符，用于保存终端驱动程序中的特殊字符
            只有在本地模式标志c_lflag中设置了IEXITEN时，POSIX没有定义的控制字符才能在Linux中使用。每个控制字符都对应一个按键组合（^C、^H等）。
            VMIN和VTIME这两个控制字符除外，它们不对应控制符。这两个控制字符只在原始模式下才有效。
                键 值             说 明
                c_cc[VMIN]      原始模式（非标准模式）读的最小字符数
                c_cc[VTIME]     原始模式（非标准模式）读时的延时，以十分之一秒为单位

                c_cc[VINTR]     默认对应的控制符是^C，作用是清空输入和输出队列的数据并且向tty设备的前台进程组中的每一个程序发送一个SIGINT信号，对SIGINT信号没有定义处理程序的进程会马上退出。
                c_cc[VQUIT]     默认对应的控制符是^/，作用是清空输入和输出队列的数据并向tty设备的前台进程组中的每一个程序发送一个SIGQUIT信号，对SIGQUIT 信号没有定义处理程序的进程会马上退出。
                c_cc[verase]    默认对应的控制符是^H或^?，作用是在标准模式下，删除本行前一个字符，该字符在原始模式下没有作用。
                c_cc[VKILL]     默认对应的控制符是^U，在标准模式下，删除整行字符，该字符在原始模式下没有作用。
                c_cc[VEOF]      默认对应的控制符是^D，在标准模式下，使用read()返回0，标志一个文件结束。
                c_cc[VSTOP]     默认对应的控制字符是^S，作用是使用tty设备暂停输出直到接收到VSTART控制字符。或者，如果设备了IXANY，则等收到任何字符就开始输出。
                c_cc[VSTART]    默认对应的控制字符是^Q，作用是重新开始被暂停的tty设备的输出。
                c_cc[VSUSP]     默认对应的控制字符是^Z，使当前的前台进程接收到一个SIGTSTP信号。
                c_cc[VEOL]
                c_cc[VEOL2]     在标准模式下，这两个下标在行的末尾加上一个换行符（'/n'），标志一个行的结束，从而使用缓冲区中的数据被发送，并开始新的一行。POSIX中没有定义VEOL2。
                c_cc[VREPRINT]  默认对应的控制符是^R，在标准模式下，如果设置了本地模式标志ECHO，使用VERPRINT对应的控制符和换行符在本地显示，并且重新打印当前缓冲区中的字符。POSIX中没有定义VERPRINT。
                c_cc[VWERASE]   默认对应的控制字符是^W，在标准模式下，删除缓冲区末端的所有空格符，然后删除与之相邻的非空格符，从而起到在一行中删除前一个单词的效果。 POSIX中没有定义VWERASE。
                c_cc[VLNEXT]    默认对应的控制符是^V，作用是让下一个字符原封不动地进入缓冲区。如果要让^V字符进入缓冲区，需要按两下^V。POSIX中没有定义 VLNEXT。
            
        
*/

int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    
    struct termios newtio, oldtio;
    /*获取原有串口配置*/
    if  ( tcgetattr( fd,&newtio)  !=  0) { 
        perror("SetupSerial 1");
        return -1;
    }
    
    //memset(&newtio, 0, sizeof(newtio));
    /*CREAD 开启串行数据接收，CLOCAL并打开本地连接模式*/
    newtio.c_cflag  |=  CLOCAL | CREAD;

    /*设置数据位*/
    newtio.c_cflag &= ~CSIZE;
    switch( nBits )
    {
        case 7:
            newtio.c_cflag |= CS7;
            break;
        case 8:
            newtio.c_cflag |= CS8;
            break;
    }
    /* 设置奇偶校验位 */
    switch( nEvent )
    {
        case 'O':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'E': 
            newtio.c_iflag |= (INPCK | ISTRIP);
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
            break;
        case 'N':  
            newtio.c_cflag &= ~PARENB;
            break;
    }
    /*
     
     设置波特率 
     cfsetispeed设置输入波特率
     cfsetospeed设置输出波特率
    */
    switch( nSpeed )
    {
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        case 460800:
            cfsetispeed(&newtio, B460800);
            cfsetospeed(&newtio, B460800);
            break;
        default:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
    }
    /*设置停止位*/
    if( nStop == 1 )/*设置停止位；若停止位为1，则清除CSTOPB，若停止位为2，则激活CSTOPB*/
    {
        newtio.c_cflag &=  ~CSTOPB;/*默认为一位停止位； */
    }
    else if ( nStop == 2 )
    {
        newtio.c_cflag |=  CSTOPB;
    }
    /*设置最少字符和等待时间，对于接收字符和等待时间没有特别的要求时*/
    newtio.c_cc[VTIME]  = 0;/*非规范模式读取时的超时时间；*/
    newtio.c_cc[VMIN] = 0;/*非规范模式读取时的最小字符数*/
    /*tcflush清空终端未完成的输入/输出请求及数据；TCIFLUSH表示清空正收到的数据，且不读取出来 */
    tcflush(fd,TCIFLUSH);
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)
    {
        perror("com set error");
        return -1;
    }
//  printf("set done!\n\r");
    return 0;
}

int uart_init(char *dev, int nSpeed, int nBits, char nEvent, int nStop)
{
    int fd = 0;
    int ret = 0;
    
    fd = open(dev , O_RDWR | O_NOCTTY | O_NDELAY, 0);    
    if (fd < 0) 
    {
        printf("open error!\n");        
        return -1;
    }
    ret = set_opt(fd, nSpeed, nBits, nEvent, nStop);
    if (ret == -1)
    {
        perror("set uart error~\n");
        return -1;
    }


    return fd;
}

void * writefunc(void *ptr)
{
    int fd = *(int *)ptr;
    char buf[1024] = {0};
    int len = 0;

    while (1)
    {
        //fgets(buf, 1024, stdin);
        gets(buf);
        len = strlen(buf);
        if ( len >= 0 )
        {
            buf[len] = '\n';
            write(fd, buf, len + 1);
        }
    }

}

void read_cb(evutil_socket_t fd, short event_flag, void *arg)
{
    char buf[1024] = {0};
    int len = read(fd, buf, sizeof(buf-1));
    //printf("read event: %s \n ", what & EV_READ ? "YES":"NO");
    printf("%s", buf);
}

int main(int argc, char **argv)
{
    int ret = 0;
    int fd = 0;
    pthread_t writeThread;
    struct event_base *base;
    struct event *readevent;
    
    
    //设置串口属性包括波特率、校验位、停止位等
    fd = uart_init(UART_DEV, 115200, 8, 'N', 1);
    if (fd == -1)
    {
        printf("UART INIT ERROR!\n");
        exit(-1);
    }

    //创建写线程
    writeThread = pthread_create(&writeThread, NULL, writefunc, &fd);
    if(ret != 0)
    {
        printf("create pthread error!\n");
        goto ERR1;
    }

    //创建event base
    base = event_base_new();
    if (base == NULL)
    {
        perror("create event baseerror\n");
        goto ERR1;
    }

    //创建事件
    readevent = event_new(base, fd, EV_READ | EV_PERSIST, read_cb, NULL);
    if (readevent == NULL)
    {
        perror("create event error\n");
        goto ERR2;
    }

    //挂起事件
    ret = event_add(readevent, NULL);
    if (ret)
    {
        perror("add event error\n");
        goto ERR2;
    }

    //轮询事件
    event_base_dispatch(base);

ERR2:
    event_base_free(base);
ERR1:
    close(fd);
    return 0;
}
```

