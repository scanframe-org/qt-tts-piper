# Agent Instructions: C++ Project

## Overview

This is a modern Qt6 cross-platform 'Text-To-Speech' plugin project using CMake.
Qt6 only uses up to C++17.
License: GNU General Public License v3.0 (GPL-3.0).

## Commands

### Repository Initialization

- **Mandatory**: Run `./init-repo.py` to initialize submodules and handle symbolic links (especially on Windows).
- **Setup**: Run `./models.py --default` to download essential voice models for testing.

### Configuring, Building, Testing and Packaging

All targets are built from the command line using the single command `./build.py`.
When run without arguments, it will show the help to build using a given toolchain.

## Boundaries

Follow these operational safety guardrails:

- **Always do**: Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) for general resource management.
- **Always do**: Adhere strictly to RAII practices.
- **Never do**: Do not bypass explicit `noexcept` specifications on move constructors.
- **Qt Ownership**: For `QObject` derived classes, prefer parent-child ownership over smart pointers where applicable.
- **Logging**: Use `qCInfo`, `qCDebug`, `qCWarning`, `qCCritical` with the centralized `logCategory()` defined in
  `logging.h`.

## Code Style

Match the constraints configured and set in the file [`.clang-format`](./.clang-format).

### Standards & Patterns

- **Header Guards**: Always use `#pragma once`.
- **Doxygen**: Use Doxygen-style comments in header files only.
- **Bracing**: Use Allman style (braces on new lines).
- **Indentation**: Use tabs (size 2).
- **Qt Signals/Slots**: Use the modern function-pointer-based `connect()` syntax.
- **Utilities**: Leverage `helpers.h` for Qt-specific utility functions (e.g., `enumToString`).

### Naming Conventions

Follow the described code conventions from document [`doc/code-conventions.md`](doc/code-conventions.md).  
Key points:

- **Classes/Structs**: PascalCase.
- **Methods**: camelCase.
- **Members**: _camelCase (leading underscore).
- **Arguments/Variables**: lower_snake_case.

## Architecture

- **Plugin Path**: The TTS engine plugin must be installed into a `texttospeech` subdirectory within the Qt plugins
  directory for discovery.
- **Dependencies**: Be mindful of the `piper1-gpl` dependency which enforces the GPL-3.0 license.

## Testing Strategy

- **Rule**: Both testing frameworks **Catch2** and **GoogleTest** are the only allowed
  frameworks and preferably in that order.
- **Rule**: Core, headless and backend libraries have a `tests/` directory.

## Commit Style

Conventional commits are preferred as described in file ['cmake/lib/doc/semantic-versioning.md'](cmake/lib/doc/semantic-versioning.md).
