#include <vector>
#include <cstdlib>
#include <cstring>
#include "font_factory.h"
#include "font.h"
#include "font_petme128_8x8.h"
#include "font_petme128_16x16.h"
#include "font_petme128_24x24.h"
#include "font_petme128_32x32.h"

static const BitmapFont static_fonts[3] = {
    { font_petme128_16x16, 16, 16, 32, 96 },
    { font_petme128_24x24, 24, 24, 32, 96 },
    { font_petme128_32x32, 32, 32, 32, 96 },
};

const BitmapFont* get_scaled_petme_font(int scale)
{
    if (scale < 2 || scale > 4)
        return nullptr;
    return &static_fonts[scale - 2];
}
