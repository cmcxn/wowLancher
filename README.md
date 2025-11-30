# WoW Launcher

一个用于魔兽世界怀旧服（Classic Era）的启动器程序，支持账号注册、一键启动游戏以及自动更新功能。

## 功能特性

- **账号注册**：支持在启动器中直接注册游戏账号
- **一键启动**：自动配置服务器连接并启动游戏
- **自动更新**：集成 WinSparkle 自动检测并更新到最新版本

## 系统要求

- Windows 操作系统
- 魔兽世界经典怀旧服客户端
- Arctium WoW Launcher.exe（用于启动游戏）

## 编译构建

### 依赖项

- MinGW-w64 编译器（推荐使用 Dev-C++ 自带的版本）
- WinSparkle 库（用于自动更新功能）

### 编译步骤

1. 安装 Dev-C++ 或配置 MinGW-w64 环境
2. 下载 [WinSparkle](https://winsparkle.org/) 并将库文件放置到 MinGW 的 lib 目录，头文件放置到 include 目录
3. 打开命令行，进入项目目录
4. 运行编译命令：
   ```bash
   mingw32-make -f Makefile.win
   ```

## 使用说明

1. 将编译好的 `WowLancher.exe` 放置到游戏客户端根目录
2. 确保 `Arctium WoW Launcher.exe` 在同一目录下
3. 运行 `WowLancher.exe`
4. 程序启动时会自动检查更新
5. 填写注册信息或直接点击"启动游戏"

## 自动更新

本程序使用 [WinSparkle](https://winsparkle.org/) 实现自动更新功能：

- 程序启动时自动检查是否有新版本
- 如果有新版本，会提示用户下载更新
- 更新完成后会重新启动程序并打开主界面

### 配置更新源

更新源通过 appcast.xml 文件配置，默认地址为：
```
https://wow.chenmin.org/appcast.xml
```

appcast.xml 示例格式：
```xml
<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0" xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
  <channel>
    <title>WoW Launcher Updates</title>
    <item>
      <title>Version 1.1.0</title>
      <sparkle:version>1.1.0</sparkle:version>
      <sparkle:os>windows</sparkle:os>
      <enclosure url="https://wow.chenmin.org/downloads/WowLancher-1.1.0.exe"
                 length="1234567"
                 type="application/octet-stream"/>
    </item>
  </channel>
</rss>
```

## 项目结构

```
├── main.cpp              # 主程序源代码
├── Makefile.win          # MinGW 编译配置
├── resource.h            # 资源头文件
├── resource.rc           # 资源文件
├── WowLancher.dev        # Dev-C++ 项目文件
├── app_icon.ico          # 程序图标
└── README.md             # 本文档
```

## 许可证

MIT License

## 联系方式

如有问题或建议，请联系项目维护者。
