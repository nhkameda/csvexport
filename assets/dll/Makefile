# Makefile for csvexport.dll - GCC/MinGW build (32-bit)
# Migrated from Visual Studio (vcxproj) project
#
# Usage:
#   make          - build csvexport.dll (32-bit PE32)
#   make clean    - remove build artifacts

CC      = i686-w64-mingw32-gcc
WINDRES = /c/msys64/mingw32/bin/windres

CFLAGS  = -O2 -Wall -Wno-unused-variable -Wno-unused-but-set-variable \
          -DWIN32 -D_WIN32 -D_WINDOWS -DNDEBUG -D_USRDLL \
          -DCSVEXPORT_EXPORTS -D_CRT_SECURE_NO_WARNINGS \
          -DWINVER=0x0500 -D_WIN32_WINNT=0x0500

LDFLAGS = -shared -static-libgcc \
          -Wl,--enable-stdcall-fixup \
          -Wl,--kill-at

LIBS    = -lkernel32 -luser32 -lgdi32 -lcomdlg32 -ladvapi32

TARGET  = csvexport.dll
IMPLIB  = csvexport.lib

OBJS    = csvexport.o winutf8.o strsep.o csvexport_res.o

all: $(TARGET)

$(TARGET): $(OBJS) csvexport.def
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) csvexport.def $(LIBS) -Wl,--out-implib,$(IMPLIB)

csvexport.o: csvexport.c csvexport.h plugin.h rosette.h strsep.h winutf8.h
	$(CC) $(CFLAGS) -c csvexport.c -o csvexport.o

winutf8.o: winutf8.c winutf8.h
	$(CC) $(CFLAGS) -c winutf8.c -o winutf8.o

strsep.o: strsep.c strsep.h
	$(CC) $(CFLAGS) -c strsep.c -o strsep.o

csvexport_res.o: csvexport.rc csvexport.h
	$(WINDRES) -i csvexport.rc -o csvexport_res.o --include-dir=.

clean:
	-del /Q *.o $(TARGET) $(IMPLIB) 2>NUL || rm -f *.o $(TARGET) $(IMPLIB)

.PHONY: all clean
