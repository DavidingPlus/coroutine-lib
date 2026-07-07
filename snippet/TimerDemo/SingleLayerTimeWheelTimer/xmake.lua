add_requires("gtest")

target("SingleLayerTimeWheelTimer")
    set_kind("binary")
    add_files("*.cpp")
    add_includedirs("$(builddir)/config/")

    add_packages("gtest")
target_end()
