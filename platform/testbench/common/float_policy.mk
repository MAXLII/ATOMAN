# Resolve the test source boundary before including the production policy.
BASE_TESTBENCH_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
include $(dir $(lastword $(MAKEFILE_LIST)))../../gcc_float_policy.mk

# Apply exceptions per translation unit, never to a whole executable that also
# links production C sources. Source paths outside testbench retain the errors.
testbench_cflags = $(CFLAGS) $(if $(filter $(BASE_TESTBENCH_ROOT)/%,$(abspath $(1))),-Wno-double-promotion -Wno-unsuffixed-float-constants)
