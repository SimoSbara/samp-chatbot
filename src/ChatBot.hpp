#pragma once
#include <string>
#include <vector>
#include <json.hpp>

struct Message
{
	std::string msg;
	bool isBot;
};

class ChatMemory
{
public:
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

private:
	std::vector<Message> messages;
};


class ChatBot
{
public:
	void SetSystemPrompt(const std::string systemPrompt)
	{
		this->systemPrompt = systemPrompt;
	}

	std::string GetSystemPrompt() const
	{
		return this->systemPrompt;
	}

	void SetModel(const std::string model)
	{
		this->model = model;
	}

	std::string GetModel() const
	{
		return this->model;
	}

	void SetEndPoint(const std::string endPoint)
	{
		this->endPoint = endPoint;
	}

	std::string GetEndPoint() const
	{
		return this->endPoint;
	}

	void SetAPIKey(const std::string apiKey)
	{
		this->apiKey = apiKey;
	}

	std::string GetAPIKey() const
	{
		return this->apiKey;
	}

private:
	std::string endPoint;
	std::string model;
	std::string systemPrompt;
	std::string apiKey;
};