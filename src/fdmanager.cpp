#include "fdmanager.h"

#include <sys/stat.h>
#include <fcntl.h>


// 模板实例化。FdManager 类有一个全局唯一的单例实例。
template class Singleton<FdManager>;

// 静态成员变量需要在类外部定义和初始化。
template <typename T>
T *Singleton<T>::instance = nullptr;

template <typename T>
std::mutex Singleton<T>::mutex;


bool FdCtx::init()
{
    // 如果已经初始化过了就直接返回 true。
    if (m_isInit) return true;

    // fstat 函数用于获取与文件描述符 m_fd 关联的文件状态信息存放到 statbuf 中。如果 fstat() 返回 -1，表示文件描述符无效或出现错误。
    struct stat statbuf;
    if (-1 == fstat(m_fd, &statbuf))
    {
        m_isInit = false;
        m_isSocket = false;
    }
    else
    {
        m_isInit = true;
        // S_ISSOCK(statbuf.st_mode) 用于检查 st_mode 中的位，以确定文件是否是一个套接字（socket）。该宏定义在 <sys/stat.h> 头文件中。
        m_isSocket = S_ISSOCK(statbuf.st_mode);
    }

    // 如果是一个套接字，设置为非阻塞。
    if (m_isSocket)
    {
        // 获取文件描述符的状态，检查当前标志中是否已经设置了非阻塞标志，如果没有则设置。
        int flags = fcntl(m_fd, F_GETFL, 0);
        if (!(flags & O_NONBLOCK)) fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);

        // 标记由框架（Hook）将 fd 设置为了非阻塞模式。
        m_sysNonblock = true;
    }
    else
    {
        m_sysNonblock = false;
    }


    return m_isInit; // 即初始化是否成功。
}

void FdCtx::setTimeout(int type, uint64_t v)
{
    // type 指定超时类型的标志。可能的值包括 SO_RCVTIMEO 和 SO_SNDTIMEO，分别用于接收超时和发送超时。v 代表设置的超时时间，单位是毫秒或者其他。
    // 如果 type 类型的读事件，则超时事件设置到 recvtimeout 上，否则就设置到 sendtimeout 上。
    if (SO_RCVTIMEO == type)
    {
        m_recvTimeout = v;
    }
    else
    {
        m_sendTimeout = v;
    }
}
