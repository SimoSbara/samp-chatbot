#pragma once
#include <string>
#include <vector>
#include <json.hpp>

#include "Global.h"
#include "ChatBot.hpp"
#include "SampHelper.h"

#include "curl/curl.h"

#ifdef WIN32

#ifdef _DEBUG
#pragma comment(lib, "libcurl_debug.lib")
#else
#pragma comment(lib, "libcurl.lib")
#endif

#endif

using json = nlohmann::json;

enum StatusCodes
{
	SUCCESS = 0,
	INVALID_MEMORY,
	INVALID_CHATBOT,
	FAILED_REQUEST,
	NUM_STATUS_CODES
};

class AIRequest
{
public:
	AIRequest()
	{
		this->id = -1;
	}

	AIRequest(int id, int chatbotID, int memoryID, std::string prompt)
	{
		this->id = id;
		this->chatbotID = chatbotID;
		this->memoryID = memoryID;
		this->prompt = prompt;
	}

	void Clear()
	{
		this->id = -1;
		this->prompt.clear();
	}

	std::string GetPrompt() const
	{
		return this->prompt;
	}

	int GetID() const
	{
		return this->id;
	}

	int GetChatBotID() const
	{
		return this->chatbotID;
	}

	int GetMemoryID() const
	{
		return this->memoryID;
	}

private:
	std::string prompt;
	int id; //request ID
	int chatbotID; //chatbot ID
	int memoryID; //memory ID
};

class AIResponse
{
public:
	AIResponse(int requestID, std::string prompt, std::string response)
	{
		this->requestID = requestID;
		this->prompt = prompt;
		this->response = response;
		this->statusCode = SUCCESS;
	}

	std::string GetPrompt()
	{
		return this->prompt;
	}

	void SetResponse(std::string response)
	{
		this->response = response;
	}

	std::string GetResponse()
	{
		return this->response;
	}

	int GetRequestID()
	{
		return this->requestID;
	}

	void SetStatusCode(StatusCodes value)
	{
		this->statusCode = value;
	}

	StatusCodes GetStatusCode() const
	{
		return this->statusCode;
	}

private:
	std::string prompt; //prompt from the player
	std::string response; //response of gpt
	int requestID; //ID of the original request
	StatusCodes statusCode;
};

class ChatBotHelper
{
public:
	static bool DoRequest(std::string& response, const std::string request, const ChatBot& chatBot, ChatMemory& memory)
	{
		CURL* curl = curl_easy_init();

		if (curl)
		{
			std::string curlResponse;
			curl_slist* headers = nullptr;

			headers = curl_slist_append(headers, "Content-Type: application/json");
			headers = curl_slist_append(headers, ("Authorization: Bearer " + chatBot.GetAPIKey()).c_str());

			std::string endPoint = chatBot.GetEndPoint();

			json requestDataDoc = CreateRequestDocument(request, chatBot, memory);

			std::string requestDataStr = requestDataDoc.dump();

			curl_easy_setopt(curl, CURLOPT_URL, endPoint.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestDataStr.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestDataStr.length());
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlResponse);

			if (globalParams.timeout > 0)
			{
				uint16_t timeout = globalParams.timeout;

				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
			}

			CURLcode res = curl_easy_perform(curl);

			if (res == CURLE_OK)
			{
				long code;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

				if (code != 200)
				{
					response = "Request Failed! Code: " + std::to_string(code);
					WriteErrorLog(response);
					return false;
				}

				try
				{
					nlohmann::json jresponse = nlohmann::json::parse(curlResponse);
					response = ParseBotAnswer(jresponse);

					//aggiungo nella memoria
					memory.AddUserMessage(request);
					memory.AddBotMessage(response);
				}
				catch (const std::exception& exc)
				{
					curl_easy_cleanup(curl);
					curl_slist_free_all(headers);

					response = "Request Parsing Failed: " + std::string(exc.what());
					WriteErrorLog(response);

					return false;
				}
			}
			else
			{
				response = "cURL Failed: " + std::to_string(res);
				WriteErrorLog(response);
			}

			curl_easy_cleanup(curl);
			curl_slist_free_all(headers);

			return res == CURLE_OK;
		}

		curl_easy_cleanup(curl);

		response = "cURL Init Failed!";
		WriteErrorLog(response);

		return false;
	}

private:
	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
	{
		size_t totalSize = size * nmemb;
		response->append((char*)contents, totalSize);
		return totalSize;
	}

	static std::string ParseBotAnswer(nlohmann::json response)
	{
		if (response.empty())
			return "";

		try
		{
			//controllo errori
			if (response.contains("error"))
			{
				try
				{
					std::string errMsg;
					if (response["error"].is_string())
						errMsg = response.at("error");
					else if (response["error"].is_object() && response["error"].contains("message"))
						errMsg = response.at("error").at("message");
					else
						errMsg = response.dump(0);

					return std::string("[ERROR] ") + errMsg;
				}
				catch (...)
				{
					std::string error = std::string("ParseBotAnswer() Unknown Exception: ") + " | " + response.dump(0);
					WriteErrorLog(error);
					return error;
				}
			}

			return response.at("choices").at(0).at("message").at("content");
		}
		catch (std::exception& exc)
		{
			std::string error = std::string("ParseBotAnswer() Exception: ") + exc.what() + " | " + response.dump(0);
			WriteErrorLog(error);

			return error;
		}

		return "";
	}

	static nlohmann::json CreateRequestDocument(const std::string request, const ChatBot& chatBot, const ChatMemory& memory)
	{
		json requestDoc;
		int index = 0;

		std::string systemPrompt = chatBot.GetSystemPrompt();

		requestDoc["model"] = chatBot.GetModel();

		if (!systemPrompt.empty())
		{
			requestDoc["messages"][index]["role"] = "system";
			requestDoc["messages"][index]["content"] = systemPrompt;
			index++;
		}

		//aggiungo memoria
		for (int i = 0; i < memory.GetSize(); i++)
		{
			Message message = memory.GetMessageFromIndex(i);

			requestDoc["messages"][index]["role"] = (message.isBot) ? "assistant" : "user";
			requestDoc["messages"][index]["content"] = message.msg;

			index++;
		}

		//ultima richiesta
		requestDoc["messages"][index]["role"] = "user";
		requestDoc["messages"][index]["content"] = request;
		requestDoc["temperature"] = 0;

		return requestDoc;
	}
};