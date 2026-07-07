add_requires("gtest")

target("AscendingOrderLinkedListTimerTest")
    set_kind("binary")
    add_files("*.cpp")

    add_packages("gtest")
target_end()
