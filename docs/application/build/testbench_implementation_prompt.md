# Testbench Implementation Prompt

## Usage

Fill in the input block, then copy the complete prompt into a new AI conversation. Give the AI access to the target workspace and, when available, the accompanying Testbench usage guide.

## Input Block

```text
WORKSPACE_ROOT: <absolute or workspace-relative repository root>
TESTBENCH_ROOT: <target directory, for example platform/testbench>

INITIAL_DUT_NAME: <name used by registration and generated files>
INITIAL_DUT_HEADERS:
- <path or filename>

INITIAL_DUT_SOURCES:
- <path>

INITIAL_DUT_DEPENDENCIES:
- <source or include path>

DUT_RUN_PERIOD_S: <period in seconds>

TEST_CASES:
- <case name>: <environment, stimulus, completion rule, and verdict requirement>

CSV_REQUIREMENTS:
- <case name>: <columns and recording condition, or none>

ADDITIONAL_CONSTRAINTS:
- <project-specific constraints, or none>
```

## Copyable Prompt

```text
You are responsible for designing, implementing, and validating a GCC host Testbench from zero in the supplied repository. Do not stop at architecture notes or pseudocode. Deliver the working common framework, build environment, initial DUT integration, test cases, and verification results.

Inputs
------

WORKSPACE_ROOT: <WORKSPACE_ROOT>
TESTBENCH_ROOT: <TESTBENCH_ROOT>

INITIAL_DUT_NAME: <INITIAL_DUT_NAME>
INITIAL_DUT_HEADERS:
<INITIAL_DUT_HEADERS>

INITIAL_DUT_SOURCES:
<INITIAL_DUT_SOURCES>

INITIAL_DUT_DEPENDENCIES:
<INITIAL_DUT_DEPENDENCIES>

DUT_RUN_PERIOD_S: <DUT_RUN_PERIOD_S>

TEST_CASES:
<TEST_CASES>

CSV_REQUIREMENTS:
<CSV_REQUIREMENTS>

ADDITIONAL_CONSTRAINTS:
<ADDITIONAL_CONSTRAINTS>

Before editing
--------------

1. Read all repository instructions that apply to the target files.
2. Inspect the DUT headers, sources, initialization contract, runtime entry, and real dependencies.
3. Inspect the current repository layout and existing build conventions.
4. If Testbench files already exist, inspect their actual contents and preserve compatible behavior instead of replacing user work blindly.
5. Read the supplied Testbench usage guide when it is available and treat its lifecycle and registration contract as authoritative.

Required architecture
---------------------

1. Use DUT to mean Design Under Test. A DUT may be a function, module, algorithm, or complete software subsystem.
2. Place the reusable framework in TESTBENCH_ROOT/common:
   - testbench.h: public descriptors, states, and registration macros.
   - testbench.cpp: section discovery, runtime list construction, execution, reporting, and main().
3. Place each DUT test project in its own directory beside common.
4. Compile the real DUT from its original location. Do not copy or move production source or header files into the test directory.
5. Reuse all host-compatible production dependencies. Create a stub only at a hardware-specific boundary that cannot execute on the host, and keep that stub minimal.
6. Support C and C++ DUTs. Compile each source with an appropriate compiler and language standard, then perform the final link with a C++ compiler driver because the common runner uses the C++ standard library.
7. Generate one executable for all compatible modules and cases linked by the test project.

Public registration contract
----------------------------

Implement these file-scope declaration macros:

TESTBENCH_REGISTER(test_name, period_s, dut_init, dut_run)
TESTBENCH_CASE(test_name, case_name, init, before_dut, after_dut)

Requirements:

1. TESTBENCH_REGISTER registers one DUT module, its positive run period in seconds, its initialization callback, and its per-period body callback.
2. TESTBENCH_CASE registers one independently initialized case below its owning module.
3. Module descriptors reside in the testbench_module linker section.
4. Case descriptors reside in a module-specific testbench_case_<test_name> linker section.
5. Use GNU attributes that retain and align every descriptor.
6. Generate descriptor symbols with token concatenation so module symbols include test_name and case symbols include both test_name and case_name.
7. Both macros expand to file-scope structure declarations. Put the terminating semicolon inside each macro and do not require a semicolon at the invocation site.
8. Keep function pointers directly in the descriptor structures. Do not create typedef aliases solely for function-pointer types.
9. Define each tagged structure and its _t alias in one declaration. Use the structure tag for self-referential next pointers.

Case states
-----------

Define TESTBENCH_CASE_STATE_E with these constants:

- TESTBENCH_CASE_RUNNING: continue with the next beat.
- TESTBENCH_CASE_COMPLETE: finish normally without a PASS or FAIL verdict.
- TESTBENCH_CASE_PASS: finish with a passing verdict.
- TESTBENCH_CASE_FAIL: finish with a failing verdict.

Enumeration typedef names use upper-case snake case and end in _E. Enumeration constants use upper-case snake case and do not end in _E.

Runtime contract
----------------

For every registered case, execute this order:

1. case init
2. DUT init
3. beat 1 at time Ts:
   - before_dut(Ts)
   - dut_run()
   - after_dut(Ts)
4. beat 2 at time 2 * Ts, then continue with the same order.

Additional requirements:

1. Reserve beat 0 for initialized conditions. If a case needs a beat-0 CSV row, open the file in case init and record time_s = 0.0 at the end of DUT init.
2. Initialize run_count to 1 before the execution loop. Increment it only after a RUNNING result, then calculate the next elapsed time.
3. Call DUT init separately for every test case.
4. before_dut and after_dut both receive elapsed simulated time in seconds.
5. Use before_dut for processing that must occur before the current DUT execution, including delayed plant response, scheduled input changes, input preparation, or pre-DUT recording.
6. Use after_dut for processing that follows the current DUT output, including immediate environment response, output recording, completion checks, and verdict calculation.
7. Continue only when after_dut returns RUNNING. Accept only COMPLETE, PASS, or FAIL as terminal states; treat any other value as FAIL.
8. Validate the module period and every required callback before execution. Express invalid conditions as an OR chain so any missing callback invalidates the case.
9. Do not add a runner timeout. Every case owns and implements its real completion condition.
10. COMPLETE is counted separately from PASS and FAIL and does not make the process fail.
11. Return process success when no case failed; return process failure when any case failed.
12. Use C++ stream output in testbench.cpp rather than printf-family output.
13. Do not use dynamic memory. Build module and case linked lists from GNU linker-section boundaries.

Test case design
----------------

1. Define a fixture as the complete test context containing the DUT instance, inputs, outputs, simulated environment state, recording resources, and case-local result state.
2. Reset the complete fixture in every case init.
3. Implement the requested stimulus and environment timing from TEST_CASES.
4. Use time_s for scheduled changes instead of adding a sample index solely to trigger an event.
5. Use if/else for procedural state changes. Reserve the ternary operator for a concise, side-effect-free selection between two values.
6. A case that requires a verdict returns PASS or FAIL after its checks. A case intended only to collect or inspect results may return COMPLETE.
7. Register every case independently so each case receives its own case init and DUT init sequence.

CSV recording
-------------

1. Let each case own its CSV file and column selection.
2. Store generated CSV files under the test project's build directory.
3. Use time_s as the first column for time-domain data.
4. Recording may occur every beat, every N beats, on a value change, when a threshold is crossed, or when a frequency point completes.
5. Open and write the header during case init, optionally record beat 0 during DUT init, record later samples in before_dut or after_dut according to signal timing, and close the file before returning a terminal state.

Build environment
-----------------

Create one simple Makefile per test project.

1. Put project directory variables, C_SOURCES, CXX_SOURCES, and INCLUDE_DIRS inside a clearly marked User modification section.
2. Derive object lists from the source lists and use vpath plus common C and C++ pattern rules.
3. Compile the common runner exactly once.
4. Use G++ for the final link.
5. Provide a default build and a test target.
6. Before every default build or test run, remove existing .o and .exe files, but preserve .csv and .log files.
7. Ensure BUILD_DIR exists before the pre-build removal step so the first build in a new directory succeeds.
8. Keep the Makefile focused on compiler selection, source paths, include paths, compile/link rules, pre-build removal, and execution.
9. Keep source basenames unique because derived object filenames use the basename.

Implementation workflow
-----------------------

1. Create the common framework first and compile it with one minimal registered module and case.
2. Integrate the initial real DUT and its dependencies.
3. Implement every requested test case and CSV output.
4. Build from a clean, previously nonexistent build directory to verify first-build behavior.
5. Run the test target and inspect the module/case summary, exit code, and generated CSV timing.
6. Fix all build errors and test failures that are within scope. Do not report completion while required work remains.
7. Preserve unrelated local changes. Do not commit unless explicitly requested.

Acceptance criteria
-------------------

1. The common runner discovers registered modules and their module-specific cases through GNU linker sections.
2. Every test case receives case init followed by a fresh DUT init.
3. Beat 1 uses time Ts, and time advances only after a RUNNING result.
4. COMPLETE, PASS, and FAIL are reported and summarized correctly.
5. One executable runs every compatible linked case.
6. The Makefile succeeds when build/ does not exist and rebuilds .o/.exe on every invocation without deleting CSV or log files.
7. All requested DUT cases execute to a terminal state.
8. Required CSV files contain the intended time and signal relationship.
9. The final response lists created or modified files, exact build/test commands, case results, and any remaining limitation.
```
