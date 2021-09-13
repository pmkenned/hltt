#include "wad.h"
#include <windows.h>
#include <fileapi.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DWORD g_BytesTransferred = 0;

static uint64_t
unpack_le(const unsigned char * ptr, size_t nbytes)
{
    size_t i;
    uint64_t x = 0;
    for (i = 0; i < nbytes; i++) {
        x <<= 8;
        x += ptr[nbytes-i-1];
    }
    return x;
}

enum {
    DIRENT_SIZE = 32
};

VOID CALLBACK FileIOCompletionRoutine(
  __in  DWORD dwErrorCode,
  __in  DWORD dwNumberOfBytesTransfered,
  __in  LPOVERLAPPED lpOverlapped )
{
    (void) lpOverlapped;
    _tprintf(TEXT("Error code:\t%lx\n"), dwErrorCode);
    _tprintf(TEXT("Number of bytes:\t%lx\n"), dwNumberOfBytesTransfered);
    g_BytesTransferred = dwNumberOfBytesTransfered;
}

static wad3_dirent_t
get_dirent(unsigned char * base)
{
    wad3_dirent_t de;
    de.nFilePos         = unpack_le(base +  0, 4);
    de.nDiskSize        = unpack_le(base +  4, 4);
    de.nSize            = unpack_le(base +  8, 4);
    de.nType            = unpack_le(base + 12, 1);
    de.bCompression     = unpack_le(base + 13, 1);
    de.padding          = unpack_le(base + 14, 2);
    de.szName           = (char *) base + 16;
    return de;
}

static wad3_texture_t
get_texture(unsigned char * base)
{
    wad3_texture_t tex;
    tex.szName          = (char *) base;
    tex.nWidth          = unpack_le(base + 16, 4);
    tex.nHeight         = unpack_le(base + 20, 4);
    tex.mip_offset[0]   = unpack_le(base + 24, 4);
    tex.mip_offset[1]   = unpack_le(base + 28, 4);
    tex.mip_offset[2]   = unpack_le(base + 32, 4);
    tex.mip_offset[3]   = unpack_le(base + 36, 4);
    return tex;
}

wad3_t
readWAD(const char * filename)
{
    wad3_t wad3;
    wad3.dirents_cap = 10;
    wad3.dirents_len = 0;
    wad3.dirents = malloc(sizeof(*wad3.dirents)*wad3.dirents_cap);
    wad3.textures_cap = 10;
    wad3.textures_len = 0;
    wad3.textures = malloc(sizeof(*wad3.textures)*wad3.textures_cap);
    uint32_t i;
    char msgbuf[256];
    const char magic[] = {0x57, 0x41, 0x44, 0x33};

    unsigned char * wad_contents = NULL;

    HANDLE hFile = CreateFile(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL
    );

    LARGE_INTEGER FileSize;
    GetFileSizeEx(hFile, &FileSize);

    OVERLAPPED ol = {0};

    wad_contents = malloc(FileSize.u.LowPart);

    if (FALSE == ReadFileEx(hFile, wad_contents, FileSize.u.LowPart, &ol, FileIOCompletionRoutine)) {
        //DisplayError(TEXT("ReadFile"));
        snprintf(msgbuf, sizeof(msgbuf), "Unable to read from file.\n GetLastError=%08lx\n", GetLastError());
        MessageBox(NULL, msgbuf, "Note", MB_OK);
        CloseHandle(hFile);
        return wad3;
    }

    if (memcmp(wad_contents, magic, sizeof(magic)) != 0) {
        snprintf(msgbuf, sizeof(msgbuf), "Error: %s is not a valid WAD file", filename);
        MessageBox(NULL, msgbuf, "Note", MB_OK);
        exit(EXIT_FAILURE);
    }

    uint32_t ntextures = unpack_le(wad_contents + 4, 4);
    uint32_t lumps_offset = unpack_le(wad_contents + 8, 4);

    if (lumps_offset > FileSize.u.LowPart) {
        snprintf(msgbuf, sizeof(msgbuf), "Error reading WAD file");
        MessageBox(NULL, msgbuf, "Note", MB_OK);
        exit(EXIT_FAILURE);
    }

    //snprintf(msgbuf, sizeof(msgbuf), "File size: %lu", FileSize.u.LowPart);
    //MessageBox(NULL, msgbuf, "Note", MB_OK);
    //snprintf(msgbuf, sizeof(msgbuf), "Number of textures: %u", ntextures);
    //MessageBox(NULL, msgbuf, "Note", MB_OK);
    //snprintf(msgbuf, sizeof(msgbuf), "Offset of lump list: %x", lumps_offset);
    //MessageBox(NULL, msgbuf, "Note", MB_OK);

    for (i = 0; i < ntextures; i++) {
        //dirent_t de = get_dirent(wad_contents + lumps_offset + i*DIRENT_SIZE);
        wad3.dirents[wad3.dirents_len++] = get_dirent(wad_contents + lumps_offset + i*DIRENT_SIZE);
        if (wad3.dirents_len >= wad3.dirents_cap) {
            wad3.dirents_cap *= 2;
            wad3.dirents = realloc(wad3.dirents, sizeof(*wad3.dirents)*wad3.dirents_cap);
        }
        wad3_dirent_t * de_p = &wad3.dirents[wad3.dirents_len - 1];
        //snprintf(msgbuf, sizeof(msgbuf), "nFilePos: %u\nnDiskSize: %u\nnSize: %u\nnType: %u\nbCompression: %u\npadding: %u\nszName: %s", de_p->nFilePos, de_p->nDiskSize, de_p->nSize, de_p->nType, de_p->bCompression, de_p->padding, de_p->szName);
        //MessageBox(NULL, msgbuf, "Note", MB_OK);

        wad3.textures[wad3.textures_len++] = get_texture(wad_contents + de_p->nFilePos);
        if (wad3.textures_len >= wad3.textures_cap) {
            wad3.textures_cap *= 2;
            wad3.textures = realloc(wad3.textures, sizeof(*wad3.textures)*wad3.textures_cap);
        }
        //wad3_texture_t * tex_p = &wad3.textures[wad3.textures_len - 1];
        //snprintf(msgbuf, sizeof(msgbuf), "szName: %s", tex_p->szName);
        //MessageBox(NULL, msgbuf, "Note", MB_OK);
    }

    free(wad_contents);

    return wad3;
}

void
wad3_destroy(wad3_t w3)
{
    free(w3.dirents);
    free(w3.textures);
}
