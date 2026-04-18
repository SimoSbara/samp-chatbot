# samp-chatbot


## Description

A GTA SAMP plugin for Chat Bot communication.

It works for both [SA-MP 0.3.7/0.3.DL](https://www.sa-mp.mp/) and [open.mp](https://www.open.mp/).

**You can use any OpenAI compatible endpoint, even a local LLM server!**

Refer to this [wiki](https://github.com/SimoSbara/samp-chatbot/wiki) for pawn implementation.

### Side Note
Before choosing a Chat Bot Provider remember:
* GPT is not free to use, check [pricing](https://openai.com/api/pricing/);
* Gemini is free to use in [certain countries](https://ai.google.dev/gemini-api/docs/available-regions?hl=it);
* LLAMA is free on [groq.com](https://groq.com/) everywhere and has some premium features (LLAMA hasn't any official API);
* Doubao performs better in Chinese;
* DeepSeek is not free to use, check [pricing](https://api-docs.deepseek.com/quick_start/pricing).

## Troubleshooting
If you encounter any crash, make sure to update [VC++ Runtimes](https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/).

## Example of use
```c++
#include <a_samp>
#include <core>
#include <float>
#include <samp-chatbot>
#include <Pawn.CMD>
#include <sscanf2>

#define COLOR_RED 0xFF0000FF
#define COLOR_MAGENTA 0xFF00FFFF

#define CHATBOT_DIALOG   10

//Groq
#define GLOBAL_CHATBOT_ID       0
#define GLOBAL_API_KEY          "GROQ_API_KEY"
#define GLOBAL_ENDPOINT         "https://api.groq.com/openai/v1/chat/completions"
#define GLOBAL_MODEL            "llama-3.1-8b-instant"
#define GLOBAL_SYSTEMPROMPT     "You are inside a GTA SAMP Server"

//Gemini
#define PLAYER_CHATBOT_ID       1
#define PLAYER_API_KEY          "GEMINI_API_KEY"
#define PLAYER_ENDPOINT         "https://generativelanguage.googleapis.com/v1beta/interactions"
#define PLAYER_MODEL            "gemini-3-flash-preview"
#define PLAYER_SYSTEMPROMPT     "You are inside a GTA SAMP Server"

//generic memory
#define MEMORY_ID               0

#pragma tabsize 0

new lastResponses[MAX_PLAYERS][1024];
new lastGlobalResponse[1024];

new lastResponseID[MAX_PLAYERS];
new lastGlobalRequestID = -1;

main()
{
    SetChatBotGlobalEncoding(CHATBOT_W1252);
    SetChatBotGlobalTimeout(10000);
    SetChatBotGlobalLogMode(CHATBOT_LOG_ERRORS);

    CreateChatBotMemory(MEMORY_ID);

    //chatbot 1
    CreateChatBot(GLOBAL_CHATBOT_ID);
    SetChatBotModel(GLOBAL_CHATBOT_ID, GLOBAL_MODEL);
    SetChatBotEndPoint(GLOBAL_CHATBOT_ID, GLOBAL_ENDPOINT);
    SetChatBotAPIKey(GLOBAL_CHATBOT_ID, GLOBAL_API_KEY);
    SetChatBotSystemPrompt(GLOBAL_CHATBOT_ID, GLOBAL_SYSTEMPROMPT);
    
    //chatbot 2
    CreateChatBot(PLAYER_CHATBOT_ID);
    SetChatBotModel(PLAYER_CHATBOT_ID, PLAYER_MODEL);
    SetChatBotEndPoint(PLAYER_CHATBOT_ID, PLAYER_ENDPOINT);
    SetChatBotAPIKey(PLAYER_CHATBOT_ID, PLAYER_API_KEY);
    SetChatBotSystemPrompt(PLAYER_CHATBOT_ID, PLAYER_SYSTEMPROMPT);
}

CMD:clearmemory(playerid, params[])
{
    new id;

    if(sscanf(params, "d", id))
        return SendClientMessage(playerid, COLOR_RED, "/clearmemory <id>");

    ClearChatBotMemory(id);

    return 1;
}

CMD:bot(playerid, params[])
{
    new prompt[512];
    new memoryid;

    if(sscanf(params, "ds[512]", memoryid, prompt))
        return SendClientMessage(playerid, COLOR_RED, "/bot <memory (-1 no memory)> <prompt>");

    lastResponseID[playerid] = RequestToChatBot(PLAYER_CHATBOT_ID, memoryid, prompt);

    return 1;
}

CMD:botglobal(playerid, params[])
{
    new prompt[512];

    if(sscanf(params, "s[512]", prompt))
        return SendClientMessage(playerid, COLOR_RED, "/botglobal <prompt>");

    //no memory
    lastGlobalRequestID = RequestToChatBot(GLOBAL_CHATBOT_ID, -1, prompt);

    return 1;
}

CMD:lastglobalresponse(playerid)
{
	ShowPlayerDialog(playerid, CHATBOT_DIALOG, DIALOG_STYLE_MSGBOX, "Chat Bot Answer", lastGlobalResponse, "Ok", "");
    return 1;
}

CMD:lastresponse(playerid)
{
    ShowPlayerDialog(playerid, CHATBOT_DIALOG, DIALOG_STYLE_MSGBOX, "Chat Bot Answer", lastResponses[playerid], "Ok", "");
    return 1;
}

public OnChatBotResponse(prompt[], response[], requestid)
{
    printf("OnChatBotResponse() ID: %d | prompt: %s | response: %s", requestid, prompt, response);

    if(lastGlobalRequestID == requestid)
    {
        format(lastGlobalResponse, 2048, "%s", response);
        SendClientMessageToAll(COLOR_MAGENTA, "Chat Bot Responded Globally! Check it with /lastglobalresponse.");
    }
    else
    {
        new playerid = -1;

        for(new i = 0; i < MAX_PLAYERS; i++)
        {
            if(lastResponseID[i] == requestid)
            {
                playerid = i;
                break;
            }
        }

        if(playerid != -1)
        {
            format(lastResponses[playerid], 2048, "%s", response);
            SendClientMessage(playerid, COLOR_MAGENTA, "Chat Bot Responded! Check it with /lastresponse.");
        }
        else
        {
            new msg[512];
            format(msg, sizeof(msg), "Unknown Request ID: %d", requestid);
            SendClientMessageToAll(COLOR_MAGENTA, msg);
        }
    }
}

public OnChatBotError(requestid, const error[])
{
    printf("OnChatBotError() ID: %d | Message: %s", requestid, error);
    return 1;
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
This project is licensed under the MIT License - see the LICENSE file for details

## Acknowledgments
Project structure inspired from:
* [samp-dns-plugin by samp-incognito](https://github.com/samp-incognito/samp-dns-plugin)
