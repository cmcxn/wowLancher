@echo off
setlocal
chcp 65001 >nul

:: ===========================================================
:: PART 1: CMD 批处理逻辑
:: ===========================================================

echo 当前目录: %CD%
echo.  
set /p INPUT_VER="请输入新版本号 (例如 1.0.3): "

if "%INPUT_VER%"=="" (
    echo 版本号不能为空！
    pause
    exit /b
)

:: 设置环境变量，传递给下面的 PowerShell
set "TARGET_VERSION=%INPUT_VER%"

echo.  
echo 正在启动 PowerShell 更新逻辑...  
echo -----------------------------------------

:: ★★★ 修复：使用兼容 PowerShell 5.1 的语法 ★★★
powershell -NoProfile -ExecutionPolicy Bypass -Command "$c = Get-Content -LiteralPath '%~f0' -Encoding UTF8; $found = $false; $script = @(); foreach ($line in $c) { if ($line -eq '#:POWERSHELL_START') { $found = $true; continue } if ($found) { $script += $line } }; if ($script.Count -gt 0) { Invoke-Expression ($script -join \"`n\") }"

echo. 
echo -----------------------------------------
echo 全部完成！
pause
goto :eof

:: ===========================================================
:: 下面这行是分界线，不要修改它
#:POWERSHELL_START

# ===========================================================
# PART 2: PowerShell 逻辑 (CMD 只有读取到上面标记后，才会执行下面的代码)
# ===========================================================

$version = $env:TARGET_VERSION
Write-Host "目标版本: $version" -ForegroundColor Cyan
echo ""

$cppFile = "main.cpp"
$issFile = "setup.iss"
$xmlFile = "appcast.xml"

# GBK 编码对象 (代码页 936)
$gbk = [System.Text.Encoding]::GetEncoding(936)

# --- 1. 更新 main.cpp (GBK 编码) - 替换 #define APP_VERSION ---
if (Test-Path $cppFile) {
    $bytes = [System.IO.File]::ReadAllBytes($cppFile)
    $content = $gbk.GetString($bytes)
    
    $newContent = $content -replace '#define APP_VERSION "[^"]+"', ('#define APP_VERSION "' + $version + '"')
    
    if ($content -ne $newContent) {
        $newBytes = $gbk.GetBytes($newContent)
        [System.IO.File]::WriteAllBytes($cppFile, $newBytes)
        Write-Host "SUCCESS: main.cpp 已更新 (#define APP_VERSION)。" -ForegroundColor Green
    } else {
        Write-Host "WARNING: main.cpp 内容未变 (可能版本号一致或正则未匹配)。" -ForegroundColor Yellow
    }
} else {
    Write-Host "ERROR: 找不到 $cppFile" -ForegroundColor Red
}

echo ""

# --- 2. 更新 setup.iss (GBK 编码) ---
if (Test-Path $issFile) {
    $bytes = [System.IO.File]::ReadAllBytes($issFile)
    $content = $gbk.GetString($bytes)
    
    $newContent = $content -replace '#define MyAppVersion "[^"]+"', ('#define MyAppVersion "' + $version + '"')
    
    if ($content -ne $newContent) {
        $newBytes = $gbk.GetBytes($newContent)
        [System.IO.File]::WriteAllBytes($issFile, $newBytes)
        Write-Host "SUCCESS: setup.iss 已更新。" -ForegroundColor Green
    } else {
        Write-Host "WARNING: setup. iss 内容未变 (可能版本号一致或正则未匹配)。" -ForegroundColor Yellow
    }
} else {
    Write-Host "ERROR: 找不到 $issFile" -ForegroundColor Red
}

echo ""

# --- 3. 更新 appcast. xml (UTF8 编码) ---
if (Test-Path $xmlFile) {
    $content = Get-Content $xmlFile -Encoding UTF8 -Raw
    $dateStr = (Get-Date). ToString("ddd, dd MMM yyyy HH:mm:ss +0000")
    
    $newContent = $content
    $newContent = $newContent -replace '<title>Version [^<]+</title>', ('<title>Version ' + $version + '</title>')
    $newContent = $newContent -replace '<sparkle:criticalUpdate version="[^"]+"', ('<sparkle:criticalUpdate version="' + $version + '"')
    $newContent = $newContent -replace '<sparkle:version>[^<]+</sparkle:version>', ('<sparkle:version>' + $version + '</sparkle:version>')
    $newContent = $newContent -replace 'sparkle:version="[^"]+"', ('sparkle:version="' + $version + '"')
    $newContent = $newContent -replace '_v[0-9\. ]+\.exe', ('_v' + $version + '.exe')
    $newContent = $newContent -replace '<pubDate>[^<]+</pubDate>', ('<pubDate>' + $dateStr + '</pubDate>')

    if ($content -ne $newContent) {
        Set-Content $xmlFile $newContent -Encoding UTF8 -NoNewline
        Write-Host "SUCCESS: appcast.xml 已更新。" -ForegroundColor Green
    } else {
        Write-Host "WARNING: appcast.xml 内容未变 (可能版本号一致或正则未匹配)。" -ForegroundColor Yellow
    }
} else {
    Write-Host "ERROR: 找不到 $xmlFile" -ForegroundColor Red
}