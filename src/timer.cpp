#include "timer.h"


bool Timer::cancel()
{
}

bool Timer::refresh()
{
}

bool Timer::reset(uint64_t ms, bool fromNow)
{
}

TimerManager::TimerManager()
{
}

TimerManager::~TimerManager()
{
}

std::shared_ptr<Timer> TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring)
{
}

std::shared_ptr<Timer> TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weakCond, bool recurring)
{
}

uint64_t TimerManager::getNextTimer()
{
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs)
{
}

bool TimerManager::hasTimer()
{
}
