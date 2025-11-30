#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <winsparkle.h>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#pragma comment(lib, "winhttp.lib")  // MSVC �ã�MinGW ��һ����Ч���Խ����� -lwinhttp
#pragma comment(lib, "winsparkle.lib")

// �ؼ� ID
#define IDC_EDIT_USERNAME   1001
#define IDC_EDIT_PASSWORD   1002
#define IDC_EDIT_PASSWORD2  1003
#define IDC_EDIT_EMAIL      1004
#define IDC_BUTTON_REGISTER 2001
#define IDC_BUTTON_START    2002

// ȫ�ֱ���ؼ����
HWND g_hEditUsername = NULL;
HWND g_hEditPassword = NULL;
HWND g_hEditPassword2 = NULL;
HWND g_hEditEmail = NULL;
using namespace std;
// ---------- ���ߣ�խ�ַ���ת���ַ��� ----------
std::wstring AnsiToWide(const std::string& s)
{
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, NULL, 0);
    if (len <= 0) return L"";
    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}


// UTF-8 �ַ���ת���ַ���
std::wstring Utf8ToWide(const std::string& s)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    if (len <= 0) return L"";
    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}

bool HttpPostRegister(const std::string& username,
                      const std::string& password,
                      const std::string& password2,
                      const std::string& email,
                      std::string& outResponse)
{
    outResponse.clear();

    // ======== ���޸����� ========
    std::string WEB_HOST = "wow.chenmin.org";  // ���޸ĳ���ķ�����IP
    int         WEB_PORT = 80;                // ��˿ڣ�httpĬ��80��HTTPS�ȱ���
    std::string WEB_PATH = "/register.php";   // ��PHP�ļ�·��
    // ============================

    // POST body
    std::string data =
        "username=" + username +
        "&password=" + password +
        "&password2=" + password2 +
        "&email=" + email;

    // ת���ִ�
    std::wstring hostW  = AnsiToWide(WEB_HOST);
    std::wstring pathW  = AnsiToWide(WEB_PATH);

    HINTERNET hSession = WinHttpOpen(L"WoWLauncher/1.0",
                                     WINHTTP_ACCESS_TYPE_NO_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS,
                                     0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, hostW.c_str(), WEB_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                            L"POST",
                                            pathW.c_str(),
                                            NULL,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    LPCWSTR headers = L"Content-Type: application/x-www-form-urlencoded\r\n";
    BOOL bOK = WinHttpSendRequest(hRequest,
                                  headers,
                                  -1,
                                  (LPVOID)data.c_str(),
                                  data.length(),
                                  data.length(),
                                  0);

    if (!bOK || !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD size = 0;
    while (WinHttpQueryDataAvailable(hRequest, &size) && size > 0) {
        std::string buf(size, '\0');
        DWORD read = 0;
        WinHttpReadData(hRequest, &buf[0], size, &read);
        buf.resize(read);
        outResponse += buf;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return true;
}


// ---------- �޸� Config.wtf �е� portal ----------
// �޸� _classic_era_\WTF\Config.wtf����һ��д�� SET portal "127.0.0.1"
int portal()
{
    // �������ļ�
    ifstream inFile("_classic_era_\\WTF\\Config.wtf");
    string line;
    vector<string> lines;

    if (inFile.is_open()) {
        while (getline(inFile, line)) {
            lines.push_back(line);
        }
        inFile.close();
    }

    // ���ԭ�ļ������ݣ���ӵڶ��п�ʼ����ԭ����
    int startLine = !lines.empty() ? 1 : 0;

    // ����д����ļ�
    ofstream outFile("_classic_era_\\WTF\\Config.wtf", ios::trunc);
    if (!outFile.is_open()) {
        cout << "�޷��� Config.wtf ����д�롣" << endl;
        return 1;
    }

    // д��������
    outFile << "SET portal \"wow.chenmin.org\"\n";

    // д��ԭ�����ݣ��ӵڶ��п�ʼ��
    for (int i = startLine; i < (int)lines.size(); ++i) {
        outFile << lines[i] << '\n';
    }

    outFile.close();
    cout << "�Ѹ��� Config.wtf �е� portal ���á�" << endl;
    return 0;
}


// ---------- ������Ϸ�߼����� wow.chenmin.org:1119 �ɹ������� Arctium WoW Launcher ---------- 
void StartGame()
{
    portal(); // д�� portal

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa)!=0) return;

    SOCKET s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(s==INVALID_SOCKET){WSACleanup();return;}

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1119); 
    
    hostent* he = gethostbyname("wow.chenmin.org");
	if(he) memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);


    if(connect(s,(sockaddr*)&addr,sizeof(addr))==SOCKET_ERROR){
        MessageBoxA(NULL,"�޷����� ������","����ʧ��",MB_OK|MB_ICONWARNING);
        closesocket(s);WSACleanup();return;
    }

    closesocket(s);WSACleanup();

    // ?? CreateProcessW ? ��Ϸ��������
    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION pi{};
    WCHAR cmd[] = L"Arctium WoW Launcher.exe --staticseed --version ClassicEra";

    if(!CreateProcessW(NULL,cmd,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
        MessageBoxA(NULL,"����ʧ�ܣ��Ҳ��� Arctium WoW Launcher.exe","����",MB_OK|MB_ICONERROR);
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}


// ---------- ���ڹ��� ----------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
    {
        // ���� 4 ���༭�� + �ı���ǩ
        CreateWindowA("STATIC", "�˺ţ�", WS_CHILD | WS_VISIBLE,
                      20, 20, 60, 20, hwnd, NULL, NULL, NULL);
        g_hEditUsername = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                        90, 20, 200, 20, hwnd, (HMENU)IDC_EDIT_USERNAME, NULL, NULL);

        CreateWindowA("STATIC", "���룺", WS_CHILD | WS_VISIBLE,
                      20, 50, 60, 20, hwnd, NULL, NULL, NULL);
        g_hEditPassword = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
                                        90, 50, 200, 20, hwnd, (HMENU)IDC_EDIT_PASSWORD, NULL, NULL);

        CreateWindowA("STATIC", "�ظ����룺", WS_CHILD | WS_VISIBLE,
                      20, 80, 90, 20, hwnd, NULL, NULL, NULL);
        g_hEditPassword2 = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
                                         90, 80, 200, 20, hwnd, (HMENU)IDC_EDIT_PASSWORD2, NULL, NULL);

        CreateWindowA("STATIC", "���䣺", WS_CHILD | WS_VISIBLE,
                      20, 110, 60, 20, hwnd, NULL, NULL, NULL);
        g_hEditEmail = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                     90, 110, 200, 20, hwnd, (HMENU)IDC_EDIT_EMAIL, NULL, NULL);

        // ������ť
        CreateWindowA("BUTTON", "ע���˺�",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      50, 150, 100, 30, hwnd, (HMENU)IDC_BUTTON_REGISTER, NULL, NULL);

        CreateWindowA("BUTTON", "������Ϸ",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      180, 150, 100, 30, hwnd, (HMENU)IDC_BUTTON_START, NULL, NULL);
    }
    break;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == IDC_BUTTON_REGISTER && HIWORD(wParam) == BN_CLICKED) {
            // ��ȡ���������
            char bufUser[64] = { 0 };
            char bufPass[64] = { 0 };
            char bufPass2[64] = { 0 };
            char bufEmail[128] = { 0 };

            GetWindowTextA(g_hEditUsername, bufUser, sizeof(bufUser));
            GetWindowTextA(g_hEditPassword, bufPass, sizeof(bufPass));
            GetWindowTextA(g_hEditPassword2, bufPass2, sizeof(bufPass2));
            GetWindowTextA(g_hEditEmail, bufEmail, sizeof(bufEmail));

            std::string username = bufUser;
            std::string password = bufPass;
            std::string password2 = bufPass2;
            std::string email = bufEmail;

            if (username.empty() || password.empty() || password2.empty() || email.empty()) {
                MessageBoxA(hwnd, "������������Ϣ���˺š��������롢���䣩", "��ʾ", MB_OK | MB_ICONINFORMATION);
                break;
            }

            std::string resp;
			bool ok = HttpPostRegister(username, password, password2, email, resp);
			if (!ok) {
			    MessageBoxA(hwnd, "����ʧ�ܣ���ȷ�ϱ��� Web ��������������", "����", MB_OK | MB_ICONERROR);
			} else {
			    // ȥ�� UTF-8 BOM��EF BB BF��
			    if (resp.size() >= 3 &&
			        (unsigned char)resp[0] == 0xEF &&
			        (unsigned char)resp[1] == 0xBB &&
			        (unsigned char)resp[2] == 0xBF) {
			        resp.erase(0, 3);
			    }
			
			    bool success = false;
			    std::string msgText;
			
			    // --- ���ϸ� JSON �������ֶ��� success �� message �ֶ� ---
			    // 1) success
			    size_t posSucc = resp.find("\"success\"");
			    if (posSucc != std::string::npos) {
			        size_t posColon = resp.find(':', posSucc);
			        if (posColon != std::string::npos) {
			            size_t posVal = resp.find_first_not_of(" \t\r\n", posColon + 1);
			            if (posVal != std::string::npos &&
			                resp.compare(posVal, 4, "true") == 0) {
			                success = true;
			            }
			        }
			    }
			
			    // 2) message
			    size_t posMsgKey = resp.find("\"message\"");
			    if (posMsgKey != std::string::npos) {
			        size_t posColon = resp.find(':', posMsgKey);
			        if (posColon != std::string::npos) {
			            size_t posQuote1 = resp.find('"', posColon + 1);
			            if (posQuote1 != std::string::npos) {
			                size_t posQuote2 = resp.find('"', posQuote1 + 1);
			                if (posQuote2 != std::string::npos) {
			                    msgText = resp.substr(posQuote1 + 1,
			                                          posQuote2 - posQuote1 - 1);
			                }
			            }
			        }
			    }
			
			    // ���û������ message�����˻���ʾ���� resp
			    if (msgText.empty()) {
			        msgText = resp;
			    }
			
			    std::wstring wMsg   = Utf8ToWide(msgText);
				std::wstring wTitle;
				if (success) {
				    wTitle = L"\u6CE8\u518C\u6210\u529F";  // ��ע��ɹ���
				} else {
				    wTitle = L"\u6CE8\u518C\u5931\u8D25";  // ��ע��ʧ�ܡ�
				}

			
			    UINT uIcon = success ? MB_ICONINFORMATION : MB_ICONERROR;
			
			    MessageBoxW(hwnd,
			                wMsg.c_str(),
			                wTitle.c_str(),
			                MB_OK | uIcon);
			}

 
        }
        else if (id == IDC_BUTTON_START && HIWORD(wParam) == BN_CLICKED) {
            StartGame();
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ---------- WinMain ----------
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // ��ʼ�� WinSparkle �Զ�����
    win_sparkle_set_appcast_url("https://wow.chenmin.org/appcast.xml");
    win_sparkle_set_app_details(L"WoW Launcher Project", L"WoW Launcher", L"1.0.0");
    win_sparkle_init();
    
    // ��鸻���Ƿ�����°汾
    win_sparkle_check_update_without_ui();
    
    // ע�ᴰ����
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "WoWLauncherWndClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "ע�ᴰ����ʧ��", "����", MB_OK | MB_ICONERROR);
        return 1;
    }

    // ��������
    HWND hwnd = CreateWindowExA(
        0,
        "WoWLauncherWndClass",
        "WoW ������ & ע��",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        340, 250,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        MessageBoxA(NULL, "��������ʧ��", "����", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // ��Ϣѭ��
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // ���� WinSparkle
    win_sparkle_cleanup();

    return (int)msg.wParam;
}

