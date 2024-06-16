# samp-chatbot


## Description

A GTA SAMP plugin for Chat Bot communication.

It works for both SAMP and [open.mp](https://www.open.mp/).

The following Chat Bots API are implemented:
* [Chat GPT](https://platform.openai.com/docs/quickstart)
* [Gemini AI](https://ai.google.dev/)
* [LLAMA](https://groq.com/) 

This plugin can permit arbitrary model and system instruction choise using natives in runtime.

Refer to this [wiki](https://github.com/SimoSbara/samp-chatbot/wiki) for pawn implementation.

### Side Note
Before choosing a Chat Bot API remember:
* GPT is not free to use, it requires some [credits](https://openai.com/api/pricing/) that you need to purchase
* Gemini is free to use in [certain countries](https://ai.google.dev/gemini-api/docs/available-regions?hl=it)
* LLAMA is free on https://groq.com/ everywhere and has some premium features (LLAMA hasn't any official API)


## Example of use
```
#include <a_samp>
#include <core>
#include <float>
#include <samp-chatbot>
#include <pawncmd>
#include <sscanf2>

#define COLOR_RED 0xFF0000FF
#define COLOR_MAGENTA 0xFF00FFFF

#define CHATBOT_DIALOG   10
#define API_KEY          "MY_API_KEY"

#pragma tabsize 0

new lastResponses[MAX_PLAYERS][1024];

main()
{
    SelectChatBot(LLAMA);
    SetModel("llama3-70b-8192");
    SetAPIKey(API_KEY);
    SetSystemPrompt("You are an assistant inside GTA SAMP");
}

CMD:disablesysprompt(playerid, params[])
{
    SetSystemPrompt("");

    return 1;
}

CMD:sysprompt(playerid, params[])
{
    new sysPrompt[512];

    if(sscanf(params, "s[512]", sysPrompt))
        return SendClientMessage(playerid, COLOR_RED, "/sysprompt <system_prompt>");

    SetSystemPrompt(sysPrompt);

    return 1;
}

CMD:bot(playerid, params[])
{
    new prompt[512];

    if(sscanf(params, "s[512]", prompt))
        return SendClientMessage(playerid, COLOR_RED, "/bot <prompt>");

    RequestToChatBot(prompt, playerid);

    return 1;
}

CMD:lastresponse(playerid, params[])
{
    ShowPlayerDialog(playerid, CHATBOT_DIALOG, DIALOG_STYLE_MSGBOX, "Chat Bot Answer", lastResponses[playerid], "Ok", "");
    return 1;
}

public OnChatBotResponse(prompt[], response[], playerid)
{
    format(lastResponses[playerid], 1024, "%s", response);

    SendClientMessage(playerid, COLOR_MAGENTA, "Chat Bot Responded! Check it with /lastresponse.");
}
```
## Installation
* Download the last [Release](https://github.com/SimoSbara/samp-chatbot/releases).
* Put ```samp-chatbot.inc``` inside ```pawno/include``` folder.
  
### Only Windows
* Put ```samp-chatbot.dll``` inside ```plugins``` folder;
* Put ```libcurl.dll libcrypto-3.dll libss-3.dll``` inside the root server folder.
  
### Only Linux
* Put ```samp-chatbot.so``` inside ```plugins``` folder.

## Development

### Compilation

#### Windows
Compiling on Windows is pretty simple, it requires Visual Studio 2022 with the latest C++ Windows SDK, libcurl is already provided.

#### Linux
In Linux (I only tried on Debian based systems) you need to do ```make clean``` and ```make``` inside the main folder, ```libcurl libssl libcrypto``` are already provided.

If you want to compile curl and openssl yourself you will need to cross-compile them in 32 bit on 64 bit machine.

Steps:
* remove libcurl and OpenSSL if it's already install and update with ```ldconfig``` otherwise it will create conflicts!
* install cross-platform multilib: ```sudo apt install gcc-multilib g++-multilib```
* download [OpenSSL 3.3.1](https://github.com/openssl/openssl/releases/tag/openssl-3.3.1) and extract it
* go inside OpenSSL folder and configure on x86: ```setarch i386 ./config -m32```
* compile and install OpenSSL ```make``` and ```sudo make install```
* download [libcurl 8.8.0](https://github.com/curl/curl) and extract it
* configure libcurl by doing: ```./configure --host=i686-pc-linux-gnu --with-openssl CFLAGS=-m32 CC=/usr/bin/gcc```
* compile libcurl ```make``` and install it ```cd lib/``` and ```sudo make install```
* find and copy ```libcurl.a libcrypto.a libssl.a``` inside ```samp-chatbot-root-folder/lib```
* now you should have everything ready for compilation!

For compiling the samp-chatbot do ```make clean``` and ```make``` inside the main folder, binaries are inside bin/linux/Release.

## License
This project is licensed under the GNU General Public License v3.0 - see the LICENSE.md file for details

## Acknowledgments
Project structure inspired from:
* [samp-dns-plugin by samp-incognito](https://github.com/samp-incognito/samp-dns-plugin)
