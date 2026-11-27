#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Генератор текстур интерьера для interior map (cube map).

Скрипт создаёт 6 бесшовно тайлящихся PNG-текстур (по умолчанию 256x256):
  * 4 стены  — обои с разными надстройками: дверь, полки, шкаф или просто обои;
  * пол      — ламинат: доски со смещением, волокнами и швами;
  * потолок  — белая плитка (или просто белый, см. --ceiling plain).

Текстуры генерируются в нейтральных (серых) тонах, поэтому их можно
тонировать в шейдере умножением на нужный цвет. При необходимости цвет
можно "запечь" сразу через --wall-tint / --floor-tint / --ceiling-tint
(мебель тонируется отдельно через --furniture-tint).

Порядок граней кубической карты (Vulkan): +X, -X, +Y, -Y, +Z, -Z.
Именно в таком порядке движок принимает layers[0..5] (см. example.cpp),
что соответствует старым файлам resources/assets/interrior: 2, 0, 6, 5, 4, 3.

Примеры:
  python3 generate_interrior.py
  python3 generate_interrior.py --size 512 --output ./out
  python3 generate_interrior.py --wall-tint d8c8b0 --floor-tint c09060
  python3 generate_interrior.py --ceiling plain --vary-walls
  python3 generate_interrior.py --wall-variants plain,door,shelves,wardrobe
