target("EnableSharedFromThis1")
    set_kind("binary")
    add_files("main1.cpp")
target_end()

target("EnableSharedFromThis2")
    set_kind("binary")
    add_files("main2.cpp")
target_end()
