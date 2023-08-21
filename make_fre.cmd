REM set target path
set TARGETPATH=.\bin

REM set DDK path
set DDKDIR=%SYSTEMDRIVE%\WinDDK\7600.16385.1

REM remove and recreate target path
rmdir %TARGETPATH% /S /Q
mkdir %TARGETPATH%
mkdir %TARGETPATH%\x86
mkdir %TARGETPATH%\x64

REM build x86 driver
setlocal
call %DDKDIR%\bin\setenv.bat %DDKDIR% fre x86 WNET
cd /d %~dp0
if not exist obj%BUILD_ALT_DIR% mkdir obj%BUILD_ALT_DIR%
build -cewgZ /jpath obj%BUILD_ALT_DIR%
endlocal
if errorlevel 1 goto :error

REM build x64 driver
setlocal
call %DDKDIR%\bin\setenv.bat %DDKDIR% fre x64 WNET
cd /d %~dp0
if not exist obj%BUILD_ALT_DIR% mkdir obj%BUILD_ALT_DIR%
build -cewgZ /jpath obj%BUILD_ALT_DIR%
endlocal
if errorlevel 1 goto :error

REM copy driver files to bin directory
copy gpl.txt %TARGETPATH%\
copy objfre_wnet_x86\i386\genahci.sys %TARGETPATH%\x86\
copy objfre_wnet_AMD64\amd64\genahci.sys %TARGETPATH%\x64\
copy genahci.inf %TARGETPATH%\x86\
copy genahci.inf %TARGETPATH%\x64\
copy txtsetup.oem %TARGETPATH%\

REM sign driver files
setlocal
PATH=%PATH%;%DDKDIR%\bin\selfsign;%DDKDIR%\bin\x86
inf2cat /driver:%TARGETPATH%\x86 /os:Server2003_X86,Server2003_X64 /verbose
inf2cat /driver:%TARGETPATH%\x64 /os:Server2003_X86,Server2003_X64 /verbose
makecert -$ individual -r -pe -ss MY -n CN="genahci" %TARGETPATH%\genahci.cer
signtool.exe sign /v /s MY /n "genahci" %TARGETPATH%\x86\*.sys %TARGETPATH%\x86\*.cat
signtool.exe sign /v /s MY /n "genahci" %TARGETPATH%\x64\*.sys %TARGETPATH%\x64\*.cat
certmgr -del -c -n "genahci" -s -r currentUser MY
endlocal
if errorlevel 1 goto :error

REM cleanup
rmdir "objfre_wnet_x86" /S /Q
rmdir "objfre_wnet_amd64" /S /Q
del %TARGETPATH%\genahci.cer
goto :end

REM show error message
:error
echo Build errors occurred
pause
exit

:end
echo Build succeeded
pause