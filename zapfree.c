#include <windows.h>
#include <stdio.h>

static void ple(const char *pre) {
    char *buf;
    DWORD dw = GetLastError(); 
    FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&buf, 0, NULL);
    fprintf(stderr, "\n%s%s\n", pre, buf);
    LocalFree(buf);
}

#define ZAPSIZE 65536

static BOOLEAN zap(HANDLE h, ULONGLONG length) {
    /* Allocate only once */
    static int alloc = 1;
    static char *buf;
    if (alloc) {
        buf = (char *)malloc(ZAPSIZE);
        if (!buf) {
            return FALSE;
        }
        memset(buf, 0, ZAPSIZE);
        alloc = 0;
    }
    ULONGLONG total = 0;
    ULONG bytesWritten;
    while (total < length) {
        ULONG bytesToWrite = (length - total > 1024*1024) ? 1024*1024 : (ULONG)(length - total);
        if (bytesToWrite > ZAPSIZE) {
            bytesToWrite = ZAPSIZE;
        }
        if (!WriteFile(h, buf, bytesToWrite, &bytesWritten, NULL)) {
            return FALSE;
        }
        total += bytesWritten;
    }
    return TRUE;
}

static int zapfreespace(const char *drive) {
    DWORD sectorsPerCluster, bytesPerSector, freeClusters, totalClusters;
    ULARGE_INTEGER bytesAvailable, totalBytes, totalFreeBytes;
    char fname[MAX_PATH];

    printf("Zapping free space on %s", drive);
    fflush(stdout);

    if (!GetDiskFreeSpace(drive, &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters)) {
        ple("Could not determine disk cluster size: ");
        return 1;
    }
    if (!GetDiskFreeSpaceExA(drive, &bytesAvailable, &totalBytes, &totalFreeBytes)) {
        ple("Could not determine free space: ");
        return 1;
    }
    if (bytesAvailable.QuadPart != totalFreeBytes.QuadPart) {
        fprintf(stderr, "Disk quota detected. Can not zap free space.\n");
        return 1;
    }

    snprintf(fname, sizeof(fname), "%szapfree.dat", drive);
    HANDLE h = CreateFile(fname, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS,
            FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_DELETE_ON_CLOSE
            |FILE_ATTRIBUTE_HIDDEN, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        ple("Could not create zapfree.dat: ");
        return 1;
    }
    ULONGLONG zapSize = (ULONGLONG)sectorsPerCluster * (ULONGLONG)bytesPerSector * 128;
    ULONGLONG tmpSize = 0;
    int prevPercent = -1;
    while (zapSize > bytesPerSector * sectorsPerCluster) {
        if (zap(h, zapSize)) {
            tmpSize += zapSize;
            int percent = (DWORD) ((tmpSize * 100)/ totalFreeBytes.QuadPart);
            if (percent != prevPercent) {
                printf("\rZapping free space on %s: %d%%", drive, percent);
            }
            prevPercent = percent;
        } else {
            zapSize -= bytesPerSector * sectorsPerCluster;
        }
    }

    snprintf(fname, sizeof(fname), "%szapfree1.dat", drive);
    h = CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_NEW,
            FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_DELETE_ON_CLOSE|FILE_ATTRIBUTE_HIDDEN
            |FILE_FLAG_WRITE_THROUGH, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        while (zapSize) {
            if (!zap(h, zapSize)) {
                zapSize--;
            }
        }
    }

    /* Handle MFT */
    char wheel[8] = "|/-\\|/-\\";
    int nrFiles = 0;
    ULONGLONG prevSize = 4096;
    while (1) {
        snprintf(fname, sizeof(fname), "%szapmft%06d.dat", drive, nrFiles++);
        h = CreateFile( fname, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_DELETE_ON_CLOSE|FILE_ATTRIBUTE_HIDDEN, NULL);
        if (h == INVALID_HANDLE_VALUE ) {
            break;
        }
        zapSize = prevSize;
        BOOLEAN created = FALSE;
        while (zapSize) {
            if (!zap(h, zapSize)) {
                zapSize--;
            } else {
                prevSize = zapSize;
                created = TRUE;
            }
        }       
        if (!created) {
            break;
        }
        if (nrFiles == 1) {
            printf("\r                                     ");
        } 
        printf("\rZapping MFT...%c", wheel[nrFiles % 8]);
    }

    return 0;
}

int main(int argc, char **argv) {
    char path[MAX_PATH];
    if (argc != 2) {
        fprintf(stderr, "Usage: zapfree <DriveLetter>:\n");
        return 1;
    }
    if (!GetFullPathNameA(argv[1], MAX_PATH, path, NULL)) {
        ple("GetFullPathNameA failed: ");
        return 1;
    }
    if (!(isalpha(path[0]) && path[1] == ':' && path[2] == '\\' && path[3] == '\0')) {
        fprintf(stderr, "%s is an invalid drive specification\n", argv[1]);
        return 1;
    }
    return zapfreespace(path);
}
