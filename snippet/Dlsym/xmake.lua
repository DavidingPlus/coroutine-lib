target("Dlsym-hook")
    set_kind("shared")
    add_files("hook.cpp")
    add_syslinks("dl")
target_end()


target("Dlsym")
    set_kind("binary")
    add_files("main.cpp")

    add_runenvs(
        "LD_PRELOAD",
        "$(projectdir)/build/$(plat)/$(arch)/$(mode)/snippet/libDlsym-hook.so"
    )
target_end()
