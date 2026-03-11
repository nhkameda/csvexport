@echo off
REM Build script for csvexport.dll - GCC/MinGW
REM Usage: build.bat [clean]

if "%1"=="clean" (
    del /Q *.o csvexport.dll csvexport.lib 2>NUL
    echo Clean complete.
    exit /b 0
)

set CC=gcc
set WINDRES=windres
set CFLAGS=-O2 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -DWIN32 -D_WIN32 -D_WINDOWS -DNDEBUG -D_USRDLL -DCSVEXPORT_EXPORTS -D_CRT_SECURE_NO_WARNINGS -DWINVER=0x0500 -D_WIN32_WINNT=0x0500
set LDFLAGS=-shared -static-libgcc -Wl,--enable-stdcall-fixup -Wl,--kill-at
set LIBS=-lkernel32 -luser32 -lgdi32 -lcomdlg32 -ladvapi32

echo [1/4] Compiling csvexport.c...
%CC% %CFLAGS% -c csvexport.c -o csvexport.o
if errorlevel 1 goto :error

echo [2/4] Compiling winutf8.c...
%CC% %CFLAGS% -c winutf8.c -o winutf8.o
if errorlevel 1 goto :error

echo [3/4] Compiling strsep.c...
%CC% %CFLAGS% -c strsep.c -o strsep.o
if errorlevel 1 goto :error

echo [4/4] Compiling resources and linking...
%WINDRES% -i csvexport.rc -o csvexport_res.o --include-dir=.
if errorlevel 1 goto :error

%CC% %LDFLAGS% -o csvexport.dll csvexport.o winutf8.o strsep.o csvexport_res.o csvexport.def %LIBS% -Wl,--out-implib,csvexport.lib
if errorlevel 1 goto :error

echo.
echo Build successful: csvexport.dll
exit /b 0

:error
echo.
echo BUILD FAILED
exit /b 1
