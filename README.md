# Piper1GPL Text-to-Speech Qt6 Plugin

This project provides a [Qt6 Text-to-Speech (TTS)](https://doc.qt.io/qt-6/qttexttospeech-index.html) engine plugin
that uses the [Piper](https://github.com/rhasspy/piper) TTS library for high-quality, local neural speech synthesis.

## Overview

`qt-tts-piper` enables Qt6 applications to use Piper as a TTS backend.  
It implements the `QTextToSpeechEngine` interface, allowing it to be used seamlessly with the `QTextToSpeech` class.

### Key Features

*   **Qt6 Integration**: Full compatibility with `QTextToSpeech` and the Qt6 Multimedia module.
*   **Piper Backend**: High-quality, fast, and local neural text-to-speech synthesis.
*   **Multiple Voices**: Supports various Piper models/voices (ONNX format).
*   **Capabilities**: Supports speaking, pausing, resuming, and word-by-word progress reporting.
*   **Cross-Platform**: Built using CMake, targeting environments where Qt6 and Piper are supported.

## Project Structure

*   `src/engine`: The core implementation of the Qt6 TTS plugin.
*   `src/tests`: Includes both CLI and Qt-based test applications to verify the engine's functionality.
*   `models.sh`: A script to download sample Piper voice models from Hugging Face.

## Requirements

*   **Qt 6.6+**: Specifically `QtTextToSpeech` and `QtMultimedia` modules.
*   **Piper**: Integrated as a submodule or external library (specifically `piper1-gpl`).
*   **espeak-ng**: Used for phonemization.
*   **CMake 3.25+**: For building the project.

## Getting Started

### 1. Build the Project

First, initialize the repository by downloading the submodules and converting 
symbolic links when running on Windows:

```shell
./init-repo.py
```

Use CMake to configure and build the project:

```shell
cmake --preset gnu-debug
cmake --build --preset gnu-debug
```

A Python build script named `build.py` is provided in the project root to automate the process.  
It will download the required dependencies and build the project.  
It also allows building the project with a special Docker image that contains all the dependencies and tools.  
The tools incorporated in the Docker image allow building the project for Windows.

```shell
# On Linux, configure and build using the native GNU.
./build.py -mb gnu-debug
# On Linux, configure and build using the MinGW cross-compiler.
./build.py -mb gw-debug
# On Windows, configure and build using the MinGW compiler.
./build.py -mb mingw-debug
# On Windows, configure and build using the MSNC compiler.
./build.py -mb gw-debug
# On Linux, configure and build using the Docker and the MSVC compiler using Wine.
./build.py docker -- wine -- -mb msvc-debug
```

> Many more options are available.

### 2. Download Models

You can use the provided script to download some initial voice models into the 
default `bin/models` directory:

```shell
./models.py --default
```
Since it is not possible to find out which models are Female or Male a `models.ini` can be 
added to the `bin/models` directory. So the voices in Qt are not listed as "Unknown Gender". 

```ini
[en_GB-cori-high.onnx]
gender_0=Female

[en_US-hfc_female-medium.onnx]
gender_0=Female

[en_US-hfc_male-medium.onnx]
gender_0=Male
```

### 3. Usage as a Plugin

Once built, the plugin library (`libsf-tts-piper.so` on Linux) should be placed in the `texttospeech` subdirectory
of your Qt plugins path so that `QTextToSpeech` can discover and load it.

Example of selecting the engine in C++:

```cpp
QTextToSpeech *tts = new QTextToSpeech("piper");
```

## License

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**.  
For a summary and the full text of the license, see the [LICENSE.md](LICENSE.md) file or the [online version](https://www.gnu.org/licenses/gpl-3.0.html).

This project is part of the Scanframe library and incorporates components under the GPL license (due to the `piper1-gpl` dependency).
