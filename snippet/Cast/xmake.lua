add_requires("gtest")

target("Cast-StaticCast")
    set_kind("binary")
    add_files("main1.cpp")

    add_packages("gtest")
target_end()
