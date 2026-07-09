add_requires("gtest")

target("Cast-StaticCast")
    set_kind("binary")
    add_files("main1.cpp")

    add_packages("gtest")
target_end()

target("Cast-ConstCast")
    set_kind("binary")
    add_files("main2.cpp")

    add_packages("gtest")
target_end()

target("Cast-ReinterpretCast")
    set_kind("binary")
    add_files("main3.cpp")

    add_packages("gtest")
target_end()

target("Cast-DynamicCast")
    set_kind("binary")
    add_files("main4.cpp")

    add_packages("gtest")
target_end()
