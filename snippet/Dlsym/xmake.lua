target("Dlsym-hook")
    set_kind("shared")
    add_files("hook.cpp")
    add_syslinks("dl")
target_end()


target("Dlsym1")
    set_kind("binary")
    add_files("main1.cpp")

    add_runenvs(
        "LD_PRELOAD",
        "$(projectdir)/build/$(plat)/$(arch)/$(mode)/snippet/libDlsym-hook.so"
    )
target_end()

target("Dlsym-hello")
    set_kind("shared")
    add_files("hello.cpp")
target_end()

target("Dlsym2")
    set_kind("binary")
    add_files("main2.cpp")

    add_runenvs(
        "HELLO_SO",
        "$(projectdir)/build/$(plat)/$(arch)/$(mode)/snippet/libDlsym-hello.so"
    )
target_end()
