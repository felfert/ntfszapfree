CC = x86_64-w64-mingw32-gcc
CC32 = i686-w64-mingw32-gcc
CFLAGS = -D_WIN32_WINNT=0x0500 -Wall -Wextra -mwin32
LDLAGS = -mconsole -static -static-libgcc

all: zapfree32.exe zapfree.exe

clean:
	rm -f *.exe

zapfree.exe: zapfree.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

zapfree32.exe: zapfree.c
	$(CC32) $(CFLAGS) $(LDFLAGS) -o $@ $<
