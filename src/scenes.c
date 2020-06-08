#include "scenes.h"
#include "shadow.h"

// pointer to tile resources
static const unsigned char * __tiles, *__empty;
static UBYTE __put_map_x, __put_map_y, __put_map_id;

void initialize_tiles(const unsigned char * tiles, const unsigned char * empty) {
    __tiles = tiles, __empty = empty;
}

UBYTE __dy, __counter;
void __merge(unsigned char * dest, const unsigned char * spr, const unsigned char * mask) __naked {
    dest; spr; mask;
__asm
        push    BC

        lda     HL, 9(SP)
        ld      D, (HL)
        dec     HL
        ld      E, (HL)
        dec     HL
        ld      B, (HL)
        dec     HL
        ld      C, (HL)
        dec     HL
        ld      A, (HL-)
        ld      L, (HL)
        ld      H, A

        ;; now HL: data, DE: mask, BC: item 

        ;; HL += ___dy << 1
        ld      A, (#___dy)
        add     A
        add     L
        ld      L, A
        adc     H
        sub     L
        ld      H, A

        ;; item is 2 tiles high
        ld      A, #0x10
        ld      (#___counter), A
1$:        
        ;; AND with item mask
        ld      A, (DE)
        and     (HL)
        ld      (HL+), A
        inc     DE
        
        ld      A, (DE)
        and     (HL)
        ld      (HL), A
        inc     DE
        
        dec     HL
        
        ;; OR with item
        ld      A, (BC)
        or      (HL)
        ld      (HL+), A
        inc     BC
        
        ld      A, (BC)
        or      (HL)
        ld      (HL+), A
        inc     BC        
        
        ;; check moving to the next tile by Y
        ld      A, (#___dy)
        inc     A
        cp      #8
        jr      NZ, 2$
        
        push    DE

        ;; move to next tile by Y
        ld      DE, #((viewport_width - 1) * 16)
        add     HL, DE
                
        ;; check shadow buffer boundaries: HL < shadow_buffer + sizeof(shadow_buffer)        
        ld      A, #>((_shadow_buffer + 0x0ea0)) 
        cp      H
        jr      C, 3$
        jr      NZ, 4$
   
        ld      A, #<((_shadow_buffer + 0x0ea0))
        cp      L
        jr      C, 3$
   
4$:        
        pop     DE        
        xor     A
        
2$:     ld      (#___dy), A
                
        ld      A, (#___counter)
        dec     A
        ld      (#___counter), A
        jr      NZ, 1$
        
        pop     BC
        
        ret
        
3$:     pop     DE
        pop     BC
        ret
        
__endasm;
}

void put_map() { 
    static unsigned char * data, * spr, * mask, * limit;
        
    if (__put_map_y < ((viewport_height - 1) * 8)) {       
        
        if (__put_map_id != 0xffu) {
            spr = (unsigned char *)&__tiles[(int)__put_map_id << 7u], mask = spr + 0x40u;
        }  else {
            spr = mask = (unsigned char *)__empty;
        }
                          
        data = (unsigned char *)shadow_rows[(__put_map_y >> 3u)];
        data += ((WORD)(__put_map_x) << 4u);
        
        if (data > shadow_buffer_limit) return;
        
        __dy = (__put_map_y & 7u);
        __merge(data, spr, mask);
        
        // next column of 2x2 tile item and mask
        spr += 0x20u, mask += 0x20u;
        
        __dy = (__put_map_y & 7u);
        __merge(data + 0x10u, spr, mask);

    }
}

/*
// old pure C put_map() for reference
void put_map() { 
    static UBYTE i, oy, dy;
    static unsigned char * data1, * data2, * spr, * mask, * limit;
        
    if (__put_map_y < ((viewport_height - 1) * 8)) {       
        oy = __put_map_y >> 3u;
        
        if (__put_map_id != 0xffu) {
            spr = (unsigned char *)&__tiles[(int)__put_map_id << 7u];
            mask = spr + 0x40u;
        }  else {
            spr = mask = (unsigned char *)__empty;
        }
               
        dy = (__put_map_y & 7u);
           
        data1 = (unsigned char *)shadow_rows[oy];
        data1 += ((WORD)(__put_map_x) << 4u);
        data1 += (dy << 1u);
        data2 = data1 + 16u;
        
        if (data1 > shadow_buffer_limit) return;
        
        for (i = 0u; i < 0x10u; i++) {
            *data1++ = *data1 & *mask++ | *spr++;
            *data1++ = *data1 & *mask++ | *spr++;
            dy++;
            if (dy == 8u) {
                dy = 0; 
                data1 += ((viewport_width - 1) * 16);
                if (data1 > shadow_buffer_limit) break; 
            }
        }
        
        dy = (__put_map_y & 7u);  

        for (i = 0u; i < 0x10u; i++) {
            *data2++ = *data2 & *mask++ | *spr++;
            *data2++ = *data2 & *mask++ | *spr++;
            dy++;
            if (dy == 8u) {
                dy = 0; 
                data2 += ((viewport_width - 1) * 16);
                if (data2 > shadow_buffer_limit) break; 
            }
        }
    }
}
*/

void redraw_scene(scene_item_t * scene) {
    static scene_item_t * item;
    item = scene;
    item = item->next; // skip zero item of the scene
    while (item) {
        __put_map_x = item->x, __put_map_y = item->y, __put_map_id = item->id;
        put_map();
        item = item->next;
    }
}

void draw_pattern_XYZ(UBYTE x, UBYTE y, UBYTE z, UBYTE id) {
    __put_map_x = to_x(x, y, z), __put_map_y = to_y(x, y, z), __put_map_id = id;
    put_map();
}

void erase_item(scene_item_t * item) {
    __put_map_x = item->x, __put_map_y = item->y, __put_map_id = 0xffu;
    put_map();
}

void replace_scene_item(scene_item_t * scene, scene_item_t * new_item) {
    static scene_item_t * item, * replace;
    static UBYTE done_ins, done_rep;
    item = scene->next;
    done_ins = done_rep = 0;
    replace = new_item->next;
    while (item) {
        if (item->next == new_item) {
            item->next = replace;
            if (done_ins) return;
            done_rep = 1;
        }
        if ((!done_ins) && ((!(item->next)) || (item->next->coords > new_item->coords))) {
            new_item->next = item->next;
            item->next = new_item;
            item = item->next;
            if (done_rep) return;
            done_ins = 1;
        }
        item = item->next;
    }
}

UBYTE copy_scene(const scene_item_t * sour, scene_item_t * dest) {
    static scene_item_t * src, * dst;
    UBYTE count;

    src = (scene_item_t *)sour, dst = dest;
    count = 0u;

    // zero item must always exist to simplify insertion of objects; it is not drawn
    dst->id = 0xffu, dst->x = 0u, dst->y = 0u, dst->coords = 0u, dst->next = dst + 1;
    dst++;

    while (src) {
        dst->id = src->id;
        dst->x = src->x;
        dst->y = src->y;
        dst->coords = src->coords;
        src = src->next;
        count++;
        if (count == 255u) src = 0;
        if (src) dst->next = dst + 1; else dst->next = 0;
        dst++;
    }

    return count;
}

void clear_map(scene_t * dest) {
    static unsigned char * tmp;
    static UWORD sz;
    sz = sizeof(*dest), tmp = (unsigned char *)dest;
    while (sz) *tmp++ = 0u, sz--;    
}

void scene_to_map(const scene_item_t * sour, scene_t * dest) {
    static scene_item_t * src;
    static UBYTE x, y, z;
    
    clear_map(dest);
    
    src = (scene_item_t *)sour;
    while (src) {
        from_coords(src->coords, x, y, z);
        if ((x < max_scene_x) && (y < max_scene_y) && (z < max_scene_z)) {
            (*dest)[x][z][y] = src->id + 1;
        }
        src = src->next;
    }    
}