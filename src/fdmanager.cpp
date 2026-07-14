#include "fdmanager.h"


// 模板实例化。FdManager 类有一个全局唯一的单例实例。
template class Singleton<FdManager>;

// 静态成员变量需要在类外部定义和初始化。
template <typename T>
T *Singleton<T>::instance = nullptr;

template <typename T>
std::mutex Singleton<T>::mutex;
