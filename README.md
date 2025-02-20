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

### Build with other languages: RU/DE

```sh
make MODEL_DE=1

# or

make MODEL_RU=1
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
- **Acoustic and Language Models**: [https://sourceforge.net/projects/cmusphinx/files/Acoustic%20and%20Language%20Models/](https://sourceforge.net/projects/cmusphinx/files/Acoustic%20and%20Language%20Models/)

## Language models
Go to **Acoustic and Language Models** website and download language models:
   - 'language model'
      Examples: `cmusphinx-voxforge-de.lm.bin` `ru.lm`
   - 'dictionary':
      Examples: `cmusphinx-voxforge-de.dic` `ru.dic`
   - 'hidden markov model':
      Examples: `voxforge.cd_ptm_5000` `zero_ru.cd_cont_4000`

Save them in one of the folders: `MODEL_DE`, `MODEL_RU`, `MODEL_UA`...

You can also find these models here: [https://drive.google.com/drive/folders/1UAlBpDFsMTmH69C1u_6yrGoLEp1GN9ov](https://drive.google.com/drive/folders/1UAlBpDFsMTmH69C1u_6yrGoLEp1GN9ov)

Then rename them into `dictionary.dic`, `hmm`, `language_model.lm` or if some of them are binary: `dictionary.dic.bin`, `language_model.lm.bin`.

Then use them inside `load_modals()` function like this:

```c
#ifdef MODEL_UA
    printf("\n");
    printf("✅ MODEL_UA flag is defined.\n");
    if (check_models_files(&is_run_default_setup, "MODEL_UA") == 1)
    {
        printf("Loading...\n");
    }
    printf("\n");
#endif

#ifdef MODEL_DE
    printf("\n");
    printf("✅ MODEL_DE flag is defined.\n");
    if (check_models_files(&is_run_default_setup, "MODEL_DE") == 1)
    {
        printf("Loading...\n");
    }
    printf("\n");
#endif
```

Here instead of `Loading...`:
```c
  printf("Loading...\n");
```

Load real models:
```c
  ps_config_set_str(speech_config, "hmm", "MODEL_DE/hmm");
  ps_config_set_str(speech_config, "lm", "MODEL_DE/language_model.lm.bin");
  ps_config_set_str(speech_config, "dict", "MODEL_DE/dictionary.dic");
```
