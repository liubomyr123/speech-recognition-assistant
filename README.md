# PocketSphinx Speech Recognition

## Overview

This project implements a basic speech recognition system using [PocketSphinx](https://github.com/cmusphinx/pocketsphinx). Currently, it serves as a template that captures audio input, converts it into text, and demonstrates command execution using recognized speech.

## Installation

Follow these steps to install and build the project:

1. Clone the repository:

   ```sh
   cd ~
   git clone https://github.com/cmusphinx/pocketsphinx.git
   ```

2. Install dependencies:

   ```sh
   sudo apt install \
        ffmpeg \
        libasound2-dev \
        libportaudio2 \
        libportaudiocpp0 \
        libpulse-dev \
        libsox-fmt-all \
        portaudio19-dev \
        sox
   ```

3. Build and install PocketSphinx:

   ```sh
   mkdir build
   cd build
   cmake ..
   cmake --build .
   sudo cmake --build . --target install
   ```

## Build and Run

### Build the Program

```sh
make
```

### Run the Program

```sh
./live
```

### Clean the Build

```sh
make clean
```

## Features

- Uses PocketSphinx for speech recognition.
- Captures audio input via SoX.
- Converts speech to text.
- Demonstrates command execution using recognized phrases.
- Example commands:
  - Saying "browser" opens a web browser.
  - Saying "browser exit" closes the browser.
- The system is designed as a foundation for further development and testing of voice commands.

## Dependencies

- [PocketSphinx](https://github.com/cmusphinx/pocketsphinx)
- SoX (for audio capture)
- PortAudio (for real-time audio processing)

## Additional Resources

- **Github**: [https://github.com/cmusphinx/pocketsphinx](https://github.com/cmusphinx/pocketsphinx)
- **Official documentation**: [https://cmusphinx.github.io](https://cmusphinx.github.io)
- **Documentation for C language**: [https://cmusphinx.github.io/doc/pocketsphinx/index.html](https://cmusphinx.github.io/doc/pocketsphinx/index.html)

