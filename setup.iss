; 脚本开始

#define MyAppName "WoW Launcher"
#define MyAppVersion "1.0.1" 
#define MyAppPublisher "ChenMin WoW"
#define MyAppURL "http://wow.chenmin.org/"
#define MyAppExeName "WowLancher.exe"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-1234-567890ABCDEF}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; 默认目录设置为空或者一个常见路径
; 但因为我们有强制检查逻辑，用户如果选错是过不去的
DefaultDirName={autopf}\

; ★关键：允许覆盖安装到已存在的目录，并不显示警告
DirExistsWarning=no

; ★关键：记住上次的安装路径（第二次更新时，用户就不需要再选了，直接覆盖）
UsePreviousAppDir=yes

; 禁用程序组（让界面更简洁）
DisableProgramGroupPage=yes

OutputBaseFilename=WoWLauncher_Setup_v{#MyAppVersion}
Compression=lzma
SolidCompression=yes

; 安装前关闭旧程序
CloseApplications=yes
RestartApplications=no

[Files]
; 这里的 DestDir: "{app}" 指的就是用户选中的那个魔兽目录
Source: "WowLancher.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "WinSparkle.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "Arctium WoW Launcher.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}" 
[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent

; =====================================================================
; ★以下是自定义界面提示信息
; =====================================================================
[Messages]
; 修改选择目录页面的标题和说明，引导用户
SelectDirLabel3=请选择您的魔兽世界（World of Warcraft）安装目录。
SelectDirBrowseLabel=为了确保游戏正常运行，请点击“浏览”并定位到包含 "World of Warcraft Launcher.exe" 的文件夹。

; =====================================================================
; ★以下是核心代码逻辑：强制检查文件是否存在
; =====================================================================
[Code]
// 这是一个系统回调函数，当用户点击“下一步”时触发
function NextButtonClick(CurPageID: Integer): Boolean;
var
  UserSelectDir: String;
  TargetFile: String;
begin
  // 默认允许通过
  Result := True;

  // 如果当前页面是“选择目录”页面 (wpSelectDir)
  if CurPageID = wpSelectDir then begin
    
    // 获取用户选择的目录路径
    UserSelectDir := WizardDirValue;
    
    // 拼接目标文件路径：用户目录 + \ + 文件名
    TargetFile := AddBackslash(UserSelectDir) + 'World of Warcraft Launcher.exe';
    
    // 检查文件是否存在
    if not FileExists(TargetFile) then begin
      // 如果不存在，弹窗警告，并阻止跳转到下一步 (Result := False)
      MsgBox('错误：在您选择的文件夹中未找到 "World of Warcraft Launcher.exe"！' + #13#10 + #13#10 +
             '本启动器必须安装在魔兽世界游戏根目录下。' + #13#10 + 
             '请点击“浏览”，找到包含该文件的正确文件夹。', mbError, MB_OK);
      Result := False;
    end;
  end;
end;