"""

import argparse
import math
import os
import random

from PIL import Image, ImageDraw

MASK32 = 0xFFFFFFFF


# ---------------------------------------------------------------------------
# Вспомогательные функции
# ---------------------------------------------------------------------------

def _hash01(x, y, seed):
    """Детерминированный хэш целых координат в [0, 1)."""
    n = (x * 0x27D4EB2D) ^ (y * 0x165667B1) ^ (seed * 0x9E3779B1)
    n &= MASK32
    n = ((n ^ (n >> 15)) * 0x85EBCA6B) & MASK32
    n = ((n ^ (n >> 13)) * 0xC2B2AE35) & MASK32
    n ^= n >> 16
    return (n & MASK32) / MASK32


def _value_noise(u, v, period_x, period_y, seed):
    """Периодический (бесшовный) value-noise с разным масштабом по осям.

    u, v       — координаты в [0, 1);
    period_x/y — количество ячеек шума по осям (целые числа);
    на границе u=1 / v=1 шум совпадает с u=0 / v=0.
    """
    x = u * period_x
    y = v * period_y

    x0 = int(math.floor(x))
    y0 = int(math.floor(y))
    fx = x - x0
    fy = y - y0

    # сглаживание (smoothstep)
    sx = fx * fx * (3.0 - 2.0 * fx)
    sy = fy * fy * (3.0 - 2.0 * fy)

    x0w = x0 % period_x
    y0w = y0 % period_y
    x1w = (x0 + 1) % period_x
    y1w = (y0 + 1) % period_y

    a = _hash01(x0w, y0w, seed)
    b = _hash01(x1w, y0w, seed)
    c = _hash01(x0w, y1w, seed)
    d = _hash01(x1w, y1w, seed)

    top = a + (b - a) * sx
    bottom = c + (d - c) * sx
    return top + (bottom - top) * sy


def _fbm(u, v, period, octaves, seed, period_y=None):
    """Фрактальный шум из нескольких октав (бесшовный)."""
    if period_y is None:
        period_y = period

    total = 0.0
    norm = 0.0
    amp = 1.0
    px = period
    py = period_y

    for octave in range(octaves):
        total += amp * _value_noise(u, v, px, py, seed + octave * 101)
        norm += amp
        amp *= 0.5
        px *= 2
        py *= 2

    return total / norm


def _smoothstep(edge0, edge1, x):
    if edge0 == edge1:
        return 0.0 if x < edge0 else 1.0
    t = (x - edge0) / (edge1 - edge0)
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


def _clamp01(value):
    return 0.0 if value < 0.0 else (1.0 if value > 1.0 else value)


# ---------------------------------------------------------------------------
# Узоры поверхностей (возвращают яркость в диапазоне ~0..1)
# ---------------------------------------------------------------------------

def _wallpaper_luminance(u, v, seed):
    """Обои: вертикальные полосы + ромбическая решётка + мотив в ромбах."""
    stripes = 0.5 + 0.5 * math.cos(2.0 * math.pi * u * 8.0)
    lum = 0.925 - 0.035 * stripes

    # тонкие акцентные линии по центрам полос
    accent = _smoothstep(0.10, 0.0, abs(math.sin(math.pi * u * 8.0)))
    lum -= 0.045 * accent

    # ромбическая решётка (две системы диагоналей)
    diag1 = abs(math.sin(math.pi * (u + v) * 8.0))
    diag2 = abs(math.sin(math.pi * (u - v) * 8.0))
    lattice = max(_smoothstep(0.10, 0.0, diag1),
                  _smoothstep(0.10, 0.0, diag2))
    lum -= 0.055 * lattice

    # небольшой цветочный мотив в центрах ромбов
    cx = ((u * 8.0 + 0.5) % 1.0) - 0.5
    cy = ((v * 8.0 + 0.5) % 1.0) - 0.5
    radius = math.hypot(cx, cy)
    angle = math.atan2(cy, cx)
    petal = 0.135 + 0.05 * math.cos(4.0 * angle)
    motif = _smoothstep(petal + 0.02, petal - 0.02, radius)
    lum -= 0.075 * motif

    # фактура бумаги
    lum += (_fbm(u, v, 16, 4, seed) - 0.5) * 0.035

    return lum


def _make_plank_layouts(size, seed):
    """Раскладка досок ламината по рядам.

    Возвращает (высота ряда, список рядов), где каждый ряд — это
    (смещение, длины досок, начала досок). Сумма длин равна size,
    поэтому текстура бесшовна по X.
    """
    rows = 8
    row_h = size // rows
    rng = random.Random(seed)

    min_len = max(16, size // 8)
    pool = sorted({max(16, size // 6), size // 5, size // 4, size // 3})
    pool = [length for length in pool if length >= min_len] or [size // 2]

    layouts = []
    for row in range(rows):
        offset = (row * (size // 2) + rng.randrange(size)) % size

        lengths = []
        total = 0
        while total < size:
            remaining = size - total
            if remaining <= min_len:
                lengths.append(remaining)
                total = size
                break

            length = min(rng.choice(pool), remaining)
            # не оставляем слишком короткий обрезок
            if remaining - length < min_len:
                length = remaining

            lengths.append(length)
            total += length

        bounds = []
        acc = 0
        for length in lengths:
            bounds.append(acc)
            acc += length

        layouts.append((offset, lengths, bounds))

    return row_h, layouts


def _laminate_luminance(x, y, size, row_h, layouts, seed):
    """Ламинат: доски, смещение по рядам, волокна и швы."""
    rows = len(layouts)
    row = (y // row_h) % rows
    offset, lengths, bounds = layouts[row]

    local = (x - offset) % size
    segment = 0
    for index, start in enumerate(bounds):
        if start <= local:
            segment = index
        else:
            break

    plank_start = bounds[segment]
    plank_len = lengths[segment]
    px = local - plank_start
    y_in_row = y % row_h

    u = x / size
    v = y / size

    base = 0.62 + 0.18 * _hash01(segment, row, seed)
    # волокна вытянуты вдоль доски (по X)
    grain = _value_noise(u, v, 4, 64, seed + 7)
    fine = _value_noise(u, v, 8, 128, seed + 13)
    rings = 0.5 + 0.5 * math.sin(y_in_row * 0.9 + grain * 8.0 + row * 2.1)

    lum = (base
           + (grain - 0.5) * 0.14
           + (fine - 0.5) * 0.05
           + (rings - 0.5) * 0.05)

    # горизонтальные швы между рядами
    seam_y = min(y_in_row, row_h - 1 - y_in_row)
    lum -= 0.18 * _smoothstep(1.6, 0.0, seam_y)

    # вертикальные швы между досками
    seam_x = min(px, plank_len - 1 - px)
    lum -= 0.15 * _smoothstep(1.6, 0.0, seam_x)

    # лёгкая фаска рядом со швами
    lum += 0.030 * _smoothstep(5.0, 1.6, seam_y)
    lum += 0.025 * _smoothstep(5.0, 1.6, seam_x)

    return lum


def _ceiling_luminance(u, v, size, seed, tiled):
    """Потолок: белая плитка (или просто белый) с лёгкой фактурой."""
    lum = 0.965 + (_fbm(u, v, 8, 3, seed) - 0.5) * 0.025

    if tiled:
        tiles = 4
        cell = size / tiles

        fx = (u * tiles) % 1.0
        fy = (v * tiles) % 1.0

        dx = min(fx, 1.0 - fx) * cell
        dy = min(fy, 1.0 - fy) * cell
        distance = min(dx, dy)

        # шов плитки и мягкая тень рядом с ним
        lum -= 0.10 * _smoothstep(1.2, 0.0, distance)
        lum -= 0.02 * _smoothstep(4.0, 1.2, distance)

    return lum


# ---------------------------------------------------------------------------
# Мебель и детали интерьера (рисуются поверх обоев)
# ---------------------------------------------------------------------------

def _lum_color(lum, tint, alpha=255):
    """Цвет RGBA из яркости и оттенка мебели."""
    lum = _clamp01(lum)
    return (
        int(round(_clamp01(lum * tint[0]) * 255.0)),
        int(round(_clamp01(lum * tint[1]) * 255.0)),
        int(round(_clamp01(lum * tint[2]) * 255.0)),
        int(alpha),
    )


def _draw_baseboard(draw, size, tint):
    """Плинтус вдоль низа стены (низ изображения = пол)."""
    height = max(3, size // 32)
    draw.rectangle([0, size - height, size - 1, size - 1],
                   fill=_lum_color(0.90, tint))
    draw.line([0, size - height, size - 1, size - height],
              fill=_lum_color(0.68, tint, 150), width=1)


def _draw_door(draw, size, seed, tint):
    """Дверь: наличник, полотно с филёнками и ручкой."""
    width = int(0.26 * size)
    height = int(0.84 * size)
    x0 = (size - width) // 2
    x1 = x0 + width
    y1 = size - 1
    y0 = y1 - height
    casing = max(3, size // 48)

    # мягкая тень от наличника
    draw.rectangle([x0 - casing, y0 - casing, x1 + casing, y1],
                   fill=_lum_color(0.30, tint, 45))
    # наличник
    draw.rounded_rectangle([x0 - casing, y0 - casing, x1 + casing, y1],
                           radius=casing, fill=_lum_color(0.92, tint))
    # полотно
    draw.rectangle([x0, y0, x1, y1], fill=_lum_color(0.74, tint))

    # вертикальные волокна древесины
    rng = random.Random(seed)
    for _ in range(max(4, width // 12)):
        grain_x = rng.randint(x0 + 2, x1 - 2)
        draw.line([grain_x, y0 + 2, grain_x, y1 - 2],
                  fill=_lum_color(0.66, tint, 70), width=1)

    # две филёнки
    margin = max(4, width // 8)
    radius = max(2, size // 128)

    def panel(px0, py0, px1, py1):
        draw.rounded_rectangle([px0, py0, px1, py1],
                               radius=radius, fill=_lum_color(0.58, tint))
        draw.rounded_rectangle([px0 + 2, py0 + 2, px1 - 2, py1 - 2],
                               radius=radius, fill=_lum_color(0.78, tint))

    middle = (y0 + y1) // 2
    panel(x0 + margin, y0 + margin, x1 - margin, middle - margin // 2)
    panel(x0 + margin, middle + margin // 2, x1 - margin, y1 - margin)

    # ручка
    handle_x = x1 - margin - max(3, width // 10)
    handle_y = y0 + int(height * 0.52)
    handle_r = max(2, size // 72)
    draw.ellipse([handle_x - handle_r, handle_y - handle_r,
                  handle_x + handle_r, handle_y + handle_r],
                 fill=_lum_color(0.30, tint))
    # тень под дверью
    draw.line([x0, y1, x1, y1], fill=_lum_color(0.22, tint, 170), width=1)


def _draw_shelves(draw, size, seed, tint):
    """Навесные полки с книгами."""
    width = int(0.62 * size)
    height = int(0.44 * size)
    x0 = (size - width) // 2
    y0 = int(size * 0.28)
    x1 = x0 + width
    y1 = y0 + height
    board = max(2, size // 72)
    side = max(3, size // 56)

    # тень под полками
    draw.rectangle([x0 + 3, y1, x1 + 3, y1 + max(2, size // 64)],
                   fill=_lum_color(0.30, tint, 45))
    # задняя стенка
    draw.rectangle([x0, y0, x1, y1], fill=_lum_color(0.64, tint))
    # боковины
    draw.rectangle([x0, y0, x0 + side, y1], fill=_lum_color(0.85, tint))
    draw.rectangle([x1 - side, y0, x1, y1], fill=_lum_color(0.85, tint))

    # горизонтальные полки
    shelf_count = 4
    for index in range(shelf_count + 1):
        shelf_y = y0 + (y1 - y0) * index // shelf_count
        draw.rectangle([x0, shelf_y, x1, shelf_y + board],
                       fill=_lum_color(0.88, tint))
        draw.line([x0, shelf_y + board, x1, shelf_y + board],
                  fill=_lum_color(0.52, tint, 130), width=1)

    # книги на полках
    rng = random.Random(seed)
    inner_x0 = x0 + side + 2
    inner_x1 = x1 - side - 2

    for index in range(shelf_count):
        cell_top = y0 + (y1 - y0) * index // shelf_count + board + 1
        cell_bottom = y0 + (y1 - y0) * (index + 1) // shelf_count - 1
        cell_height = cell_bottom - cell_top
        if cell_height <= 3:
            continue

        book_x = inner_x0
        while book_x < inner_x1 - 4:
            book_w = rng.randint(max(3, size // 64), max(5, size // 28))
            book_h = rng.randint(int(cell_height * 0.55),
                                 int(cell_height * 0.95))
            if book_x + book_w > inner_x1:
                break

            lum = 0.42 + 0.32 * rng.random()
            draw.rectangle([book_x, cell_bottom - book_h,
                            book_x + book_w - 1, cell_bottom],
                           fill=_lum_color(lum, tint))
            draw.line([book_x, cell_bottom - book_h, book_x, cell_bottom],
                      fill=_lum_color(lum * 0.75, tint, 150), width=1)
            book_x += book_w + rng.randint(1, 3)


def _draw_wardrobe(draw, size, seed, tint):
    """Шкаф: корпус, карниз, цоколь, две створки и ручки."""
    width = int(0.46 * size)
    height = int(0.80 * size)
    x0 = (size - width) // 2
    x1 = x0 + width
    y1 = size - 1
    y0 = y1 - height
    edge = max(2, size // 96)

    # тонкая контактная тень у правого торца (без "полупрозрачной стенки")
    shadow = max(1, size // 128)
    draw.rectangle([x1, y0 + shadow, x1 + shadow, y1],
                   fill=_lum_color(0.28, tint, 50))
    # корпус
    draw.rectangle([x0, y0, x1, y1], fill=_lum_color(0.80, tint))

    # карниз
    cornice = max(3, size // 48)
    draw.rectangle([x0 - edge, y0, x1 + edge, y0 + cornice],
                   fill=_lum_color(0.89, tint))
    # цоколь
    plinth = max(3, size // 48)
    draw.rectangle([x0, y1 - plinth, x1, y1], fill=_lum_color(0.56, tint))

    # створки
    middle = (x0 + x1) // 2
    inset = max(3, size // 64)
    door_top = y0 + cornice + inset
    door_bottom = y1 - plinth - inset

    for door_x0, door_x1 in ((x0 + inset, middle - 1),
                             (middle + 1, x1 - inset)):
        draw.rectangle([door_x0, door_top, door_x1, door_bottom],
                       fill=_lum_color(0.76, tint))
        draw.rectangle([door_x0, door_top, door_x1, door_bottom],
                       outline=_lum_color(0.60, tint), width=1)

        # вертикальные волокна
        door_rng = random.Random(seed + door_x0)
        for _ in range(3):
            grain_x = door_rng.randint(door_x0 + 2,
                                       max(door_x0 + 2, door_x1 - 2))
            draw.line([grain_x, door_top + 2, grain_x, door_bottom - 2],
                      fill=_lum_color(0.68, tint, 80), width=1)

    # щель между створками
    draw.line([middle, y0 + cornice, middle, y1 - plinth],
              fill=_lum_color(0.38, tint), width=1)

    # ручки
    handle_r = max(2, size // 80)
    handle_y = (door_top + door_bottom) // 2
    handle_offset = max(4, size // 32)
    for handle_x in (middle - handle_offset, middle + handle_offset):
        draw.rectangle([handle_x - handle_r, handle_y - handle_r * 2,
                        handle_x + handle_r, handle_y + handle_r * 2],
                       fill=_lum_color(0.32, tint))


# ---------------------------------------------------------------------------
# Рендер граней
# ---------------------------------------------------------------------------

def _apply_tint(lum, tint):
    lum = _clamp01(lum)
    return (
        int(round(_clamp01(lum * tint[0]) * 255.0)),
        int(round(_clamp01(lum * tint[1]) * 255.0)),
        int(round(_clamp01(lum * tint[2]) * 255.0)),
    )


def render_wall(size, seed, tint, variant="plain",
                furniture_tint=(1.0, 1.0, 1.0)):
    buffer = bytearray(size * size * 3)
    inv = 1.0 / size
    index = 0

    for y in range(size):
        v = y * inv
        for x in range(size):
            u = x * inv
            lum = _wallpaper_luminance(u, v, seed)
            r, g, b = _apply_tint(lum, tint)
            buffer[index] = r
            buffer[index + 1] = g
            buffer[index + 2] = b
            index += 3

    base = Image.frombytes("RGB", (size, size), bytes(buffer))

    # мебель и плинтус рисуем поверх обоев на отдельном слое,
    # чтобы оттенок мебели не зависел от оттенка стен
    overlay = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    _draw_baseboard(draw, size, furniture_tint)
    if variant == "door":
        _draw_door(draw, size, seed, furniture_tint)
    elif variant == "shelves":
        _draw_shelves(draw, size, seed, furniture_tint)
    elif variant == "wardrobe":
        _draw_wardrobe(draw, size, seed, furniture_tint)

    return Image.alpha_composite(base.convert("RGBA"), overlay).convert("RGB")


def render_floor(size, seed, tint):
    row_h, layouts = _make_plank_layouts(size, seed)
    buffer = bytearray(size * size * 3)
    index = 0

    for y in range(size):
        for x in range(size):
            lum = _laminate_luminance(x, y, size, row_h, layouts, seed)
            r, g, b = _apply_tint(lum, tint)
            buffer[index] = r
            buffer[index + 1] = g
            buffer[index + 2] = b
            index += 3

    return Image.frombytes("RGB", (size, size), bytes(buffer))


def render_ceiling(size, seed, tint, tiled):
    buffer = bytearray(size * size * 3)
    inv = 1.0 / size
    index = 0

    for y in range(size):
        v = y * inv
        for x in range(size):
            u = x * inv
            lum = _ceiling_luminance(u, v, size, seed, tiled)
            r, g, b = _apply_tint(lum, tint)
            buffer[index] = r
            buffer[index + 1] = g
            buffer[index + 2] = b
            index += 3

    return Image.frombytes("RGB", (size, size), bytes(buffer))


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _parse_tint(value):
    text = value.lstrip("#")
    if len(text) != 6:
        raise argparse.ArgumentTypeError(
            "ожидается цвет в формате RRGGBB, например d8c8b0")
    try:
        channels = [int(text[i:i + 2], 16) / 255.0 for i in (0, 2, 4)]
    except ValueError:
        raise argparse.ArgumentTypeError(
            "ожидается цвет в формате RRGGBB, например d8c8b0")
    return tuple(channels)


def _parse_wall_variants(value):
    items = [item.strip().lower() for item in value.split(",")]
    allowed = ("plain", "door", "shelves", "wardrobe")
    if len(items) != 4:
        raise argparse.ArgumentTypeError(
            "нужно ровно 4 варианта через запятую, например "
            "shelves,wardrobe,door,plain")
    for item in items:
        if item not in allowed:
            raise argparse.ArgumentTypeError(
                "неизвестный вариант стены '{}', допустимо: {}".format(
                    item, ", ".join(allowed)))
    return items


def _build_parser():
    parser = argparse.ArgumentParser(
        description="Генерация текстур интерьера (4 стены, пол, потолок) "
                    "для interior cube map.",
    )
    parser.add_argument(
        "-o", "--output", default=None,
        help="папка для результата (по умолчанию — папка самого скрипта)",
    )
    parser.add_argument(
        "-s", "--size", type=int, default=256,
        help="размер стороны текстуры в пикселях (по умолчанию 256)",
    )
    parser.add_argument(
        "--seed", type=int, default=1337,
        help="базовое зерно генератора (по умолчанию 1337)",
    )
    parser.add_argument(
        "--wall-tint", type=_parse_tint, default=(1.0, 1.0, 1.0),
        help="запечь оттенок стен, формат RRGGBB (по умолчанию белый)",
    )
    parser.add_argument(
        "--floor-tint", type=_parse_tint, default=(1.0, 1.0, 1.0),
        help="запечь оттенок пола, формат RRGGBB (по умолчанию белый)",
    )
    parser.add_argument(
        "--ceiling-tint", type=_parse_tint, default=(1.0, 1.0, 1.0),
        help="запечь оттенок потолка, формат RRGGBB (по умолчанию белый)",
    )
    parser.add_argument(
        "--ceiling", choices=("tile", "plain"), default="tile",
        help="потолок: плитка или однотонный белый (по умолчанию tile)",
    )
    parser.add_argument(
        "--vary-walls", action="store_true",
        help="сделать стены с разным смещением узора (по умолчанию одинаковые)",
    )
    parser.add_argument(
        "--wall-variants", type=_parse_wall_variants,
        default=["shelves", "wardrobe", "door", "plain"],
        help="варианты 4 стен в порядке +X, -X, +Z, -Z "
             "(plain, door, shelves, wardrobe); "
             "по умолчанию shelves,wardrobe,door,plain",
    )
    parser.add_argument(
        "--furniture-tint", type=_parse_tint, default=(1.0, 1.0, 1.0),
        help="оттенок мебели (двери, полки, шкаф), формат RRGGBB "
             "(по умолчанию белый)",
    )
    return parser


def main():
    parser = _build_parser()
    args = parser.parse_args()

    size = args.size
    if size < 64 or size % 8 != 0:
        parser.error("--size должен быть >= 64 и кратен 8")

    output_dir = args.output
    if output_dir is None:
        output_dir = os.path.dirname(os.path.abspath(__file__))
    os.makedirs(output_dir, exist_ok=True)

    tiled_ceiling = args.ceiling == "tile"

    # имя файла, тип поверхности, индекс стены
    faces = [
        ("face0_px_wall.png", "wall", 0),
        ("face1_nx_wall.png", "wall", 1),
        ("face2_py_ceiling.png", "ceiling", None),
        ("face3_ny_floor.png", "floor", None),
        ("face4_pz_wall.png", "wall", 2),
        ("face5_nz_wall.png", "wall", 3),
    ]

    for file_name, surface, wall_index in faces:
        if surface == "wall":
            seed = args.seed
            if args.vary_walls:
                seed += wall_index * 101
            variant = args.wall_variants[wall_index]
            image = render_wall(size, seed, args.wall_tint,
                                variant, args.furniture_tint)
        elif surface == "floor":
            image = render_floor(size, args.seed, args.floor_tint)
        else:
            image = render_ceiling(
                size, args.seed, args.ceiling_tint, tiled_ceiling)

        path = os.path.join(output_dir, file_name)
        image.save(path, format="PNG")
        print("written: {}".format(path))

    readme = (
        "interior cube map faces\n"
        "=======================\n\n"
        "Порядок слоёв кубической карты (Vulkan): +X, -X, +Y, -Y, +Z, -Z\n\n"
        "layer 0  (+X, стена)   face0_px_wall.png\n"
        "layer 1  (-X, стена)   face1_nx_wall.png\n"
        "layer 2  (+Y, потолок) face2_py_ceiling.png\n"
        "layer 3  (-Y, пол)     face3_ny_floor.png\n"
        "layer 4  (+Z, стена)   face4_pz_wall.png\n"
        "layer 5  (-Z, стена)   face5_nz_wall.png\n\n"
        "Варианты стен по умолчанию:\n"
        "  +X  полки\n"
        "  -X  шкаф\n"
        "  +Z  дверь\n"
        "  -Z  обычные обои\n\n"
        "В resources/assets/interrior этому порядку соответствуют файлы:\n"
        "2.jpg, 0.jpg, 6.jpg, 5.jpg, 4.jpg, 3.jpg\n"
    )
    readme_path = os.path.join(output_dir, "README.txt")
    with open(readme_path, "w", encoding="utf-8") as readme_file:
        readme_file.write(readme)
    print("written: {}".format(readme_path))

    print("\nПорядок граней для layers[0..5]: +X, -X, +Y, -Y, +Z, -Z")
    print("Соответствие старым ассетам: 2.jpg, 0.jpg, 6.jpg, 5.jpg, 4.jpg, 3.jpg")


if __name__ == "__main__":
    main()
