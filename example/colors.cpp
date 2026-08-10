#include "colors.h"

#include <algorithm>
#include <charconv>

namespace map {

constexpr uint32_t kDefaultColor = 0xff'ff'ff'ffu;

struct ColorEntry {
    std::string_view name;
    uint32_t hex;
};

static constexpr ColorEntry cssColors[] = {
    {"aliceblue",               0xFFF8F0},
    {"antiquewhite",            0xD7EBFA},
    {"aqua",                    0xFFFF00},
    {"aquamarine",              0xD4FF7F},
    {"azure",                   0xFFFFF0},
    {"beige",                   0xDCF5F5},
    {"bisque",                  0xC4E4FF},
    {"black",                   0x000000},
    {"blanchedalmond",          0xCDEBFF},
    {"blue",                    0xFF0000},
    {"brick",                   0x0F34B0},
    {"brown",                   0x2A2AA5},
    {"burlywood",               0x87B8DE},
    {"cadetblue",               0xA09E5F},
    {"chartreuse",              0x00FF7F},
    {"chocolate",               0x1E69D2},
    {"coral",                   0x507FFF},
    {"cornflowerblue",          0xED9564},
    {"cornsilk",                0xDCF8FF},
    {"crimson",                 0x3C14DC},
    {"cyan",                    0xFFFF00},
    {"darkblue",                0x8B0000},
    {"dark_blue",               0x8B0000},
    {"darkcyan",                0x8B8B00},
    {"dark_cyan",               0x8B8B00},
    {"darkgoldenrod",           0x0B86B8},
    {"dark_goldenrod",          0x0B86B8},
    {"darkgray",                0xA9A9A9},
    {"dark_gray",               0xA9A9A9},
    {"darkgreen",               0x006400},
    {"dark_green",              0x006400},
    {"darkkhaki",               0x6BB7BD},
    {"dark_khaki",              0x6BB7BD},
    {"darkmagenta",             0x8B008B},
    {"dark_magenta",            0x8B008B},
    {"darkolivegreen",          0x2F6B55},
    {"dark_olivegreen",         0x2F6B55},
    {"darkorange",              0x008CFF},
    {"dark_orange",             0x008CFF},
    {"darkorchid",              0xCC3299},
    {"dark_orchid",             0xCC3299},
    {"darkred",                 0x00008B},
    {"dark_red",                0x00008B},
    {"darksalmon",              0x7A96E9},
    {"dark_salmon",             0x7A96E9},
    {"darkseagreen",            0x8FBC8F},
    {"dark_seagreen",           0x8FBC8F},
    {"darkslateblue",           0x8B3D48},
    {"dark_slateblue",          0x8B3D48},
    {"darkslategray",           0x4F4F2F},
    {"dark_slategray",          0x4F4F2F},
    {"darkturquoise",           0xD1CE00},
    {"dark_turquoise",          0xD1CE00},
    {"darkviolet",              0xD30094},
    {"dark_violet",             0xD30094},
    {"deeppink",                0x9314FF},
    {"deepskyblue",             0xFFBF00},
    {"dimgray",                 0x696969},
    {"dodgerblue",              0xFF901E},
    {"drown",                   0x2A2AA5},
    {"firebrick",               0x2222B2},
    {"floralwhite",             0xF0FAFF},
    {"forestgreen",             0x228B22},
    {"fuchsia",                 0xFF00FF},
    {"gainsboro",               0xDCDCDC},
    {"ghostwhite",              0xFFF8F8},
    {"gold",                    0x00D7FF},
    {"goldenrod",               0x20A5DA},
    {"gray",                    0x808080},
    {"grey",                    0x808080},
    {"green",                   0x008000},
    {"greenyellow",             0x2FFFAD},
    {"honeydew",                0xF0FFF0},
    {"hotpink",                 0xB469FF},
    {"indianred",               0x5C5CCD},
    {"indigo",                  0x82004B},
    {"ivory",                   0xF0FFFF},
    {"khaki",                   0x8CE6F0},
    {"lavender",                0xFAE6E6},
    {"lavenderblush",           0xF5F0FF},
    {"lawngreen",               0x00FC7C},
    {"lemonchiffon",            0xCDFAFF},
    {"lightblue",               0xE6D8AD},
    {"lightcoral",              0x8080F0},
    {"lightcyan",               0xFFFFE0},
    {"lightgoldenrodyellow",    0xDDFADA},
    {"lightgray",               0xD3D3D3},
    {"lightgrey",               0xD3D3D3},
    {"lightgreen",              0x90EE90},
    {"lightpink",               0xC1B6FF},
    {"lightsalmon",             0x7AA0FF},
    {"lightseagreen",           0xAAB220},
    {"lightskyblue",            0xFA87CE},
    {"lightslategray",          0x998877},
    {"lightsteel_blue",         0xDEC4B0},
    {"lightyellow",             0xE0FFFF},
    {"lime",                    0x00FF00},
    {"limegreen",               0x32CD32},
    {"linen",                   0xE6F0FA},
    {"magenta",                 0xFF00FF},
    {"maroon",                  0x000080},
    {"mediumaquamarine",        0xAACD66},
    {"mediumblue",              0xCD0000},
    {"mediumorchid",            0xD355BA},
    {"mediumpurple",            0xDB7093},
    {"mediumseagreen",          0x71B33C},
    {"mediumslateblue",         0xEE687B},
    {"mediumspringgreen",       0x9AFA00},
    {"mediumturquoise",         0xCCD148},
    {"mediumvioletred",         0x8515C7},
    {"midnightblue",            0x701919},
    {"mintcream",               0xFAFFF5},
    {"mistyrose",               0xE1E4FF},
    {"moccasin",                0xB5E4FF},
    {"navajowhite",             0xADDEFF},
    {"navy",                    0x800000},
    {"oldlace",                 0xE6F5FD},
    {"olive",                   0x008080},
    {"olivedrab",               0x238E6B},
    {"orange",                  0x00A5FF},
    {"orangered",               0x0045FF},
    {"orchid",                  0xD670DA},
    {"palegoldenrod",           0xAAE8EE},
    {"palegreen",               0x98FB98},
    {"paleturquoise",           0xEEEEAF},
    {"palevioletred",           0x9370DB},
    {"papayawhip",              0xDAEFF9},
    {"peachpuff",               0xB9DAFF},
    {"peru",                    0x3F85CD},
    {"pink",                    0xCBC0FF},
    {"plum",                    0xDDA0DD},
    {"powderblue",              0xE6E0B0},
    {"purple",                  0x800080},
    {"rebeccapurple",           0x993366},
    {"red",                     0x0000FF},
    {"rosybrown",               0x8F8FBC},
    {"royalblue",               0xE16941},
    {"saddlebrown",             0x13458B},
    {"salmon",                  0x7280FA},
    {"sandybrown",              0x60A4F4},
    {"seagreen",                0x578B2E},
    {"seashell",                0xEEF5FF},
    {"sienna",                  0x2D52A0},
    {"silver",                  0xC0C0C0},
    {"skyblue",                 0xEBCE87},
    {"slateblue",               0xCD5A6A},
    {"slategray",               0x908070},
    {"snow",                    0xFAFAFF},
    {"springgreen",             0x7FFF00},
    {"steelblue",               0xB48246},
    {"tan",                     0x8CB4D2},
    {"teal",                    0x808000},
    {"thistle",                 0xD8BFD8},
    {"tomato",                  0x4763FF},
    {"turquoise",               0xD0E040},
    {"violet",                  0xEE82EE},
    {"wheat",                   0xB3DEF5},
    {"white",                   0xFFFFFF},
    {"whitesmoke",              0xF5F5F5},
    {"yellow",                  0x00FFFF},
    {"yellowgreen",             0x32CD9A}
};

constexpr char toLowerChar(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

constexpr bool compareStrings(std::string_view a, std::string_view b) noexcept {
    const size_t n = std::min(a.size(), b.size());
    for (size_t i = 0u; i < n; ++i) {
        char ca = toLowerChar(a[i]);
        char cb = toLowerChar(b[i]);
        if (ca != cb) return false;
    }
    return a.size() == b.size();
}

ColorResult colorNameToUint32t(std::string_view name) noexcept {
    if (name.empty()) {
        return {
            kDefaultColor,
            false
        };
    }

    const auto it = std::find_if(
        std::begin(cssColors),
        std::end(cssColors),
        [name] (const ColorEntry & color) noexcept {
            return compareStrings(color.name, name);
        }
    );

    if (it == std::end(cssColors)) {
        return {
            kDefaultColor,
            false
        };
    }

    return {
            it->hex,
            true
        };
}

ColorResult hexStringToUint32(std::string_view sv) noexcept {
    // check prefix
    if (!sv.empty() && sv.front() == '#') {
        sv.remove_prefix(1);
    }

    if (sv.size() != 6u) {
        return {
            kDefaultColor,
            false
        };
    }

    uint32_t value = 0u;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value, 16);

    if (ec != std::errc()) {
        return {
            kDefaultColor,
            false
        };
    }
    if (ptr != sv.data() + sv.size()) {
        return {
            kDefaultColor,
            false
        };
    }

    return { 
        value,
        true
    };
}

}