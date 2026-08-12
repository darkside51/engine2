#include "colors.h"

#include <algorithm>
#include <charconv>

namespace map {

constexpr uint32_t kDefaultColor = 0xFF'FF'FFu;

struct ColorEntry {
    std::string_view name;
    uint32_t hex;
};

// static constexpr ColorEntry cssColors[] = {
//     {"aliceblue",               0xFFF8F0},
//     {"antiquewhite",            0xD7EBFA},
//     {"aqua",                    0xFFFF00},
//     {"aquamarine",              0xD4FF7F},
//     {"azure",                   0xFFFFF0},
//     {"beige",                   0xDCF5F5},
//     {"bisque",                  0xC4E4FF},
//     {"black",                   0x000000},
//     {"blanchedalmond",          0xCDEBFF},
//     {"blue",                    0xFF0000},
//     {"brick",                   0x0F34B0},
//     {"brown",                   0x2A2AA5},
//     {"burlywood",               0x87B8DE},
//     {"cadetblue",               0xA09E5F},
//     {"chartreuse",              0x00FF7F},
//     {"chocolate",               0x1E69D2},
//     {"coral",                   0x507FFF},
//     {"cornflowerblue",          0xED9564},
//     {"cornsilk",                0xDCF8FF},
//     {"crimson",                 0x3C14DC},
//     {"cyan",                    0xFFFF00},
//     {"darkblue",                0x8B0000},
//     {"dark_blue",               0x8B0000},
//     {"darkcyan",                0x8B8B00},
//     {"dark_cyan",               0x8B8B00},
//     {"darkgoldenrod",           0x0B86B8},
//     {"dark_goldenrod",          0x0B86B8},
//     {"darkgray",                0xA9A9A9},
//     {"dark_gray",               0xA9A9A9},
//     {"darkgreen",               0x006400},
//     {"dark_green",              0x006400},
//     {"darkkhaki",               0x6BB7BD},
//     {"dark_khaki",              0x6BB7BD},
//     {"darkmagenta",             0x8B008B},
//     {"dark_magenta",            0x8B008B},
//     {"darkolivegreen",          0x2F6B55},
//     {"dark_olivegreen",         0x2F6B55},
//     {"darkorange",              0x008CFF},
//     {"dark_orange",             0x008CFF},
//     {"darkorchid",              0xCC3299},
//     {"dark_orchid",             0xCC3299},
//     {"darkred",                 0x00008B},
//     {"dark_red",                0x00008B},
//     {"darksalmon",              0x7A96E9},
//     {"dark_salmon",             0x7A96E9},
//     {"darkseagreen",            0x8FBC8F},
//     {"dark_seagreen",           0x8FBC8F},
//     {"darkslateblue",           0x8B3D48},
//     {"dark_slateblue",          0x8B3D48},
//     {"darkslategray",           0x4F4F2F},
//     {"dark_slategray",          0x4F4F2F},
//     {"darkturquoise",           0xD1CE00},
//     {"dark_turquoise",          0xD1CE00},
//     {"darkviolet",              0xD30094},
//     {"dark_violet",             0xD30094},
//     {"deeppink",                0x9314FF},
//     {"deepskyblue",             0xFFBF00},
//     {"dimgray",                 0x696969},
//     {"dodgerblue",              0xFF901E},
//     {"drown",                   0x2A2AA5},
//     {"firebrick",               0x2222B2},
//     {"floralwhite",             0xF0FAFF},
//     {"forestgreen",             0x228B22},
//     {"fuchsia",                 0xFF00FF},
//     {"gainsboro",               0xDCDCDC},
//     {"ghostwhite",              0xFFF8F8},
//     {"gold",                    0x00D7FF},
//     {"goldenrod",               0x20A5DA},
//     {"gray",                    0x808080},
//     {"grey",                    0x808080},
//     {"green",                   0x008000},
//     {"greenyellow",             0x2FFFAD},
//     {"honeydew",                0xF0FFF0},
//     {"hotpink",                 0xB469FF},
//     {"indianred",               0x5C5CCD},
//     {"indigo",                  0x82004B},
//     {"ivory",                   0xF0FFFF},
//     {"khaki",                   0x8CE6F0},
//     {"lavender",                0xFAE6E6},
//     {"lavenderblush",           0xF5F0FF},
//     {"lawngreen",               0x00FC7C},
//     {"lemonchiffon",            0xCDFAFF},
//     {"lightblue",               0xE6D8AD},
//     {"lightcoral",              0x8080F0},
//     {"lightcyan",               0xFFFFE0},
//     {"lightgoldenrodyellow",    0xDDFADA},
//     {"lightgray",               0xD3D3D3},
//     {"lightgrey",               0xD3D3D3},
//     {"lightgreen",              0x90EE90},
//     {"lightpink",               0xC1B6FF},
//     {"lightsalmon",             0x7AA0FF},
//     {"lightseagreen",           0xAAB220},
//     {"lightskyblue",            0xFA87CE},
//     {"lightslategray",          0x998877},
//     {"lightsteel_blue",         0xDEC4B0},
//     {"lightyellow",             0xE0FFFF},
//     {"lime",                    0x00FF00},
//     {"limegreen",               0x32CD32},
//     {"linen",                   0xE6F0FA},
//     {"magenta",                 0xFF00FF},
//     {"maroon",                  0x000080},
//     {"mediumaquamarine",        0xAACD66},
//     {"mediumblue",              0xCD0000},
//     {"mediumorchid",            0xD355BA},
//     {"mediumpurple",            0xDB7093},
//     {"mediumseagreen",          0x71B33C},
//     {"mediumslateblue",         0xEE687B},
//     {"mediumspringgreen",       0x9AFA00},
//     {"mediumturquoise",         0xCCD148},
//     {"mediumvioletred",         0x8515C7},
//     {"midnightblue",            0x701919},
//     {"mintcream",               0xFAFFF5},
//     {"mistyrose",               0xE1E4FF},
//     {"moccasin",                0xB5E4FF},
//     {"navajowhite",             0xADDEFF},
//     {"navy",                    0x800000},
//     {"oldlace",                 0xE6F5FD},
//     {"olive",                   0x008080},
//     {"olivedrab",               0x238E6B},
//     {"orange",                  0x00A5FF},
//     {"orangered",               0x0045FF},
//     {"orchid",                  0xD670DA},
//     {"palegoldenrod",           0xAAE8EE},
//     {"palegreen",               0x98FB98},
//     {"paleturquoise",           0xEEEEAF},
//     {"palevioletred",           0x9370DB},
//     {"papayawhip",              0xDAEFF9},
//     {"peachpuff",               0xB9DAFF},
//     {"peru",                    0x3F85CD},
//     {"pink",                    0xCBC0FF},
//     {"plum",                    0xDDA0DD},
//     {"powderblue",              0xE6E0B0},
//     {"purple",                  0x800080},
//     {"rebeccapurple",           0x993366},
//     {"red",                     0x0000FF},
//     {"rosybrown",               0x8F8FBC},
//     {"royalblue",               0xE16941},
//     {"saddlebrown",             0x13458B},
//     {"salmon",                  0x7280FA},
//     {"sandybrown",              0x60A4F4},
//     {"seagreen",                0x578B2E},
//     {"seashell",                0xEEF5FF},
//     {"sienna",                  0x2D52A0},
//     {"silver",                  0xC0C0C0},
//     {"skyblue",                 0xEBCE87},
//     {"slateblue",               0xCD5A6A},
//     {"slategray",               0x908070},
//     {"snow",                    0xFAFAFF},
//     {"springgreen",             0x7FFF00},
//     {"steelblue",               0xB48246},
//     {"tan",                     0x8CB4D2},
//     {"teal",                    0x808000},
//     {"thistle",                 0xD8BFD8},
//     {"tomato",                  0x4763FF},
//     {"turquoise",               0xD0E040},
//     {"violet",                  0xEE82EE},
//     {"wheat",                   0xB3DEF5},
//     {"white",                   0xFFFFFF},
//     {"whitesmoke",              0xF5F5F5},
//     {"yellow",                  0x00FFFF},
//     {"yellowgreen",             0x32CD9A}
// };

static constexpr ColorEntry cssColors[] = {
    {"aliceblue",               0xD9AF80},
    {"antiquewhite",            0x6CA7D4},
    {"aqua",                    0xD9D941},
    {"aquamarine",              0xD4FF7F},
    {"azure",                   0xD9D980},
    {"beige",                   0x71CFCF},
    {"bisque",                  0x5B9FD9},
    {"black",                   0x000000},
    {"blanchedalmond",          0x62A9D9},
    {"blue",                    0xD94141},
    {"brick",                   0x4360BF},
    {"brown",                   0x4343BF},
    {"burlywood",               0x4D8DBF},
    {"cadetblue",               0xA09E5F},
    {"chartreuse",              0x41D98D},
    {"chocolate",               0x4377BF},
    {"coral",                   0x507FFF},
    {"cornflowerblue",          0xED9564},
    {"cornsilk",                0x6FC4D9},
    {"crimson",                 0x5C43BF},
    {"cyan",                    0xD9D941},
    {"darkblue",                0xBF3939},
    {"dark_blue",               0xBF3939},
    {"darkcyan",                0xBFBF39},
    {"dark_cyan",               0xBFBF39},
    {"darkgoldenrod",           0x439BBF},
    {"dark_goldenrod",          0x439BBF},
    {"darkgray",                0xA9A9A9},
    {"dark_gray",               0xA9A9A9},
    {"darkgreen",               0x39BF39},
    {"dark_green",              0x39BF39},
    {"darkkhaki",               0x6BB7BD},
    {"dark_khaki",              0x6BB7BD},
    {"darkmagenta",             0xBF39BF},
    {"dark_magenta",            0xBF39BF},
    {"darkolivegreen",          0x2F6B55},
    {"dark_olivegreen",         0x2F6B55},
    {"darkorange",              0x4194D9},
    {"dark_orange",             0x4194D9},
    {"darkorchid",              0xBF4396},
    {"dark_orchid",             0xBF4396},
    {"darkred",                 0x3939BF},
    {"dark_red",                0x3939BF},
    {"darksalmon",              0x7A96E9},
    {"dark_salmon",             0x7A96E9},
    {"darkseagreen",            0x4FBF4F},
    {"dark_seagreen",           0x4FBF4F},
    {"darkslateblue",           0x8B3D48},
    {"dark_slateblue",          0x8B3D48},
    {"darkslategray",           0x4F4F2F},
    {"dark_slate_gray",         0x4F4F2F},
    {"darkturquoise",           0xBFBD39},
    {"dark_turquoise",          0xBFBD39},
    {"darkviolet",              0xBF3997},
    {"dark_violet",             0xBF3997},
    {"deeppink",                0x984CD9},
    {"deepskyblue",             0xD9B341},
    {"dimgray",                 0x696969},
    {"dodgerblue",              0xD9934C},
    {"drown",                   0x4343BF},
    {"firebrick",               0x4343BF},
    {"floralwhite",             0x80BBD9},
    {"forestgreen",             0x43BF43},
    {"fuchsia",                 0xD941D9},
    {"gainsboro",               0xBF7C7C},
    {"ghostwhite",              0xD98787},
    {"gold",                    0x41C1D9},
    {"goldenrod",               0x439CBF},
    {"gray",                    0x808080},
    {"grey",                    0x808080},
    {"green",                   0x39BF39},
    {"greenyellow",             0x4CD9A1},
    {"honeydew",                0x80D980},
    {"hotpink",                 0xB469FF},
    {"indianred",               0x5C5CCD},
    {"indigo",                  0xBF3987},
    {"ivory",                   0x80D9D9},
    {"khaki",                   0x8CE6F0},
    {"lavender",                0xD47979},
    {"lavenderblush",           0x9E80D9},
    {"lawngreen",               0x40D68A},
    {"lemonchiffon",            0x62CDD9},
    {"lightblue",               0xC0A44D},
    {"lightcoral",              0x8080F0},
    {"lightcyan",               0xD9D973},
    {"lightgoldenrodyellow",    0x78D46F},
    {"lightgray",               0xBF7C7C},
    {"lightgrey",               0xBF7C7C},
    {"lightgreen",              0x50C850},
    {"lightpink",               0x6A57D9},
    {"lightsalmon",             0x7AA0FF},
    {"lightseagreen",           0xB8BF43},
    {"lightskyblue",            0xFA87CE},
    {"lightslategray",          0x998877},
    {"lightsteel_blue",         0xBF8355},
    {"lightyellow",             0x73D9D9},
    {"lime",                    0x41D941},
    {"limegreen",               0x43BF43},
    {"linen",                   0x79A6D4},
    {"magenta",                 0xD941D9},
    {"maroon",                  0x3939BF},
    {"mediumaquamarine",        0xAACD66},
    {"mediumblue",              0xBF3939},
    {"mediumorchid",            0xD355BA},
    {"mediumpurple",            0xDB7093},
    {"mediumseagreen",          0x71B33C},
    {"mediumslateblue",         0xEE687B},
    {"mediumspringgreen",       0x9BD440},
    {"mediumturquoise",         0xCCD148},
    {"mediumvioletred",         0x9143BF},
    {"midnightblue",            0xBF4343},
    {"mintcream",               0xAFD984},
    {"mistyrose",               0x737ED9},
    {"moccasin",                0x57A9D9},
    {"navajowhite",             0x57A4D9},
    {"navy",                    0xBF3939},
    {"oldlace",                 0x78B6D7},
    {"olive",                   0x39BFBF},
    {"olivedrab",               0x43BF97},
    {"orange",                  0x41A3D9},
    {"orangered",               0x416AD9},
    {"orchid",                  0xD670DA},
    {"palegoldenrod",           0x50BDC8},
    {"palegreen",               0x55D555},
    {"paleturquoise",           0xC8C850},
    {"palevioletred",           0x9370DB},
    {"papayawhip",              0x6FB2D3},
    {"peachpuff",               0x5794D9},
    {"peru",                    0x3F85CD},
    {"pink",                    0x6E57D9},
    {"plum",                    0xBF4DBF},
    {"powderblue",              0xC0B350},
    {"purple",                  0xBF39BF},
    {"rebeccapurple",           0x993366},
    {"red",                     0x4141D9},
    {"rosybrown",               0x4F4FBF},
    {"royalblue",               0xBF6243},
    {"saddlebrown",             0x4377BF},
    {"salmon",                  0x7280FA},
    {"sandybrown",              0x60A4F4},
    {"seagreen",                0x578B2E},
    {"seashell",                0x7EA4D9},
    {"sienna",                  0x436BBF},
    {"silver",                  0xBF7C7C},
    {"skyblue",                 0xEBCE87},
    {"slateblue",               0xCD5A6A},
    {"slategray",               0x908070},
    {"snow",                    0x8989D9},
    {"springgreen",             0x8DD941},
    {"steelblue",               0xB48246},
    {"tan",                     0x4D8EBF},
    {"teal",                    0xBFBF39},
    {"thistle",                 0xBF66BF},
    {"tomato",                  0x4C61D9},
    {"turquoise",               0xB3BF43},
    {"violet",                  0xEE82EE},
    {"wheat",                   0x53A4CF},
    {"white",                   0xD98D8D},
    {"whitesmoke",              0xCF8686},
    {"yellow",                  0x41D9D9},
    {"yellowgreen",             0x43BF96}
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