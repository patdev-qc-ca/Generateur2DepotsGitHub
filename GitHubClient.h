#pragma once
#include <string>
class GitHubClient
{
public:
	GitHubClient(const std::wstring& token);
	bool CreateRepository(const std::wstring& name, const std::wstring& description, bool isPrivate, std::wstring& outResponse);
private:
	std::wstring m_token;
};
