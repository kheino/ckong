// --------------------------------------------------------------------------
//
// C Kong
// Copyright (C) 2018 Jeff Panici
// All rights reserved.
//
// This software source file is licensed according to the
// MIT License.  Refer to the LICENSE file distributed along
// with this source file to learn more.
//
// --------------------------------------------------------------------------

#include <assert.h>
#include <SDL2/SDL_surface.h>
#include "log.h"
#include "tile.h"
#include "video.h"
#include "sprite.h"
#include "window.h"
#include "palette.h"
#include "tile_map.h"

static SDL_Surface* s_bg_surface;

static SDL_Surface* s_fg_surface;

static spr_control_block_t s_spr_control[SPRITE_MAX];

static bg_control_block_t s_bg_control[TILE_MAP_SIZE];

static void video_bg_update(void) {
    SDL_LockSurface(s_bg_surface);
    uint32_t tx = 0;
    uint32_t ty = 0;
    for (uint32_t i = 0; i < TILE_MAP_SIZE; i++) {
        bg_control_block_t* block = &s_bg_control[i];
        if ((block->flags & f_bg_changed) == 0
        ||  (block->flags & f_bg_enabled) == 0)
            continue;

        const palette_t* pal = palette(block->palette);
        if (pal == NULL)
            continue;

        const tile_bitmap_t* bitmap = tile_bitmap(block->tile);
        if (bitmap == NULL)
            continue;

        bool horizontal_flip = (block->flags & f_bg_hflip) != 0;
        bool vertical_flip = (block->flags & f_bg_vflip) != 0;

        uint8_t sy = (uint8_t) (vertical_flip ? TILE_HEIGHT - 1 : 0);
        int8_t syd = (int8_t) (vertical_flip ? -1 : 1);
        int8_t sxd = (int8_t) (horizontal_flip ? -1 : 1);

        for (uint32_t y = 0; y < TILE_HEIGHT; y++) {
            uint8_t* p = s_bg_surface->pixels + ((ty + y) * s_bg_surface->pitch + (tx * 4));
            uint8_t sx = (uint8_t) (horizontal_flip ? TILE_WIDTH - 1 : 0);
            for (uint32_t x = 0; x < TILE_WIDTH; x++) {
                const uint32_t pixel_offset = (const uint32_t) (sy * TILE_WIDTH + sx);
                const palette_entry_t* pal_entry = &pal->entries[bitmap->data[pixel_offset]];
                *p++ = pal_entry->red;
                *p++ = pal_entry->green;
                *p++ = pal_entry->blue;
                *p++ = 0xff;
                sx += sxd;
            }
            sy += syd;
        }

        block->flags &= ~f_bg_changed;

        tx += TILE_WIDTH;
        if (tx == screen_width) {
            tx = 0;
            ty += TILE_HEIGHT;
        }
    }
    SDL_UnlockSurface(s_bg_surface);
    SDL_BlitSurface(s_bg_surface, NULL, s_fg_surface, NULL);
}

static void video_fg_update(void) {
    SDL_LockSurface(s_fg_surface);
    for (uint32_t i = 0; i < SPRITE_MAX; i++) {
        spr_control_block_t* block = &s_spr_control[i];

        if ((block->flags & f_spr_enabled) == 0)
            continue;

        const palette_t* pal = palette(block->palette);
        if (pal == NULL)
            continue;

        const sprite_bitmap_t* bitmap = sprite_bitmap(block->tile);
        if (bitmap == NULL)
            continue;

        bool horizontal_flip = (block->flags & f_spr_hflip) != 0;
        bool vertical_flip = (block->flags & f_spr_vflip) != 0;

        uint8_t sy = (uint8_t) (vertical_flip ? SPRITE_HEIGHT - 1 : 0);
        int8_t syd = (int8_t) (vertical_flip ? -1 : 1);
        int8_t sxd = (int8_t) (horizontal_flip ? -1 : 1);

        for (uint32_t y = 0; y < SPRITE_HEIGHT; y++) {
            uint8_t* p = s_fg_surface->pixels +
                ((block->y + y) * s_fg_surface->pitch +
                 (block->x * 4));
            uint8_t sx = (uint8_t) (horizontal_flip ? SPRITE_WIDTH - 1 : 0);
            for (uint32_t x = 0; x < SPRITE_WIDTH; x++) {
                const uint32_t pixel_offset = (const uint32_t) (sy * SPRITE_WIDTH + sx);
                const palette_entry_t* pal_entry = &pal->entries[bitmap->data[pixel_offset]];
                if (pal_entry->alpha == 0x00)
                    p += 4;
                else {
                    *p++ = pal_entry->red;
                    *p++ = pal_entry->green;
                    *p++ = pal_entry->blue;
                    *p++ = pal_entry->alpha;
                }
                sx += sxd;
            }
            sy += syd;
        }

        block->flags &= ~f_spr_changed;
    }
    SDL_UnlockSurface(s_fg_surface);
}

void video_init(void) {
    log_message(category_video, "allocate RGBA8888 bg surface.");
    s_bg_surface = SDL_CreateRGBSurfaceWithFormat(
        0,
        screen_width,
        screen_height,
        32,
        SDL_PIXELFORMAT_RGBA8888);
    log_message(category_video, "set s_bg_surface blend mode: none.");
    SDL_SetSurfaceBlendMode(s_bg_surface, SDL_BLENDMODE_NONE);

    log_message(category_video, "allocate RGBA8888 fg surface.");
    s_fg_surface = SDL_CreateRGBSurfaceWithFormat(
        0,
        screen_width,
        screen_height,
        32,
        SDL_PIXELFORMAT_RGBA8888);
    log_message(category_video, "set s_fg_surface blend mode: none.");
    SDL_SetSurfaceBlendMode(s_fg_surface, SDL_BLENDMODE_NONE);
    log_message(category_video, "set s_bg_surface RLE enabled.");
    SDL_SetSurfaceRLE(s_fg_surface, SDL_TRUE);
}

void video_update(void) {
    video_bg_update();
    video_fg_update();
}

void video_shutdown(void) {
    log_message(category_video, "free bg surface.");
    SDL_FreeSurface(s_bg_surface);
    log_message(category_video, "free fg surface.");
    SDL_FreeSurface(s_fg_surface);
}

void video_reset_sprites(void) {
    for (uint32_t i = 0; i < SPRITE_MAX; i++) {
        s_spr_control[i].x = 0;
        s_spr_control[i].y = 0;
        s_spr_control[i].tile = 0;
        s_spr_control[i].palette = 0;
        s_spr_control[i].flags = f_spr_none;
    }
}

SDL_Surface* video_surface(void) {
    return s_fg_surface;
}

void video_set_bg(const tile_map_t* map) {
    assert(map != NULL);

    for (uint32_t i = 0; i < TILE_MAP_SIZE; i++) {
        s_bg_control[i].tile = map->data[i].tile;
        s_bg_control[i].palette = map->data[i].palette;
        s_bg_control[i].flags = map->data[i].flags | f_bg_enabled | f_bg_changed;
    }
}

spr_control_block_t* video_sprite(uint8_t number) {
    return &s_spr_control[number];
}

bg_control_block_t* video_tile(uint8_t y, uint8_t x) {
    return &s_bg_control[y * TILE_MAP_WIDTH + x];
}
