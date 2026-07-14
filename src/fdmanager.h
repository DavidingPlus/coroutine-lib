#ifndef _COROUTINE_FDMANAGER_H_
#define _COROUTINE_FDMANAGER_H_

#include <memory>
#include <vector>
#include <shared_mutex>


// FdCtx 类主要用于管理与文件描述符相关的状态和操作。在用户态记录了 fd 的读写超时和非阻塞信息，其中非阻塞包括用户显示设置的非阻塞和 hook 内部设置的非阻塞，区分这两种非阻塞可以有效应对用户对 fd 设置/获取 NONBLOCK 模式的情形。
class FdCtx : public std::enable_shared_from_this<FdCtx>
{

public:

    FdCtx(int fd);

    ~FdCtx();

    // 初始化 FdCtx 对象。
    bool init();

    bool isInit() const { return m_isInit; }

    bool isSocket() const { return m_isSocket; }

    bool isClosed() const { return m_isClosed; }

    // 设置和获取用户层面的非阻塞状态。
    void setUserNonblock(bool v) { m_userNonblock = v; }

    bool getUserNonblock() const { return m_userNonblock; }

    // 设置和获取系统层面的非阻塞状态。
    void setSysNonblock(bool v) { m_sysNonblock = v; }

    bool getSysNonblock() const { return m_sysNonblock; }

    // 设置和获取超时时间，type 用于区分读事件和写事件的超时设置，v 表示时间毫秒。
    void setTimeout(int type, uint64_t v);

    uint64_t getTimeout(int type);


private:

    // 标记文件描述符是否已初始化。
    bool m_isInit = false;

    // 标记文件描述符是否是一个套接字。
    bool m_isSocket = false;

    // 标记文件描述符是否设置为系统非阻塞模式。
    bool m_sysNonblock = false;

    // 标记文件描述符是否设置为用户非阻塞模式。
    bool m_userNonblock = false;

    // 标记文件描述符是否已关闭。
    bool m_isClosed = false;

    // 文件描述符的整数值。
    int m_fd = -1;

    // 读事件的超时时间，默认为 -1 表示没有超时限制。
    uint64_t m_recvTimeout = static_cast<uint64_t>(-1);

    // 写事件的超时时间，默认为 -1 表示没有超时限制。
    uint64_t m_sendTimeout = static_cast<uint64_t>(-1);
};


// 用于管理 FdCtx 对象的集合。它提供了对文件描述符上下文的访问和管理功能。
// 显然这个类是一个单例模式，确保类只有一个全局实例，并提供全局访问点。
class FdManager
{

public:

    // 构造函数。
    FdManager();

    // 获取指定文件描述符的 FdCtx 对象。如果 autoCreate 为 true，在不存在时自动创建新的 FdCtx 对象。
    std::shared_ptr<FdCtx> get(int fd, bool autoCreate = false);

    // 删除指定文件描述符的 FdCtx 对象。
    void del(int fd);


private:

    // 用于保护对 m_datas 的访问，支持共享读锁和独占写锁。
    std::shared_mutex m_mutex;

    // 存储所有 FdCtx 对象的共享指针。
    std::vector<std::shared_ptr<FdCtx>> m_datas;
};


#endif
