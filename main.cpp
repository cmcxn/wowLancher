#include <winsock2.h>
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#include <winsparkle.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "winsparkle.lib")

// 版本号定义
#define APP_VERSION "1.0.2"

// 控件 ID
#define IDC_EDIT_USERNAME   1001
#define IDC_EDIT_PASSWORD   1002
#define IDC_EDIT_PASSWORD2  1003
#define IDC_EDIT_EMAIL      1004
#define IDC_BUTTON_REGISTER 2001
#define IDC_BUTTON_START    2002

// 全局保存控件句柄

HWND g_hwnd = NULL; // 全局窗口句柄，用于跨线程关闭窗口

HWND g_hEditUsername = NULL;
HWND g_hEditPassword = NULL;
HWND g_hEditPassword2 = NULL;
HWND g_hEditEmail = NULL;

// 全局标志：是否在检查完更新后自动启动游戏
bool g_bAutoStartGame = false;

using namespace std;

// ---------- 工具：窄字符串转宽字符串 ----------
std::wstring AnsiToWide(const std::string& s)
{
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, NULL, 0);
    if (len <= 0) return L"";
    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}

// UTF-8 字符串转宽字符串
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

    // ======== 可修改配置 ========
    std::string WEB_HOST = "wow.chenmin.org";  
    int         WEB_PORT = 80;                
    std::string WEB_PATH = "/register.php";   
    // ============================

    std::string data =
        "username=" + username +
        "&password=" + password +
        "&password2=" + password2 +
        "&email=" + email;

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

// ---------- 修改 Config.wtf 中的 portal ----------
int portal()
{
    ifstream inFile("_classic_era_\\WTF\\Config.wtf");
    string line;
    vector<string> lines;

    if (inFile.is_open()) {
        while (getline(inFile, line)) {
            lines.push_back(line);
        }
        inFile.close();
    }

    int startLine = !lines.empty() ? 1 : 0;

    ofstream outFile("_classic_era_\\WTF\\Config.wtf", ios::trunc);
    if (!outFile.is_open()) {
        cout << "无法打开 Config.wtf 进行写入。" << endl;
        return 1;
    }

    outFile << "SET portal \"wow.chenmin.org\"\n";

    for (int i = startLine; i < (int)lines.size(); ++i) {
        outFile << lines[i] << '\n';
    }

    outFile.close();
    return 0;
}

