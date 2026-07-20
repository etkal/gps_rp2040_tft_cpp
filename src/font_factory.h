#pragma once
#include "font.h"

// Get a generated scaled font based on the existing 8x8 font.
// scale = 2 -> 16x16, 3 -> 24x24, 4 -> 32x32
const BitmapFont* get_scaled_petme_font(int scale);
