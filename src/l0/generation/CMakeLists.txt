add_library(generation generator.h generator.cpp type_converter.h
                       type_converter.cpp generator_error.h generator_error.cpp)

target_link_libraries(generation common)
