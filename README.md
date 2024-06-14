# samp-chatbot


## Description

A GTA SAMP plugin for Chat Bot communication.

The following Chat Bots API are implemented:
* [Chat GPT](https://platform.openai.com/docs/quickstart)
* [Gemini AI](https://ai.google.dev/)
* [LLAMA](https://groq.com/) 

This plugin can permit arbitrary model and system instruction choise using natives in runtime.

Refer to this [wiki](https://github.com/SimoSbara/samp-chatbot/wiki) for pawn implementation.

## Getting Started

### Compiling

* If you want to compile the project on Windows, you will need Visual Studio 2022.
* If you want to compile the project on Linux, you will need to execute "make" in the global folder.
* Otherwise, there are also pre-compiled binaries in [Releases](https://github.com/SimoSbara/samp-chatbot/releases).

### Installing

* Put samp-chatbot.dll or .so in plugins/ samp server folder.
* Put samp-chatbot.inc in pawno/include/ samp server folder.

## Only for windows
* Compile [curl](https://github.com/curl/curl) with OpenSSL or get precompiled binaries
* Put dlls inside the root SAMP Server folder.

## Only for linux debian based
* Install libcurl ```sudo apt-get install -y libcurl-dev```

## Authors
[@SimoSbara](https://github.com/SimoSbara)

## License
This project is licensed under the GNU General Public License v3.0 - see the LICENSE.md file for details

## Acknowledgments

Project structure inspired from:
* [samp-dns-plugin by samp-incognito](https://github.com/samp-incognito/samp-dns-plugin)
