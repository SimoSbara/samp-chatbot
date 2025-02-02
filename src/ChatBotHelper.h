#pragma once
#include <string>
#include <vector>
#include <json.hpp>

#include "curl/curl.h"

using json = nlohmann::json;

enum ChatBots
{
	GPT = 0,
	GEMINI,
	LLAMA,
	DOUBAO,
	NUM_CHAT_BOTS
};

class AIRequest
{
public:
	AIRequest()
	{
		this->id = -1;
	}

	AIRequest(int id, std::string prompt)
	{
		this->id = id;
		this->prompt = prompt;
	}

	void Clear()
	{
		this->id = -1;
		this->prompt.clear();
	}

	std::string GetPrompt()
	{
		return this->prompt;
	}

	int GetID()
	{
		return this->id;
	}

private:
	std::string prompt;
	int id; //generic ID
};

class AIResponse
{
public:
	AIResponse(int id, std::string prompt, std::string response)
	{
		this->id = id;
		this->prompt = prompt;
		this->response = response;
	}

	std::string GetPrompt()
	{
		return this->prompt;
	}

	std::string GetResponse()
	{
		return this->response;
	}

	int GetID()
	{
		return this->id;
	}

private:
	std::string prompt; //prompt from the player
	std::string response; //response of gpt
	int id; //ID of the original request
};

struct Message
{
	std::string msg;
	bool isBot;
};

struct ChatMemory
{
	std::vector<Message> messages;
	
	inline void Clear()
	{
		messages.clear();
	}

	inline bool IsEmpty() const
	{
		return messages.size() == 0;
	}

	inline int GetSize() const
	{
		return messages.size();
	}

	Message GetMessageFromIndex(int index) const
	{	
		if (index < 0 || index >= GetSize())
			return Message();

		return messages[index];
	}

	void AddBotMessage(std::string msg)
	{
		Message m;
		m.msg = msg;
		m.isBot = true;

		messages.push_back(m);
	}

	void AddUserMessage(std::string msg)
	{
		Message m;
		m.msg = msg;
		m.isBot = false;

		messages.push_back(m);
	}
};

struct ChatBotParams
{
	std::string apikey;
	std::string systemPrompt;
	std::string model;
	int botType; //enum ChatBots
	int encoding; //enum Encodings
};

class ChatBotHelper
{
public:
	static bool DoRequest(std::string& response, const std::string request, const ChatBotParams& params, ChatMemory& memory);

private:
	static std::string GetBotAnswer(const ChatBotParams& params, nlohmann::json response);

	static nlohmann::json CreateRequestDocument(const std::string request, const ChatBotParams& params, const ChatMemory& memory);

	static curl_slist* GetHeader(const ChatBotParams& params);
	static std::string GetURL(const ChatBotParams& params);
};