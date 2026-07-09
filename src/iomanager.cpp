#include "iomanager.h"


IOManager::FdContext::EventContext &IOManager::FdContext::getEventContext(Event event)
{
}

void IOManager::FdContext::resetEventContext(EventContext &ctx)
{
}

void IOManager::FdContext::triggerEvent(Event event)
{
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
{
}

IOManager::~IOManager()
{
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
}

bool IOManager::delEvent(int fd, Event event)
{
}

bool IOManager::cancelEvent(int fd, Event event)
{
}

bool IOManager::cancelAll(int fd)
{
}

void IOManager::tickle()
{
}

bool IOManager::stopping()
{
}

void IOManager::idle()
{
}

void IOManager::onTimerInsertedAtFront()
{
}

void IOManager::contextResize(size_t size)
{
}
