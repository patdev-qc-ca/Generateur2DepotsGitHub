// GitHubRepoManager.cpp
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <winhttp.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "resource.h"
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shell32.lib")

// IDs contrôles, menus, etc.

enum
{
 IDC_TOKEN_EDIT = 1001,
 IDC_REPONAME_EDIT,
 IDC_DESC_EDIT,
 IDC_PRIVATE_CHECK,
 IDC_CREATE_BUTTON,
 IDC_OUTPUT_EDIT,
 IDC_HISTORY_LIST,
 IDC_SEARCH_EDIT,
 ID_MENU_FILE_EXIT = 2001,
 ID_MENU_FILE_EXPORTZIP,
 ID_MENU_HELP_ABOUT,
 ID_TOOL_CREATE_REPO = 3001,
 ID_TOOL_EXPORT_ZIP,
 ID_STATUS_MAIN = 4001
};


// Structures & globals


struct RepoEntry
{
	std::wstring name;
	bool isPrivate;
	std::wstring date;
	std::wstring url;
};
HINSTANCE g_hInst = nullptr;
HWND g_hMainWnd = nullptr;
HWND g_hStatus = nullptr;
HWND g_hHistoryList = nullptr;
WNDCLASS wc = {};
HWND g_hSearchEdit = nullptr;
HIMAGELIST g_hImageList = nullptr;
std::vector<RepoEntry> g_history;
int g_sortColumn = 0;
bool g_sortAscending[4] = { true, true, true, true };
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreerInterface(HWND hwnd);
void CreerMenu(HWND hwnd);
void CreerRuban(HWND hwnd);
void CreerStatut(HWND hwnd);
void Ajustement(HWND hwnd);
void InitialiserListe(HWND hList);
void MettreHistoriqueAJour(HWND hwnd, const RepoEntry& e);
void ActualiserHistorique(HWND hwnd, const std::wstring& filter = L"");
void IntegrerHistorique(HWND hList, int index, const RepoEntry& e);
void EnregisterHistorique(const std::wstring& path);
void ChargerHistorique(const std::wstring& path);
std::wstring GetAppDir();
std::wstring ExtraireLien(const std::wstring& json);
bool CreerDepotGIT(const std::wstring& token,const std::wstring& name,const std::wstring& description,bool isPrivate,std::wstring& outResponse);
std::string WStringToUtf8(const std::wstring& w);
std::wstring Utf8ToWString(const std::string& s);
void DefinirTexteSaisie(HWND hDlg, int controlId, const std::wstring& text);
std::wstring LireTexteSaisie(HWND hDlg, int controlId);
void AfficherLeStatut(const std::wstring& text);
void TrierHistorique(int column);
int CALLBACK HistoryCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
void FiltrerHistorique(const std::wstring& filter);
void GererSortieZIP(HWND hwnd);
bool Zipper(const std::wstring& folder, const std::wstring& zipPath);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	g_hInst = hInstance;
	INITCOMMONCONTROLSEX icc = {};
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
	InitCommonControlsEx(&icc);
	const wchar_t CLASS_NAME[] = L"GitHubRepoManagerWindowClass";
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hIcon = LoadIcon(wc.hInstance, (LPCWSTR)IDI_ICON1);
	if (!RegisterClass(&wc))return 0;
	HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"GitHub Repo Manager (Win32 C++)", WS_OVERLAPPED |WS_CAPTION |WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 900, 600, nullptr, nullptr, hInstance, nullptr);
	if (!hwnd)return 0;
	g_hMainWnd = hwnd;
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
	ChargerHistorique(GetAppDir() + L"history.json");
	ActualiserHistorique(hwnd);
	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
