# Project C sources use single precision by default. Necessary host/ABI code may
# locally suppress these diagnostics with paired GCC diagnostic push/pop in .c.
# These diagnostics reject promotions and unsuffixed literals, not every explicit
# double declaration. Do not use -fsingle-precision-constant to change C semantics.
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:C>:-Werror=double-promotion>"
        "$<$<COMPILE_LANGUAGE:C>:-Werror=unsuffixed-float-constants>"
    )
endif()
