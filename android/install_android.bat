@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

set "ANDROID_DIR=%~dp0"
set "ADB=D:\Admin\Softwares\Android\Sdk\platform-tools\adb.exe"
set "CLIENT=%ANDROID_DIR%app\build\outputs\apk\debug\app-debug.apk"
set "SERVER=%ANDROID_DIR%server\build\outputs\apk\debug\server-debug.apk"

set "MODE=all"
set "MODE_TEXT=客户端和服务端"
if /i "%~1"=="client" (
    set "MODE=client"
    set "MODE_TEXT=仅客户端"
)
if /i "%~1"=="server" (
    set "MODE=server"
    set "MODE_TEXT=仅服务端"
)

if not exist "%ADB%" set "ADB=adb"
if not exist "%CLIENT%" (
    echo 找不到客户端 APK：
    echo %CLIENT%
    echo.
    pause
    exit /b 1
)
if not exist "%SERVER%" (
    echo 找不到服务端 APK：
    echo %SERVER%
    echo.
    pause
    exit /b 1
)

echo ============================================================
echo UNOOL 安卓端安装脚本
echo 安装内容：%MODE_TEXT%
echo 客户端 APK：%CLIENT%
echo 服务端 APK：%SERVER%
echo ============================================================
echo.

echo 当前 adb 设备列表：
"%ADB%" devices
echo.

set "DEVICE_COUNT=0"
set "FAILED=0"

for /f "tokens=1,2" %%A in ('"%ADB%" devices') do (
    if "%%B"=="device" (
        set /a DEVICE_COUNT+=1
        echo -------- 正在安装到设备 %%A --------
        if /i "%MODE%"=="all" (
            echo 安装客户端...
            "%ADB%" -s %%A install -r "%CLIENT%"
            if errorlevel 1 set "FAILED=1"
            echo 安装服务端...
            "%ADB%" -s %%A install -r "%SERVER%"
            if errorlevel 1 set "FAILED=1"
        ) else if /i "%MODE%"=="client" (
            echo 安装客户端...
            "%ADB%" -s %%A install -r "%CLIENT%"
            if errorlevel 1 set "FAILED=1"
        ) else if /i "%MODE%"=="server" (
            echo 安装服务端...
            "%ADB%" -s %%A install -r "%SERVER%"
            if errorlevel 1 set "FAILED=1"
        )
        echo.
    ) else if "%%B"=="unauthorized" (
        echo 设备 %%A 未授权：请解锁手机并允许 USB 调试。
        echo.
    )
)

if "%DEVICE_COUNT%"=="0" (
    echo 没有找到已授权设备。
    echo 请连接手机、解锁屏幕，并允许 USB 调试。
    echo.
    set "FAILED=1"
)

if "%FAILED%"=="0" (
    echo 完成：安装成功。
) else (
    echo 完成：部分安装失败。
    echo 如果手机拒绝了安装权限，请解锁手机、点“允许通过 USB 安装”，然后重新运行本脚本。
)

echo.
pause
exit /b %FAILED%
