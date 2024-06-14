# samp-chatbot


## Description

A GTA SAMP plugin for Chat Bot communication.

The following Chat Bots API are implemented:
* [Chat GPT](https://platform.openai.com/docs/quickstart)
* [Gemini AI](https://ai.google.dev/)
* [LLAMA](https://groq.com/) 

This plugin can permit arbitrary model and system instruction choise using natives in runtime.

Refer to this [wiki](https://github.com/SimoSbara/samp-chatbot/wiki) for pawn implementation.

## Installation
* Download the last [Release](https://github.com/SimoSbara/samp-chatbot/releases).
* Put ```samp-chatbot.inc``` inside ```pawno/include``` folder.
  
### Only Windows
* Put ```samp-chatbot.dll``` inside ```plugins``` folder;
* Put ```libcurl.dll libcrypto-3.dll libss-3.dll``` inside the root server folder.
  
### Only Linux
* Put ```samp-chatbot.so``` inside ```plugins``` folder;
* Put ```libcurl.so.4 libcurl.so``` inside the root server folder.

## Development

### Compilation

#### Windows
Compiling on Windows is pretty simple, it requires Visual Studio 2022 with the latest C++ Windows SDK, libcurl is already provided.

#### Linux
In Linux (I only tried on Debian based systems) you need to cross-compile curl and openssl in x86 on x64 machine.

Steps:
* remove libcurl if it's already install and update with ```ldconfig```
* install cross-platform multilib: ```sudo apt install gcc-multilib g++-multilib```
* download [OpenSSL 3.3.1](https://github.com/openssl/openssl/releases/tag/openssl-3.3.1) and extract it
* go inside OpenSSL folder and configure on x86: ```setarch i386 ./config -m32```
* compile and install OpenSSL ```make``` and ```sudo make install```
* download [libcurl 8.8.0](https://github.com/curl/curl) and extract it
* configure libcurl by doing: ```./configure --host=i686-pc-linux-gnu --with-openssl CFLAGS=-m32 CC=/usr/bin/gcc```
* compile libcurl ```make``` and install it ```cd lib/``` and ```sudo make install```
* now you should have everything ready for compilation!

For compiling the samp-chatbot do ```make``` inside the main folder, binaries are inside bin/linux/Release.

## License
This project is licensed under the GNU General Public License v3.0 - see the LICENSE.md file for details

## Acknowledgments
Project structure inspired from:
* [samp-dns-plugin by samp-incognito](https://github.com/samp-incognito/samp-dns-plugin)
