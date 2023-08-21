REM set target path
set TARGETPATH=.\bin

REM set DDK path
set DDKDIR=C:\WinDDK\7600.16385.1

REM remove and recreate target path
rmdir %TARGETPATH% /S /Q
mkdir %TARGETPATH%
mkdir %TARGETPATH%\x86
mkdir %TARGETPATH%\x64

REM build x86 driver
setlocal
call %DDKDIR%\bin\setenv.bat %DDKDIR% chk x86 WNET
cd /d %~dp0
if not exist obj%BUILD_ALT_DIR% mkdir obj%BUILD_ALT_DIR%
build -cewgZ /jpath obj%BUILD_ALT_DIR%
endlocal
if errorlevel 1 goto :error

REM build x64 driver
setlocal
call %DDKDIR%\bin\setenv.bat %DDKDIR% chk x64 WNET
cd /d %~dp0
if not exist obj%BUILD_ALT_DIR% mkdir obj%BUILD_ALT_DIR%
build -cewgZ /jpath obj%BUILD_ALT_DIR%
endlocal
if errorlevel 1 goto :error

REM copy driver files to bin directory
copy gpl.txt %TARGETPATH%\
copy objchk_wnet_x86\i386\storahci.sys %TARGETPATH%\x86\
copy objchk_wnet_AMD64\amd64\storahci.sys %TARGETPATH%\x64\
copy storahci.inf %TARGETPATH%\x86\
copy storahci.inf %TARGETPATH%\x64\
copy txtsetup.oem %TARGETPATH%\

REM sign driver files
setlocal
PATH=%PATH%;%DDKDIR%\bin\selfsign;%DDKDIR%\bin\x86
inf2cat /driver:%TARGETPATH%\x86 /os:Server2003_X86,Server2003_X64 /verbose
inf2cat /driver:%TARGETPATH%\x64 /os:Server2003_X86,Server2003_X64 /verbose
makecert -$ individual -r -pe -ss MY -n CN="StorAhci" %TARGETPATH%\StorAhci.cer
signtool.exe sign /v /s MY /n "StorAhci" %TARGETPATH%\x86\*.sys %TARGETPATH%\x86\*.cat
signtool.exe sign /v /s MY /n "StorAhci" %TARGETPATH%\x64\*.sys %TARGETPATH%\x64\*.cat
certmgr -del -c -n "StorAhci" -s -r currentUser MY
endlocal
if errorlevel 1 goto :error

REM cleanup
REM do not delete the checked object folders, because we need the PDB file for debugging
REM rmdir "objchk_wnet_x86" /S /Q
REM rmdir "objchk_wnet_amd64" /S /Q
del %TARGETPATH%\StorAhci.cer
goto :end

REM show error message
:error
echo Build errors occurred
pause
exit

:end
echo Build succeeded
pause