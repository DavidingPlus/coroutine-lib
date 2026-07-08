#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <thread>


/**
 * @brief std::chrono 中两种常用时钟的区别。
 *
 * system_clock：
 *   - 表示系统当前时间（墙上时钟），可以转换为日期和时间。
 *   - 会受到手动修改系统时间、NTP 校时等影响，时间可能跳变或回退。
 *   - 适用于日志、时间戳、日期时间显示等场景。
 *
 * steady_clock：
 *   - 表示单调递增时钟，只关心时间流逝，不表示真实时间。
 *   - 不受系统时间调整影响，不会回退，适用于计时、定时器和超时检测。
 *
 * 本程序会同时输出两种时钟的结果，可在运行过程中手动修改系统时间，
 * 观察 system_clock 会发生跳变，而 steady_clock 始终保持单调递增。
 */
int main()
{
    auto systemStart = std::chrono::system_clock::now();
    auto steadyStart = std::chrono::steady_clock::now();

    while (true)
    {
        auto systemNow = std::chrono::system_clock::now();
        auto steadyNow = std::chrono::steady_clock::now();

        // system_clock 当前时间。
        std::time_t t = std::chrono::system_clock::to_time_t(systemNow);

        // 已经过了多久。
        auto systemElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(systemNow - systemStart).count();
        auto steadyElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(steadyNow - steadyStart).count();

        std::cout << "========================================\n";
        std::cout << "system_clock: " << std::put_time(std::localtime(&t), "%F %T") << std::endl;
        std::cout << "system elapsed: " << systemElapsed << " ms" << std::endl;
        std::cout << "steady elapsed: " << steadyElapsed << " ms" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
