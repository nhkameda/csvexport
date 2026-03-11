#!/bin/bash
# Build script for csvexport.dll - GCC/MinGW (bash version)
# Usage: ./build.sh [clean]

set -e

CC=gcc
WINDRES=windres
CFLAGS="-O2 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -DWIN32 -D_WIN32 -D_WINDOWS -DNDEBUG -D_USRDLL -DCSVEXPORT_EXPORTS -D_CRT_SECURE_NO_WARNINGS -DWINVER=0x0500 -D_WIN32_WINNT=0x0500"
LDFLAGS="-shared -static-libgcc -Wl,--enable-stdcall-fixup -Wl,--kill-at"
LIBS="-lkernel32 -luser32 -lgdi32 -lcomdlg32 -ladvapi32"

if [ "$1" = "clean" ]; then
    rm -f *.o csvexport.dll csvexport.lib
    echo "Clean complete."
    exit 0
fi

echo "[1/4] Compiling csvexport.c..."
$CC $CFLAGS -c csvexport.c -o csvexport.o

echo "[2/4] Compiling winutf8.c..."
$CC $CFLAGS -c winutf8.c -o winutf8.o

echo "[3/4] Compiling strsep.c..."
$CC $CFLAGS -c strsep.c -o strsep.o

echo "[4/4] Compiling resources and linking..."
$WINDRES -i csvexport.rc -o csvexport_res.o --include-dir=.

$CC $LDFLAGS -o csvexport.dll csvexport.o winutf8.o strsep.o csvexport_res.o csvexport.def $LIBS -Wl,--out-implib,csvexport.lib

echo ""
echo "Build successful: csvexport.dll"
