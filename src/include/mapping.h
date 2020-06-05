#ifndef MAPPING_H_INCLUDE
#define MAPPING_H_INCLUDE

#include <gb/gb.h>

#define used_tiles_count 170

typedef struct tiledesc_t {
    UBYTE count;
    unsigned char * data;
} tiledesc_t;

extern const unsigned char viewport_map[];
void set_view_port(UBYTE x, UBYTE y) __banked;

extern const tiledesc_t * used_tile_range;
void copy_tiles() __nonbanked;

#endif