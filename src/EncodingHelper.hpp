#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>

#include "SampHelper.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <iconv.h>
#include <errno.h>
#endif

enum Encodings
{
	UTF8 = 0, //in case the source code uses UTF8, it won't be converted
	W1252, //windows 1252 for latin
	GB2312, //GB2312 for simplified chinese
	W1251, //windows 1251 for cyrilic
	NUM_ENCODINGS
};

//IT: artigianale...
//EN: homemade
class EncodingHelper
{
public:
	//maledette lingue diverse!!! (da encoding scelto a utf8)
	static std::string ConvertToUTF8(std::string input, uint8_t inputEncoding)
	{
		if(inputEncoding == UTF8)
			return input;

#ifdef _WIN32
		int inputCodePage;

		switch (inputEncoding)
		{
		default:
		case W1252:
		{
			inputCodePage = 1252;
			break;
		}
		case GB2312: inputCodePage = 936; break;
		case W1251: inputCodePage = 1251; break;
		}

		//verifico se va bene
		int wideLen = MultiByteToWideChar(inputCodePage, 0, input.c_str(), input.size(), nullptr, 0);

		if (wideLen == 0)
		{
			logprintf("\n\nChatBot UTF8 conversion error: wideLen = 0\n");
			return "";
		}

		std::vector<wchar_t> utf16Str(wideLen);
		MultiByteToWideChar(inputCodePage, 0, input.c_str(), input.size(), utf16Str.data(), wideLen);

		//verifico se va bene da utf16 a utf8
		int utf8Len = WideCharToMultiByte(CP_UTF8, 0, utf16Str.data(), wideLen, nullptr, 0, nullptr, nullptr);
		
		if (utf8Len == 0) 
		{
			logprintf("\n\nChatBot UTF8 conversion error: utf8Len = 0\n");
			return "";
		}

		std::vector<char> utf8Str(utf8Len);
		WideCharToMultiByte(CP_UTF8, 0, utf16Str.data(), wideLen, utf8Str.data(), utf8Len, nullptr, nullptr);

		return std::string(utf8Str.begin(), utf8Str.end());
#else
		std::string inputCodePage;

		switch (inputEncoding)
		{
		default:
		case W1252:
		{
			inputCodePage = "WINDOWS-1252";
			break;
		}
		case GB2312: inputCodePage = "GB2312"; break;
		case W1251: inputCodePage = "WINDOWS-1251"; break;
		}

		iconv_t handle = iconv_open("UTF8", inputCodePage.c_str());

		if (handle == (iconv_t)-1)
		{
			logprintf("\n\nChatBot UTF8 conversion error: iconv_open failed\n");
			return "";
		}

		size_t outputLen = input.size() * 4; //per stare sereni
		char* output = new char[outputLen];

		if (!output)
		{
			iconv_close(handle);
			logprintf("\n\nChatBot UTF8 conversion error: output malloc failed\n");
			return "";
		}

		memset(output, 0, outputLen);

		char* inBuf = (char*)input.c_str();
		char* outBuf = output;
		size_t inBytesLeft = input.size();
		size_t outBytesLeft = outputLen;

		if (iconv(handle, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft) == (size_t)-1)
		{
			std::string err;

			if(errno == E2BIG)
				err = "There is not sufficient room at *outbuf.";
			else if(errno == EILSEQ)
				err = "An invalid multibyte sequence has been encountered in the input.";
			else if(errno == EINVAL)
				err = "An incomplete multibyte sequence has been encountered in the input.";
			else
				err = "Generic Error";

			logprintf("\n\nChatBot UTF8 conversion iconv failed: %s\n", err.c_str());
			delete[] output;
			iconv_close(handle);
			return "";
		}

		iconv_close(handle);

		std::string result(output, strlen(output));
		delete[] output;

		return result;
#endif
	}

	//gli accenti sono micidiali!!!! (da utf8 a l'encoding scelto)
	static std::string ConvertToWideByte(std::string input, uint8_t outputEncoding)
	{
		if(outputEncoding == UTF8)
			return input;

#ifdef _WIN32
		int outputCodePage;

		switch (outputEncoding)
		{
		default:
		case W1252:
		{
			outputCodePage = 1252;
			break;
		}
		case GB2312: outputCodePage = 936; break;
		case W1251: outputCodePage = 1251; break;
		}

		//verifico se va bene
		int wideLen = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), input.size(), nullptr, 0);

		if (wideLen == 0)
		{
			logprintf("\n\nChatBot WideByte conversion error: wideLen = 0\n");
			return "";
		}

		std::vector<wchar_t> utf16Str(wideLen);
		MultiByteToWideChar(CP_UTF8, 0, input.c_str(), input.size(), utf16Str.data(), wideLen);

		//verifico se va bene da utf16 a encoding scelto
		int encLen = WideCharToMultiByte(outputCodePage, 0, utf16Str.data(), wideLen, nullptr, 0, nullptr, nullptr);

		if (encLen == 0)
		{
			logprintf("\n\nChatBot WideByte conversion error: encLen = 0\n");
			return "";
		}

		std::vector<char> encStr(encLen);
		WideCharToMultiByte(outputCodePage, 0, utf16Str.data(), wideLen, encStr.data(), encLen, nullptr, nullptr);

		return std::string(encStr.begin(), encStr.end());
#else
		std::string outputCodePage;

		switch (outputEncoding)
		{
		default:
		case W1252:
		{
			outputCodePage = "WINDOWS-1252";
			break;
		}
		case GB2312: outputCodePage = "GB2312"; break;
		case W1251: outputCodePage = "WINDOWS-1251"; break;
		}

		iconv_t handle = iconv_open(outputCodePage.c_str(), "UTF8");

		if (handle == (iconv_t)-1)
		{
			logprintf("\n\nChatBot WideByte conversion error: iconv_open failed\n");
			return "";
		}

		size_t outputLen = input.size() * 4; //per stare sereni
		char* output = new char[outputLen];

		if (!output)
		{
			iconv_close(handle);
			logprintf("\n\nChatBot WideByte conversion error: output malloc failed\n");
			return "";
		}

		memset(output, 0, outputLen);

		char* inBuf = (char*)input.c_str();
		char* outBuf = output;
		size_t inBytesLeft = input.size();
		size_t outBytesLeft = outputLen;

		if (iconv(handle, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft) == (size_t)-1)
		{
			std::string err;

			if(errno == E2BIG)
				err = "There is not sufficient room at *outbuf.";
			else if(errno == EILSEQ)
				err = "An invalid multibyte sequence has been encountered in the input.";
			else if(errno == EINVAL)
				err = "An incomplete multibyte sequence has been encountered in the input.";
			else
				err = "Generic Error";

			logprintf("\n\nChatBot WideByte conversion iconv failed: %s\n", err.c_str());

			delete[] output;
			iconv_close(handle);
			return "";
		}

		iconv_close(handle);

		std::string result(output, strlen(output));
		delete[] output;

		return result;
#endif
	}
};
