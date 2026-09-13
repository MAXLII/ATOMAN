# Single-precision diagnostics for project C sources only. Keep vendor and
# assembly flags separate. Exceptions belong in paired pragmas in the owning .c.
# GCC does not diagnose every explicit double declaration with these warnings.
BASE_C_FP_WARNINGS := -Werror=double-promotion -Werror=unsuffixed-float-constants

ifneq ($(origin PROJECT_CFLAGS),undefined)
override PROJECT_CFLAGS += $(BASE_C_FP_WARNINGS)
else
override CFLAGS += $(BASE_C_FP_WARNINGS)
endif

ifneq ($(origin HOST_CFLAGS),undefined)
override HOST_CFLAGS += $(BASE_C_FP_WARNINGS)
endif
