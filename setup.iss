; 脚本开始

#define MyAppName "WoW Launcher"
#define MyAppVersion "1.0.1" 
#define MyAppPublisher "ChenMin WoW"
#define MyAppURL "http://wow.chenmin.org/"
#define MyAppExeName "WowLauncher.exe"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-1234-567890ABCDEF}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

DefaultDirName={autopf}\World of Warcraft
DirExistsWarning=no
UsePreviousAppDir=yes
DisableProgramGroupPage=yes
OutputBaseFilename=WoWLauncher_Setup_v{#MyAppVersion}
Compression=lzma
SolidCompression=yes
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "chinesesimplified"; MessagesFile: "ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加图标"; Flags: unchecked

[Files]
Source: "WowLauncher.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "WinSparkle.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "Arctium WoW Launcher.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "运行 {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Messages]
SelectDirLabel3=请选择您的魔兽世界（World of Warcraft）安装目录。
SelectDirBrowseLabel=安装程序会自动尝试检测您的游戏路径。如果没有检测到，请手动点击“浏览”并定位到包含 "World of Warcraft Launcher.exe" 的文件夹。

; =====================================================================
; ★ 核心代码区域
; =====================================================================
[Code]

const
  TargetFileName = 'World of Warcraft Launcher.exe';
  // 手动定义文件夹属性常量
  faDirectory = $10;

// --- 辅助函数：检查某目录下是否有目标文件 ---
function FindFileInDir(Dir: String): Boolean;
begin
  Result := FileExists(AddBackslash(Dir) + TargetFileName);
end;

// --- 辅助函数：深度递归查找 ---
function RecursiveSearch(StartDir: String; CurrentDepth, MaxDepth: Integer): String;
var
  FindRec: TFindRec;
  SubDir: String;
  DirNameLower: String;
begin
  Result := '';
  
  if FindFileInDir(StartDir) then begin
    Result := StartDir;
    Exit;
  end;

  if CurrentDepth >= MaxDepth then Exit;

  // ★★★ 修复点：这里只传 2 个参数 ★★★
  // 我们查找所有内容('*')，然后在下面用 if 判断它是不是文件夹
  if FindFirst(AddBackslash(StartDir) + '*', FindRec) then begin
    try
      repeat
        // 判断是否为文件夹 ((FindRec.Attributes and faDirectory) <> 0)
        if (FindRec.Name <> '.') and (FindRec.Name <> '..') and ((FindRec.Attributes and faDirectory) <> 0) then begin
          
          SubDir := AddBackslash(StartDir) + FindRec.Name;
          DirNameLower := Lowercase(FindRec.Name);
          
          // 排除逻辑：不进入系统敏感目录，提高速度
          if (Pos('windows', DirNameLower) = 0) and 
             (Pos('users', DirNameLower) = 0) and 
             (Pos('programdata', DirNameLower) = 0) and
             (Pos('$recycle.bin', DirNameLower) = 0) and
             (Pos('system volume information', DirNameLower) = 0) then
          begin
             Result := RecursiveSearch(SubDir, CurrentDepth + 1, MaxDepth);
             if Result <> '' then Exit;
          end;
        end;
      until not FindNext(FindRec);
    finally
      FindClose(FindRec);
    end;
  end;
end;

// --- 主查找逻辑 ---
function AutoFindWoWPath(): String;
var
  Drives: array of String;
  i: Integer;
  RegPath: String;
begin
  Result := '';

  // 1. 注册表
  if RegQueryStringValue(HKLM64, 'SOFTWARE\Blizzard Entertainment\World of Warcraft', 'InstallPath', RegPath) then begin
    if FindFileInDir(RegPath) then begin
      Result := RemoveBackslash(RegPath);
      Exit;
    end;
  end;
  if RegQueryStringValue(HKLM32, 'SOFTWARE\Blizzard Entertainment\World of Warcraft', 'InstallPath', RegPath) then begin
    if FindFileInDir(RegPath) then begin
      Result := RemoveBackslash(RegPath);
      Exit;
    end;
  end;

  // 2. 全盘扫描
  Drives := ['C:', 'D:', 'E:', 'F:', 'G:', 'H:'];

  for i := 0 to GetArrayLength(Drives) - 1 do begin
    if DirExists(Drives[i]) then begin
      Result := RecursiveSearch(Drives[i], 0, 4); // 搜索深度4层
      if Result <> '' then Exit;
    end;
  end;
end;

// --- 系统回调 ---
procedure InitializeWizard;
var
  DetectedPath: String;
begin
  DetectedPath := AutoFindWoWPath();
  if DetectedPath <> '' then begin
    WizardForm.DirEdit.Text := DetectedPath;
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  UserSelectDir: String;
  TargetFile: String;
begin
  Result := True;

  if CurPageID = wpSelectDir then begin
    UserSelectDir := WizardDirValue;
    TargetFile := AddBackslash(UserSelectDir) + TargetFileName;
    
    if not FileExists(TargetFile) then begin
      MsgBox('错误：在目录 "' + UserSelectDir + '" 中未找到 "' + TargetFileName + '"！' + #13#10 + #13#10 +
             '请手动点击“浏览”，找到包含该文件的正确游戏目录。', mbError, MB_OK);
      Result := False;
    end;
  end;
end;