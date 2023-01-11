add_library(StbImage stb/stb_image.h stb/impl.c)
target_include_directories(StbImage PUBLIC stb)