// ---------- 启动游戏逻辑 ---------- 
void StartGame()
{
    portal(); // 写入 portal

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
        MessageBoxA(NULL,"无法连接 服务器","连接失败",MB_OK|MB_ICONWARNING);
        closesocket(s);WSACleanup();return;
    }

    closesocket(s);WSACleanup();

    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION pi{};
    WCHAR cmd[] = L"Arctium WoW Launcher.exe --staticseed --version ClassicEra";

    if(!CreateProcessW(NULL,cmd,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)){
        MessageBoxA(NULL,"启动失败，找不到 Arctium WoW Launcher.exe","错误",MB_OK|MB_ICONERROR);
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

// ---------- WinSparkle 回调函数 ----------
// 1. 没有发现更新 -> 启动游戏
void OnDidNotFindUpdate()
{
    if (g_bAutoStartGame) {
        g_bAutoStartGame = false;
        StartGame();
    }
}

// 2. 用户取消更新或关闭更新窗口 -> 启动游戏
void OnUpdateCancelled()
{
    if (g_bAutoStartGame) {
        g_bAutoStartGame = false;
        StartGame();
    }
}

// 3. 发现更新 -> 不自动启动游戏，让 WinSparkle UI 处理更新重启
void OnDidFindUpdate()
{
    // 发现更新了，用户会看到更新窗口
    // 如果用户选择更新，程序会重启，游戏自然不会启动（这是对的，因为要启动新版）
    // 如果用户点击“以后再说”或关闭窗口，会触发 OnUpdateCancelled，从而启动游戏
}
// 4. ★★★ 关键回调：下载完成，准备安装，请求关闭当前程序 ★★★
void OnShutdownRequest() {
    // WinSparkle 准备运行更新包了，我们必须立刻退出
    // 否则更新包无法覆盖当前的 exe 文件
    g_bAutoStartGame = false; 
    
    // 发送关闭消息给主窗口
    if (g_hwnd) {
        PostMessage(g_hwnd, WM_CLOSE, 0, 0);
    }
}

// ---------- 窗口过程 ----------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
    {
        // 创建控件...
        CreateWindowA("STATIC", "账号：", WS_CHILD | WS_VISIBLE,
                      20, 20, 60, 20, hwnd, NULL, NULL, NULL);
        g_hEditUsername = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                        90, 20, 200, 20, hwnd, (HMENU)IDC_EDIT_USERNAME, NULL, NULL);

        CreateWindowA("STATIC", "密码：", WS_CHILD | WS_VISIBLE,
                      20, 50, 60, 20, hwnd, NULL, NULL, NULL);
        g_hEditPassword = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
                                        90, 50, 200, 20, hwnd, (HMENU)IDC_EDIT_PASSWORD, NULL, NULL);

        CreateWindowA("STATIC", "重复密码：", WS_CHILD | WS_VISIBLE,
                      20, 80, 90, 20, hwnd, NULL, NULL, NULL);
        g_hEditPassword2 = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
                                         90, 80, 200, 20, hwnd, (HMENU)IDC_EDIT_PASSWORD2, NULL, NULL);

        CreateWindowA("STATIC", "邮箱：", WS_CHILD | WS_VISIBLE,
                      20, 110, 60, 20, hwnd, NULL, NULL, NULL);
        g_hEditEmail = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER,
                                     90, 110, 200, 20, hwnd, (HMENU)IDC_EDIT_EMAIL, NULL, NULL);

        CreateWindowA("BUTTON", "注册账号",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      50, 150, 100, 30, hwnd, (HMENU)IDC_BUTTON_REGISTER, NULL, NULL);

        CreateWindowA("BUTTON", "启动游戏",
                      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                      180, 150, 100, 30, hwnd, (HMENU)IDC_BUTTON_START, NULL, NULL);
    }
    break;

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == IDC_BUTTON_REGISTER && HIWORD(wParam) == BN_CLICKED) {
            // (注册逻辑保持不变)
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
                MessageBoxA(hwnd, "请输入完整信息（账号、两次密码、邮箱）", "提示", MB_OK | MB_ICONINFORMATION);
                break;
            }

            std::string resp;
            bool ok = HttpPostRegister(username, password, password2, email, resp);
            if (!ok) {
                MessageBoxA(hwnd, "请求失败，请确认本机 Web 服务器已启动。", "错误", MB_OK | MB_ICONERROR);
            } else {
                // UTF-8 BOM 处理
                if (resp.size() >= 3 &&
                    (unsigned char)resp[0] == 0xEF &&
                    (unsigned char)resp[1] == 0xBB &&
                    (unsigned char)resp[2] == 0xBF) {
                    resp.erase(0, 3);
                }
                bool success = false;
                std::string msgText;
                size_t posSucc = resp.find("\"success\"");
                if (posSucc != std::string::npos) {
                    size_t posColon = resp.find(':', posSucc);
                    if (posColon != std::string::npos) {
                        size_t posVal = resp.find_first_not_of(" \t\r\n", posColon + 1);
                        if (posVal != std::string::npos && resp.compare(posVal, 4, "true") == 0) success = true;
                    }
                }
                size_t posMsgKey = resp.find("\"message\"");
                if (posMsgKey != std::string::npos) {
                    size_t posColon = resp.find(':', posMsgKey);
                    if (posColon != std::string::npos) {
                        size_t posQuote1 = resp.find('"', posColon + 1);
                        if (posQuote1 != std::string::npos) {
                            size_t posQuote2 = resp.find('"', posQuote1 + 1);
                            if (posQuote2 != std::string::npos) msgText = resp.substr(posQuote1 + 1, posQuote2 - posQuote1 - 1);
                        }
                    }
                }
                if (msgText.empty()) msgText = resp;

                std::wstring wMsg = Utf8ToWide(msgText); 
				std::wstring wTitle;
				if (success) {
				    wTitle = L"\u6CE8\u518C\u6210\u529F";  // “注册成功”
				} else {
				    wTitle = L"\u6CE8\u518C\u5931\u8D25";  // “注册失败”
				}

                MessageBoxW(hwnd, wMsg.c_str(), wTitle.c_str(), MB_OK | (success ? MB_ICONINFORMATION : MB_ICONERROR));
            }
        }
        else if (id == IDC_BUTTON_START && HIWORD(wParam) == BN_CLICKED) {
            // ★修改：点击按钮时，先检查更新
            g_bAutoStartGame = true; // 设置标志，如果无更新则启动
           // ★ Change: Use WITHOUT_UI to hide the "Up to date" popup
            // If an update IS found, the UI will still appear automatically.
            // If NO update is found, it silently calls OnDidNotFindUpdate, which starts the game.
            win_sparkle_check_update_without_ui(); 
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
    // 初始化 WinSparkle
    win_sparkle_set_appcast_url("http://wow.chenmin.org/appcast.xml");
win_sparkle_set_app_details(L"WoW Launcher Project", L"WoW Launcher", L"" APP_VERSION); // 字符串字面量拼接
    
    // 注册回调函数
    win_sparkle_set_did_not_find_update_callback(OnDidNotFindUpdate);
    win_sparkle_set_update_cancelled_callback(OnUpdateCancelled);
    win_sparkle_set_did_find_update_callback(OnDidFindUpdate);
 // ★ 注册关闭回调：这一步至关重要
    win_sparkle_set_shutdown_request_callback(OnShutdownRequest);
    win_sparkle_init();
    
    // 注意：移除了启动时自动检查 update 的代码，避免干扰按钮逻辑

    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "WoWLauncherWndClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "注册窗口类失败", "错误", MB_OK | MB_ICONERROR);
        return 1;
    }

  std::string windowTitle = "WoW 启动器 & 注册 " + std::string(APP_VERSION);
    g_hwnd = CreateWindowExA(0, "WoWLauncherWndClass", windowTitle.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 340, 250, NULL, NULL, hInstance, NULL);

    if (!g_hwnd) return 1;

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    win_sparkle_cleanup();

    return (int)msg.wParam;
}
