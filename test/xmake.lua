if not get_config("with_gtest") then
    return
end

add_requires("gtest")

-- 请用 xmake test 执行测试。
target("tests")
    set_default(false)

    set_kind("binary")
    set_targetdir("$(builddir)/$(plat)/$(arch)/$(mode)/test")
    add_deps("coroutine-lib")
    add_packages("gtest")

    on_install(function () end)
    apply_current_platform_target_config()

    add_files("main.cpp")

    add_tests("testrun", {
        realtime_output = true,
        files = {"testrun/*.cpp"}
    })

    add_tests("coroutine-lib", {
        realtime_output = true,
        files = {"coroutine-lib/*.cpp"}
    })
target_end()