void CreerMenu(HWND hwnd)
{
	HMENU hMenuBar = CreateMenu();
	HMENU hFile = CreateMenu();
	HMENU hHelp = CreateMenu();
	AppendMenu(hFile, MF_STRING, ID_MENU_FILE_EXPORTZIP, L"&Exporter projet Git en ZIP...");
	AppendMenu(hFile, MF_SEPARATOR, 0, nullptr);
	AppendMenu(hFile, MF_STRING, ID_MENU_FILE_EXIT, L"&Quitter");
	AppendMenu(hHelp, MF_STRING, ID_MENU_HELP_ABOUT, L"&À propos");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFile, L"&Fichier");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, L"&Aide");
	SetMenu(hwnd, hMenuBar);
}
void CreerRuban(HWND hwnd)
{
 HWND hToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, nullptr,WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,0, 0, 0, 0,hwnd, nullptr, g_hInst, nullptr);
 SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
 TBBUTTON tbb[2] = {};
 tbb[0].iBitmap = 0;
 tbb[0].idCommand = ID_TOOL_CREATE_REPO;
 tbb[0].fsState = TBSTATE_ENABLED;
 tbb[0].fsStyle = TBSTYLE_BUTTON;
 tbb[0].iString = (INT_PTR)L"Créer repo";
 tbb[1].iBitmap = 1;
 tbb[1].idCommand = ID_TOOL_EXPORT_ZIP;
 tbb[1].fsState = TBSTATE_ENABLED;
 tbb[1].fsStyle = TBSTYLE_BUTTON;
 tbb[1].iString = (INT_PTR)L"Export ZIP";
 SendMessage(hToolbar, TB_ADDBUTTONS, (WPARAM)2, (LPARAM)&tbb);
}
void CreerStatut(HWND hwnd)
{
	g_hStatus = CreateWindowEx(0, STATUSCLASSNAME, nullptr, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, (HMENU)ID_STATUS_MAIN, g_hInst, nullptr);
}
void CreerInterface(HWND hwnd)
{
	CreerMenu(hwnd);
	CreerRuban(hwnd);
	CreerStatut(hwnd);
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	CreateWindowEx(0, L"STATIC", L"GitHub Token:", WS_CHILD | WS_VISIBLE, 10, 40, 100, 20, hwnd, nullptr, nullptr, nullptr);
	HWND hTokenEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_PASSWORD, 120, 40, 750, 20, hwnd, (HMENU)IDC_TOKEN_EDIT, nullptr, nullptr);
	CreateWindowEx(0, L"STATIC", L"Nom du repository:", WS_CHILD | WS_VISIBLE, 10, 70, 150, 20, hwnd, nullptr, nullptr, nullptr);
	HWND hRepoEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 140, 70, 300, 20, hwnd, (HMENU)IDC_REPONAME_EDIT, nullptr, nullptr);
	CreateWindowEx(0, L"STATIC", L"Description:", WS_CHILD | WS_VISIBLE, 10, 100, 100, 20, hwnd, nullptr, nullptr, nullptr);
	HWND hDescEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 120, 100, 750, 20, hwnd, (HMENU)IDC_DESC_EDIT, nullptr, nullptr);
	HWND hPrivateCheck = CreateWindowEx(0, L"BUTTON", L"Privé", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 10, 130, 80, 20, hwnd, (HMENU)IDC_PRIVATE_CHECK, nullptr, nullptr);
	HWND hCreateBtn = CreateWindowEx(0, L"BUTTON", L"Créer le repository", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 120, 130, 180, 25, hwnd, (HMENU)IDC_CREATE_BUTTON, nullptr, nullptr);
	CreateWindowEx(0, L"STATIC", L"Réponse GitHub:", WS_CHILD | WS_VISIBLE, 10, 165, 120, 20, hwnd, nullptr, nullptr, nullptr);
	HWND hOutputEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, 10, 185, 860, 120, hwnd, (HMENU)IDC_OUTPUT_EDIT, nullptr, nullptr);
	CreateWindowEx(0, L"STATIC", L"Recherche dans l'historique:", WS_CHILD | WS_VISIBLE, 10, 315, 160, 20, hwnd, nullptr, nullptr, nullptr);
	g_hSearchEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 180, 315, 300, 20, hwnd, (HMENU)IDC_SEARCH_EDIT, nullptr, nullptr);
	g_hHistoryList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, 10, 345, 860, 180, hwnd, (HMENU)IDC_HISTORY_LIST, g_hInst, nullptr);
	ListView_SetExtendedListViewStyle(g_hHistoryList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	g_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 2);
	HICON hPublic = LoadIcon(wc.hInstance, (LPWSTR)IDI_ICON3);
	HICON hPrivate = LoadIcon(wc.hInstance, (LPWSTR)IDI_ICON2);
	ImageList_AddIcon(g_hImageList, hPublic);
	ImageList_AddIcon(g_hImageList, hPrivate);
	ListView_SetImageList(g_hHistoryList, g_hImageList, LVSIL_SMALL);
	InitialiserListe(g_hHistoryList);
	SendMessage(hTokenEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hRepoEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hDescEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hPrivateCheck, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hCreateBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hOutputEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(g_hSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(g_hHistoryList, WM_SETFONT, (WPARAM)hFont, TRUE);
}
void InitialiserListe(HWND hList)
{
	LVCOLUMN col = {};
	col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	col.pszText = (LPWSTR)L"Nom";
	col.cx = 200;
	col.iSubItem = 0;
	ListView_InsertColumn(hList, 0, &col);
	col.pszText = (LPWSTR)L"Privé";
	col.cx = 60;
	col.iSubItem = 1;
	ListView_InsertColumn(hList, 1, &col);
	col.pszText = (LPWSTR)L"Date";
	col.cx = 200;
	col.iSubItem = 2;
	ListView_InsertColumn(hList, 2, &col);
	col.pszText = (LPWSTR)L"URL";
	col.cx = 380;
	col.iSubItem = 3;
	ListView_InsertColumn(hList, 3, &col);
}
void Ajustement(HWND hwnd)
{
	RECT rcClient;
	GetClientRect(hwnd, &rcClient);
	HWND hToolbar = FindWindowEx(hwnd, nullptr, TOOLBARCLASSNAME, nullptr);
	if (hToolbar)SendMessage(hToolbar, TB_AUTOSIZE, 0, 0);
	if (g_hStatus)SendMessage(g_hStatus, WM_SIZE, 0, 0);
	int statusHeight = 0;
	if (g_hStatus)
	{
		RECT rcStatus;
		GetWindowRect(g_hStatus, &rcStatus);
		statusHeight = rcStatus.bottom - rcStatus.top;
	}
	int toolbarHeight = 0;
	if (hToolbar)
	{
		RECT rcTb;
		GetWindowRect(hToolbar, &rcTb);
		toolbarHeight = rcTb.bottom - rcTb.top;
	}
	int top = toolbarHeight + 5;
	MoveWindow(GetDlgItem(hwnd, IDC_TOKEN_EDIT), 120, top, rcClient.right - 130, 20, TRUE);
	top += 30;
	MoveWindow(GetDlgItem(hwnd, IDC_REPONAME_EDIT), 140, top, 300, 20, TRUE);
	top += 30;
	MoveWindow(GetDlgItem(hwnd, IDC_DESC_EDIT), 120, top, rcClient.right - 130, 20, TRUE);
	top += 30;
	MoveWindow(GetDlgItem(hwnd, IDC_PRIVATE_CHECK), 10, top, 80, 20, TRUE);
	MoveWindow(GetDlgItem(hwnd, IDC_CREATE_BUTTON), 120, top, 180, 25, TRUE);
	top += 35;
	MoveWindow(GetDlgItem(hwnd, IDC_OUTPUT_EDIT), 10, top, rcClient.right - 20, 120, TRUE);
	top += 130;
	MoveWindow(g_hSearchEdit, 180, top, 300, 20, TRUE);
	top += 30;
	MoveWindow(g_hHistoryList, 10, top, rcClient.right - 20, rcClient.bottom - statusHeight - top - 5, TRUE);
}
void DefinirTexteSaisie(HWND hDlg, int controlId, const std::wstring& text)
{
	HWND hCtrl = GetDlgItem(hDlg, controlId);
	if (hCtrl)	SetWindowText(hCtrl, text.c_str());
}
std::wstring LireTexteSaisie(HWND hDlg, int controlId)
{
	HWND hCtrl = GetDlgItem(hDlg, controlId);
	if (!hCtrl)	return L"";
	int len = GetWindowTextLength(hCtrl);
	std::wstring text(len, L'\0');
	GetWindowText(hCtrl, &text[0], len + 1);
	return text;
}
void AfficherLeStatut(const std::wstring& text)
{
	if (g_hStatus)
	{
		SendMessage(g_hStatus, SB_SETTEXT, 0, (LPARAM)text.c_str());
	}
}
void EnregisterHistorique(const std::wstring& path)
{
	std::wofstream f(path);
	if (!f.is_open())
		return;
	f << L"[\n";
	for (size_t i = 0; i < g_history.size(); ++i)
	{
		const auto& e = g_history[i];
		f << L"  {\n";
		f << L" \"name\": \"" << e.name << L"\",\n";
		f << L" \"private\": " << (e.isPrivate ? L"true" : L"false") << L",\n";
		f << L" \"date\": \"" << e.date << L"\",\n";
		f << L" \"url\": \"" << e.url << L"\"\n";
		f << L"  }" << (i + 1 < g_history.size() ? L"," : L"") << L"\n";
	}
	f << L"]\n";
}
static std::wstring Trim(const std::wstring& s)
{
	size_t start = s.find_first_not_of(L" \t\r\n");
	if (start == std::wstring::npos) return L"";
	size_t end = s.find_last_not_of(L" \t\r\n");
	return s.substr(start, end - start + 1);
}
void ChargerHistorique(const std::wstring& path)
{
	g_history.clear();
	std::wifstream f(path);
	if (!f.is_open())return;
	std::wstringstream buffer;
	buffer << f.rdbuf();
	std::wstring content = buffer.str();
	size_t pos = 0;
	while (true)
	{
		pos = content.find(L"{", pos);
		if (pos == std::wstring::npos) break;
		size_t end = content.find(L"}", pos);
		if (end == std::wstring::npos) break;
		std::wstring obj = content.substr(pos, end - pos + 1);
		pos = end + 1;
		RepoEntry e;
		size_t p = obj.find(L"\"name\"");
		if (p != std::wstring::npos)
		{
			p = obj.find(L"\"", p + 6);
			size_t p2 = obj.find(L"\"", p + 1);
			if (p != std::wstring::npos && p2 != std::wstring::npos)e.name = obj.substr(p + 1, p2 - p - 1);
		}
		p = obj.find(L"\"private\"");
		if (p != std::wstring::npos)
		{
			size_t p2 = obj.find(L"true", p);
			e.isPrivate = (p2 != std::wstring::npos && p2 < obj.find(L",", p));
		}
		p = obj.find(L"\"date\"");
		if (p != std::wstring::npos)
		{
			p = obj.find(L"\"", p + 6);
			size_t p2 = obj.find(L"\"", p + 1);
			if (p != std::wstring::npos && p2 != std::wstring::npos)e.date = obj.substr(p + 1, p2 - p - 1);
		}
		p = obj.find(L"\"url\"");
		if (p != std::wstring::npos)
		{
			p = obj.find(L"\"", p + 5);
			size_t p2 = obj.find(L"\"", p + 1);
			if (p != std::wstring::npos && p2 != std::wstring::npos)e.url = obj.substr(p + 1, p2 - p - 1);
		}
		if (!e.name.empty())g_history.push_back(e);
	}
}
void IntegrerHistorique(HWND hList, int index, const RepoEntry& e)
{
	LVITEM item = {};
	item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	item.iItem = ListView_GetItemCount(hList);
	item.iSubItem = 0;
	item.pszText = const_cast<LPWSTR>(e.name.c_str());
	item.iImage = e.isPrivate ? 1 : 0;
	item.lParam = index; // index dans g_history
	int row = ListView_InsertItem(hList, &item);
	ListView_SetItemText(hList, row, 1, const_cast<LPWSTR>(e.isPrivate ? L"Oui" : L"Non"));
	ListView_SetItemText(hList, row, 2, const_cast<LPWSTR>(e.date.c_str()));
	ListView_SetItemText(hList, row, 3, const_cast<LPWSTR>(e.url.c_str()));
}
void ActualiserHistorique(HWND hwnd, const std::wstring& filter)
{
	if (!g_hHistoryList) return;
	ListView_DeleteAllItems(g_hHistoryList);
	std::wstring f = Trim(filter);
	for (size_t i = 0; i < g_history.size(); ++i)
	{
		const auto& e = g_history[i];
		if (!f.empty())
		{
			if (e.name.find(f) == std::wstring::npos &&
				e.url.find(f) == std::wstring::npos)
				continue;
		}
		IntegrerHistorique(g_hHistoryList, (int)i, e);
	}
}
void MettreHistoriqueAJour(HWND hwnd, const RepoEntry& e)
{
 g_history.push_back(e);
 EnregisterHistorique(GetAppDir() + L"history.json");
 std::wstring filter = LireTexteSaisie(hwnd, IDC_SEARCH_EDIT);
 ActualiserHistorique(hwnd, filter);
}
void TrierHistorique(int column)
{
	if (!g_hHistoryList) return;
	g_sortColumn = column;
	g_sortAscending[column] = !g_sortAscending[column];
	ListView_SortItemsEx(g_hHistoryList, HistoryCompareProc, 0);
}
int CALLBACK HistoryCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM)
{
	int idx1 = (int)lParam1;
	int idx2 = (int)lParam2;
	if (idx1 < 0 || idx1 >= (int)g_history.size()) return 0;
	if (idx2 < 0 || idx2 >= (int)g_history.size()) return 0;
	const auto& a = g_history[idx1];
	const auto& b = g_history[idx2];
	int cmp = 0;
	switch (g_sortColumn)
	{
	case 0: cmp = a.name.compare(b.name); break;
	case 1: cmp = (int)a.isPrivate - (int)b.isPrivate; break;
	case 2: cmp = a.date.compare(b.date); break;
	case 3: cmp = a.url.compare(b.url); break;
	}
	return g_sortAscending[g_sortColumn] ? cmp : -cmp;
}
std::wstring GetAppDir()
{
	wchar_t path[MAX_PATH] = {};
	GetModuleFileName(nullptr, path, MAX_PATH);
	std::wstring p(path);
	size_t pos = p.find_last_of(L"\\/");
	if (pos != std::wstring::npos)
		p = p.substr(0, pos + 1);
	else
		p += L"\\";
	return p;
}
std::string WStringToUtf8(const std::wstring& w)
{
 if (w.empty()) return {};
 int size_needed = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
 std::string result(size_needed, 0);
 WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &result[0], size_needed, nullptr, nullptr);
 return result;
}
std::wstring Utf8ToWString(const std::string& s)
{
	if (s.empty()) return {};
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
	std::wstring result(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &result[0], size_needed);
	return result;
}
// GitHub API
std::wstring ExtraireLien(const std::wstring& json)
{
	size_t pos = json.find(L"\"html_url\"");
	if (pos == std::wstring::npos) return L"";
	pos = json.find(L":", pos);
	if (pos == std::wstring::npos) return L"";
	pos = json.find(L"\"", pos);
	if (pos == std::wstring::npos) return L"";
	size_t end = json.find(L"\"", pos + 1);
	if (end == std::wstring::npos) return L"";
	return json.substr(pos + 1, end - pos - 1);
}
bool CreerDepotGIT(const std::wstring& token, const std::wstring& name, const std::wstring& description, bool isPrivate, std::wstring& outResponse)
{
	outResponse.clear();
	HINTERNET hSession = WinHttpOpen(L"Win32 GitHub Repo Manager/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession)
		return false;
	HINTERNET hConnect = WinHttpConnect(hSession, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!hConnect)
	{
		WinHttpCloseHandle(hSession);
		return false;
	}
	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/user/repos", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	if (!hRequest)
	{
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}
	// Entetes
	std::wstring authHeader = L"Authorization: token " + token;
	WinHttpAddRequestHeaders(hRequest, authHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
	WinHttpAddRequestHeaders(hRequest, L"User-Agent: Win32-GitHub-Client", -1, WINHTTP_ADDREQ_FLAG_ADD);
	WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/json", -1, WINHTTP_ADDREQ_FLAG_ADD);
	std::wstring jsonW = L"{\"name\":\"" + name + L"\",\"description\":\"" + description + L"\",\"private\":" + (isPrivate ? L"true" : L"false") + L"}";
	std::string jsonUtf8 = WStringToUtf8(jsonW);
	BOOL bResult = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)jsonUtf8.c_str(), (DWORD)jsonUtf8.size(), (DWORD)jsonUtf8.size(), 0);
	if (!bResult)
	{
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}
	if (!WinHttpReceiveResponse(hRequest, nullptr))
	{
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}
	std::string responseUtf8;
	DWORD dwSize = 0;
	do
	{
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize))break;
		if (dwSize == 0)break;
		std::string buffer;
		buffer.resize(dwSize);
		DWORD dwDownloaded = 0;
		if (!WinHttpReadData(hRequest, &buffer[0], dwSize, &dwDownloaded) || dwDownloaded == 0)break;
		buffer.resize(dwDownloaded);
		responseUtf8.append(buffer);
	} while (dwSize > 0);
	outResponse = Utf8ToWString(responseUtf8);
	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return true;
}
// Export ZIP via tar.exe

