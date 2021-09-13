#ifndef WAD_H
#define WAD_H

#include <stdint.h>
#include <stddef.h>

enum {
    WAD3_TYPE_TEMP      = 0x40,
    WAD3_TYPE_CACHED    = 0x42,
    WAD3_TYPE_TEX       = 0x43,
    WAD3_TYPE_FONT      = 0x46,
};

typedef struct {
    uint32_t nFilePos;
    uint32_t nDiskSize;
    uint32_t nSize;
    uint8_t  nType;
    uint8_t  bCompression;
    uint16_t padding;
    char *   szName;
} wad3_dirent_t;

typedef struct {
    char * szName;
    uint32_t nWidth;
    uint32_t nHeight;
    uint32_t mip_offset[4];
} wad3_texture_t;

typedef struct {
    size_t dirents_len;
    size_t dirents_cap;
    wad3_dirent_t * dirents;
    size_t textures_len;
    size_t textures_cap;
    wad3_texture_t * textures;
} wad3_t;

wad3_t readWAD(const char * filename);
void wad3_destroy(wad3_t w3);

#endif /* WAD_H */
