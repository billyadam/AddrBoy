// AddrBoy logo "Book Boy": drawn in code, so no PNG/BMP or grit step is needed.
// 4bpp tiles. Each bn::tile is 8 rows of uint32; pixel x lives in bits [4x, 4x+3], so the leftmost pixel is the low nibble.
// Palette indices: 0 transparent, 1 outline, 2 spine rings, 3 body, 4 body shade/spine, 5 screen, 6 page tab + A/B.
// Recolor per theme by swapping the palette only; the tiles are shared. Single frame.
#ifndef ADDRBOY_LOGO_BOOKBOY_H
#define ADDRBOY_LOGO_BOOKBOY_H

#include "bn_tile.h"
#include "bn_color.h"
#include "bn_span.h"
#include "bn_sprite_item.h"
#include "bn_sprite_tiles_item.h"
#include "bn_sprite_palette_item.h"
#include "bn_sprite_shape_size.h"

namespace adb::logo
{
    // 16x16: 4 tiles (TL, TR, BL, BR)
    alignas(4) constexpr bn::tile tiles_16[4] = {
        bn::tile{ { 0x00000000, 0x11111100, 0x11111410, 0x55511210, 0x11511410, 0x11511210, 0x55511410, 0x11511210 } },
        bn::tile{ { 0x00000000, 0x00011111, 0x00111111, 0x01115555, 0x16115115, 0x16115115, 0x16115551, 0x01115155 } },
        bn::tile{ { 0x11511410, 0x55511210, 0x11111410, 0x31331210, 0x11131410, 0x31331210, 0x44441410, 0x11111100 } },
        bn::tile{ { 0x00115515, 0x00115555, 0x00111111, 0x00136333, 0x00133633, 0x00133333, 0x00144444, 0x00011111 } }
    };

    // 32x32 pre-scaled: 16 tiles, row-major. Used on Welcome.
    alignas(4) constexpr bn::tile tiles_32[16] = {
        bn::tile{ { 0x00000000, 0x00000000, 0x11110000, 0x11110000, 0x11441100, 0x11441100, 0x11221100, 0x11221100 } },
        bn::tile{ { 0x00000000, 0x00000000, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x55555511, 0x55555511 } },
        bn::tile{ { 0x00000000, 0x00000000, 0x11111111, 0x11111111, 0x11111111, 0x11111111, 0x55555555, 0x55555555 } },
        bn::tile{ { 0x00000000, 0x00000000, 0x00000011, 0x00000011, 0x00001111, 0x00001111, 0x00111111, 0x00111111 } },
        bn::tile{ { 0x11441100, 0x11441100, 0x11221100, 0x11221100, 0x11441100, 0x11441100, 0x11221100, 0x11221100 } },
        bn::tile{ { 0x11115511, 0x11115511, 0x11115511, 0x11115511, 0x55555511, 0x55555511, 0x11115511, 0x11115511 } },
        bn::tile{ { 0x55111155, 0x55111155, 0x55111155, 0x55111155, 0x55555511, 0x55555511, 0x55115555, 0x55115555 } },
        bn::tile{ { 0x11661111, 0x11661111, 0x11661111, 0x11661111, 0x11661111, 0x11661111, 0x00111111, 0x00111111 } },
        bn::tile{ { 0x11441100, 0x11441100, 0x11221100, 0x11221100, 0x11441100, 0x11441100, 0x11221100, 0x11221100 } },
        bn::tile{ { 0x11115511, 0x11115511, 0x55555511, 0x55555511, 0x11111111, 0x11111111, 0x33113333, 0x33113333 } },
        bn::tile{ { 0x55551155, 0x55551155, 0x55555555, 0x55555555, 0x11111111, 0x11111111, 0x66333333, 0x66333333 } },
        bn::tile{ { 0x00001111, 0x00001111, 0x00001111, 0x00001111, 0x00001111, 0x00001111, 0x00001133, 0x00001133 } },
        bn::tile{ { 0x11441100, 0x11441100, 0x11221100, 0x11221100, 0x11441100, 0x11441100, 0x11110000, 0x11110000 } },
        bn::tile{ { 0x11111133, 0x11111133, 0x33113333, 0x33113333, 0x44444444, 0x44444444, 0x11111111, 0x11111111 } },
        bn::tile{ { 0x33663333, 0x33663333, 0x33333333, 0x33333333, 0x44444444, 0x44444444, 0x11111111, 0x11111111 } },
        bn::tile{ { 0x00001133, 0x00001133, 0x00001133, 0x00001133, 0x00001144, 0x00001144, 0x00000011, 0x00000011 } }
    };

    constexpr bn::color palette_field[16] = {
        bn::color(31, 0, 31), bn::color(8, 9, 11), bn::color(25, 26, 27), bn::color(27, 7, 5), bn::color(19, 4, 2), bn::color(26, 29, 31), bn::color(31, 25, 7), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0)
    };
    constexpr bn::color palette_net[16] = {
        bn::color(31, 0, 31), bn::color(0, 3, 6), bn::color(15, 23, 29), bn::color(1, 10, 20), bn::color(1, 7, 15), bn::color(7, 27, 31), bn::color(15, 31, 11), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0)
    };
    constexpr bn::color palette_lcd[16] = {
        bn::color(31, 0, 31), bn::color(3, 6, 4), bn::color(21, 25, 18), bn::color(10, 15, 10), bn::color(3, 6, 4), bn::color(21, 25, 18), bn::color(29, 30, 26), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0)
    };
    constexpr bn::color palette_amber[16] = {
        bn::color(31, 0, 31), bn::color(6, 4, 1), bn::color(20, 15, 5), bn::color(16, 12, 4), bn::color(6, 4, 1), bn::color(31, 22, 6), bn::color(31, 22, 6), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0), bn::color(0, 0, 0)
    };

    constexpr bn::sprite_tiles_item tiles_item_16(bn::span<const bn::tile>(tiles_16), bn::bpp_mode::BPP_4, 1);
    constexpr bn::sprite_tiles_item tiles_item_32(bn::span<const bn::tile>(tiles_32), bn::bpp_mode::BPP_4, 1);

    // Usage:
    //   bn::sprite_palette_item pal(bn::span<const bn::color>(palette_field), bn::bpp_mode::BPP_4);
    //   bn::sprite_item item(bn::sprite_shape_size(bn::sprite_shape::SQUARE, bn::sprite_size::NORMAL), tiles_item_16, pal);
    //   bn::sprite_ptr logo = item.create_sprite(x - 120 + 8, y - 80 + 8);   // top-left (x,y) -> Butano centre
    // For the 32x32 version use bn::sprite_size::BIG and tiles_item_32, with centre offset +16.
}

#endif