bool Zipper(const std::wstring& folder, const std::wstring& zipPath)
{
	// Commande: tar.exe -a -c -f "zipPath" -C "folder" .
	std::wstringstream cmd;
	cmd << L"tar.exe -a -c -f \"" << zipPath << L"\" -C \"" << folder << L"\" .";
	std::wstring cmdStr = cmd.str();
	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi = {};
	std::wstring cmdMutable = cmdStr;
	if (!CreateProcessW(nullptr, &cmdMutable[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
	{
		return false;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exitCode = 0;
	GetExitCodeProcess(pi.hProcess, &exitCode);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return (exitCode == 0);
}
void GererSortieZIP(HWND hwnd)
{
	std::wstring folder = LireTexteSaisie(hwnd, IDC_REPONAME_EDIT); // hack: à remplacer par un vrai chemin si tu veux
	if (folder.empty())
	{
		MessageBox(hwnd, L"Pour la démo, saisis le chemin du dossier Git local dans 'Nom du repository'.", L"Info", MB_ICONINFORMATION);
		return;
	}
	std::wstring zipPath = folder + L".zip";
	AfficherLeStatut(L"Export ZIP en cours...");
	if (Zipper(folder, zipPath))
	{
		AfficherLeStatut(L"Export ZIP terminé.");
		MessageBox(hwnd, (L"ZIP créé : " + zipPath).c_str(), L"Export ZIP", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		AfficherLeStatut(L"Échec de l'export ZIP.");
		MessageBox(hwnd, L"Échec de l'export ZIP (tar.exe requis).", L"Erreur", MB_OK | MB_ICONERROR);
	}
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		CreerInterface(hwnd);
		AfficherLeStatut(L"Prêt.");
		return 0;
	case WM_SIZE:  Ajustement(hwnd);
		return 0;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDC_CREATE_BUTTON:
		case ID_TOOL_CREATE_REPO:
		{
			std::wstring token = LireTexteSaisie(hwnd, IDC_TOKEN_EDIT);
			std::wstring name = LireTexteSaisie(hwnd, IDC_REPONAME_EDIT);
			std::wstring desc = LireTexteSaisie(hwnd, IDC_DESC_EDIT);
			BOOL checked = (SendDlgItemMessage(hwnd, IDC_PRIVATE_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED);
			bool isPrivate = (checked == TRUE);
			if (token.empty() || name.empty())
			{
				MessageBox(hwnd, L"Token GitHub et nom du repository requis.", L"Erreur", MB_ICONERROR);
				return 0;
			}
			AfficherLeStatut(L"Création du repository en cours...");
			DefinirTexteSaisie(hwnd, IDC_OUTPUT_EDIT, L"Envoi de la requête à GitHub...");
			std::wstring response;
			bool ok = CreerDepotGIT(token, name, desc, isPrivate, response);
			if (!ok)
			{
				AfficherLeStatut(L"Échec de la requête GitHub.");
				DefinirTexteSaisie(hwnd, IDC_OUTPUT_EDIT, L"Échec de la requête HTTP vers GitHub.");
			}
			else
			{
				AfficherLeStatut(L"Repository créé (voir historique).");
				DefinirTexteSaisie(hwnd, IDC_OUTPUT_EDIT, response);
				std::wstring url = ExtraireLien(response);
				SYSTEMTIME st;
				GetLocalTime(&st);
				wchar_t dateStr[64];
				swprintf(dateStr, 64, L"%02d/%02d/%04d %02d:%02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
				RepoEntry e{ name, isPrivate, dateStr, url };
				MettreHistoriqueAJour(hwnd, e);
			}
		}
		return 0;
		case ID_MENU_FILE_EXIT:
			PostQuitMessage(0);
			return 0;
		case ID_MENU_FILE_EXPORTZIP:
		case ID_TOOL_EXPORT_ZIP:
			GererSortieZIP(hwnd);
			return 0;
		case ID_MENU_HELP_ABOUT:
			MessageBox(hwnd, L"GitHub Repo Manager\nWin32 C++\n\nDémo complète : création de repos GitHub, historique, tri, recherche, export ZIP.", L"À propos", MB_OK | MB_ICONINFORMATION);
			return 0;
		}
	}
	break;
	case WM_NOTIFY:
	{
		LPNMHDR hdr = (LPNMHDR)lParam;
		if (hdr->idFrom == IDC_HISTORY_LIST)
		{
			if (hdr->code == LVN_COLUMNCLICK)
			{
				LPNMLISTVIEW pnm = (LPNMLISTVIEW)lParam;
				TrierHistorique(pnm->iSubItem);
				return 0;
			}
			else if (hdr->code == NM_DBLCLK)
			{
				LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
				int item = act->iItem;
				if (item >= 0)
				{
					LVITEM lv = {};
					lv.mask = LVIF_PARAM;
					lv.iItem = item;
					if (ListView_GetItem(g_hHistoryList, &lv))
					{
						int idx = (int)lv.lParam; if (idx >= 0 && idx < (int)g_history.size()) { const auto& e = g_history[idx]; if (!e.url.empty()) ShellExecute(nullptr, L"open", e.url.c_str(), nullptr, nullptr, SW_SHOWNORMAL); }
					}
				}
				return 0;
			}
		}
	}
	break;
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		// (optionnel : custom draw)
		break;
	case WM_DESTROY:
		EnregisterHistorique(GetAppDir() + L"history.json");
		PostQuitMessage(0);
		return 0;
	}
	if (uMsg == WM_COMMAND && HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_SEARCH_EDIT)
	{
		std::wstring filter = LireTexteSaisie(hwnd, IDC_SEARCH_EDIT);
		ActualiserHistorique(hwnd, filter);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);

}
