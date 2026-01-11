#include "GitHubClient.h"
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

GitHubClient::GitHubClient(const std::wstring& token)
    : m_token(token)
{
}

bool GitHubClient::CreateRepository(const std::wstring& name, const std::wstring& description, bool isPrivate, std::wstring& outResponse)
{
    HINTERNET hSession = WinHttpOpen(L"Win32 GitHub Client/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession)
        return false;

    HINTERNET hConnect = WinHttpConnect(hSession, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"POST", L"/user/repos",
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // JSON body
    std::wstring json = L"{\"name\":\"" + name +
        L"\",\"description\":\"" + description +
        L"\",\"private\":" + (isPrivate ? L"true" : L"false") + L"}";

    // Headers
    std::wstring authHeader = L"Authorization: token " + m_token;
    WinHttpAddRequestHeaders(hRequest, authHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);

    WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/json", -1, WINHTTP_ADDREQ_FLAG_ADD);

    BOOL bResult = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        (LPVOID)json.c_str(),
        (DWORD)(json.size() * sizeof(wchar_t)),
        (DWORD)(json.size() * sizeof(wchar_t)),
        0);

    if (!bResult)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    WinHttpReceiveResponse(hRequest, NULL);

    DWORD dwSize = 0;
    std::wstring response;

    do
    {
        WinHttpQueryDataAvailable(hRequest, &dwSize);
        if (dwSize == 0)
            break;

        wchar_t* buffer = new wchar_t[dwSize / sizeof(wchar_t) + 1];
        ZeroMemory(buffer, dwSize + sizeof(wchar_t));

        DWORD dwDownloaded = 0;
        WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded);

        response.append(buffer, dwDownloaded / sizeof(wchar_t));
        delete[] buffer;

    } while (dwSize > 0);

    outResponse = response;

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return true;
}
