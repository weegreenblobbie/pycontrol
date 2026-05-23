# AI Agent Instructions and Project Context

You are an autonomous AI agent working on this repository. You MUST adhere strictly to the following rules at all times.

## 1. Code Editing & Patching Constraints
* **CRITICAL:** Do NOT write Python, `awk`, or `sed` scripts to edit, patch, or modify source code files.
* To make changes, use standard unified diffs, or rewrite the specific function/block entirely using standard file-writing tools. Do not attempt complex string-escaping manipulations.

## 2. C++ Backend (GNU Make)
The C++ project uses GNU Make at the top level. There is no Meson or CMake involved.
* **CRITICAL RULE:** Whenever you invoke `make`, you MUST ALWAYS use 4 threads (e.g., `make -j4`).

**Commands:**
* **To build the entire project:**
  * Run: `[INSERT COMMAND TO BUILD MAIN PROJECT, e.g., make -j4 all]`
* **To build the C++ unit tests:**
  * Run: `cd src/camera_control; make -j4`
* **To run the C++ unit tests:**
  * Run: `cd src/camera_control; make -j4 test`
  * *Note on Catch2:* If you need to abort the test suite on the first failure to save time during iterations, append the `-a` flag (e.g., `make -j4 test-a`).
* **To see full build commands:**
  * Specify `VERBOSE=1` make flag, (e.g. `make VERBOSE=1`)

## 3. Python/Flask Webapp
The frontend web application is built with Python and Flask.

**Commands:**
* **To build/setup the Flask webapp:**
  * Run: `cd webapp; make`
* **To run the Flask unit tests (including Playwright):**
  * Run: `cd webapp; make test`