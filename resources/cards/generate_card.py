#!/usr/bin/env python3
"""Generate flat-style Skyjo number cards (-2..12) plus a face-down card back.

Each card is drawn at ~2:3 proportions (64x96) centered in a 64x128 RGBA
texture with transparent padding, so it drops straight into a power-of-2 NDS
texture slot. Rendered at 4x then downsampled with LANCZOS for clean edges.

Style: flat — solid card-color face, rounded rectangle, thin darker border,
bold centered number with small inverted corner numbers, like the physical
Skyjo cards.

Run from the project venv:
    source env/bin/activate
    python resources/cards/generate_card.py            # all 15 cards + back
    python resources/cards/generate_card.py --value 7  # a single card
    python resources/cards/generate_card.py --back     # just the card back
"""

import argparse
import math
import os
from PIL import Image, ImageDraw, ImageFont

OUT_W, OUT_H = 64, 128       # power-of-2 texture size
SCALE = 4                    # supersample factor
CARD_W, CARD_H = 64, 96      # card area, centered in the texture

MIN_VALUE, MAX_VALUE = -2, 12

# Skyjo card palette, value -2 -> 12. Copied from generate_hex_bg.py so this
# script stays self-contained; index into it with (value + 2).
SKYJO_CARDS: list[tuple[int, int, int]] = [
    ( 31,  75, 142),  # -2  dark blue
    ( 92, 172, 238),  # -1  light blue
    (232, 232, 232),  #  0  pale grey
    (152, 211, 121),  #  1  light green
    (120, 190,  90),  #  2
    ( 90, 170,  70),  #  3
    ( 60, 150,  50),  #  4  deep green
    (220, 210,  80),  #  5  yellow
    (240, 200,  50),  #  6
    (255, 180,  30),  #  7
    (255, 140,  20),  #  8  orange
    (255, 100,  50),  #  9
    (240,  70,  50),  # 10
    (200,  40,  40),  # 11
    (140,  25,  30),  # 12  dark red
]

BACK_COLOR = (40, 52, 78)    # slate-blue back, distinct from every face color
BACK_ACCENT = (236, 238, 245)


def darker(rgb, f=0.70):
    return tuple(max(0, int(c * f)) for c in rgb)


def text_color(rgb: tuple[int, int, int]) -> tuple[int, int, int]:
    """Black on light faces, white on dark ones (Rec. 601 luma)."""
    luma = 0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]
    return (28, 28, 30) if luma > 140 else (255, 255, 255)


def load_font(size: int):
    candidates = [
        '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',
        '/usr/share/fonts/dejavu/DejaVuSans-Bold.ttf',
        '/usr/share/fonts/TTF/DejaVuSans-Bold.ttf',
        '/Library/Fonts/Arial Bold.ttf',
        '/System/Library/Fonts/Helvetica.ttc',
        'C:\\Windows\\Fonts\\arialbd.ttf',
    ]
    for path in candidates:
        try:
            return ImageFont.truetype(path, size=size)
        except (OSError, IOError):
            continue
    return ImageFont.load_default()


def fit_font(text: str, max_w: float, max_h: float, start: int):
    """Largest DejaVu-Bold size whose rendered glyphs fit the given box."""
    size = max(int(start), 10)
    while size > 10:
        font = load_font(size)
        left, top, right, bottom = font.getbbox(text)
        if (right - left) <= max_w and (bottom - top) <= max_h:
            return font
        size -= 2
    return load_font(10)


def hex_corners(cx, cy, size):
    return [(cx + size * math.cos(math.radians(a)),
             cy + size * math.sin(math.radians(a)))
            for a in (0, 60, 120, 180, 240, 300)]


def card_box(W, H, cw, ch):
    """Bounding box (x0, y0, x1, y1) of the card, centered in the texture."""
    x0 = (W - cw) // 2
    y0 = (H - ch) // 2
    return x0, y0, x0 + cw, y0 + ch


def draw_centered_text(draw, cx, cy, text, font, fill):
    """Draw `text` centered on (cx, cy), accounting for glyph bbox offsets."""
    left, top, right, bottom = draw.textbbox((0, 0), text, font=font)
    draw.text((cx - (right - left) / 2 - left,
               cy - (bottom - top) / 2 - top), text, font=font, fill=fill)


def render_card(value: int, out_w=OUT_W, out_h=OUT_H, scale=SCALE,
                card_w=CARD_W, card_h=CARD_H) -> Image.Image:
    W, H = out_w * scale, out_h * scale
    cw, ch = card_w * scale, card_h * scale

    face = SKYJO_CARDS[value + 2]
    ink = text_color(face) + (255,)
    border = darker(face, 0.5) + (255,)

    img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    x0, y0, x1, y1 = card_box(W, H, cw, ch)
    radius = int(0.14 * cw)
    border_w = max(1, int(0.03 * cw))
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2

    # 1) Card face: solid rounded rectangle with a thin darker border.
    draw.rounded_rectangle([x0, y0, x1 - 1, y1 - 1], radius=radius,
                           fill=face + (255,), outline=border, width=border_w)

    label = str(value)

    # 2) Big centered number — sized down to fit two-digit / signed labels.
    big_font = fit_font(label, cw * 0.62, ch * 0.5, ch * 0.58)
    draw_centered_text(draw, cx, cy, label, big_font, ink)

    # 3) Small corner numbers: top-left upright, bottom-right rotated 180 deg
    #    (drawn upright then rotated about the centered card) — same as the
    #    physical Skyjo cards.
    small_font = fit_font(label, cw * 0.30, ch * 0.16, ch * 0.16)
    margin = int(0.11 * cw)
    sl, st, sr, sb = small_font.getbbox(label)

    corner = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(corner).text((x0 + margin - sl, y0 + margin - st),
                                label, font=small_font, fill=ink)
    img = Image.alpha_composite(img, corner)
    # Bottom-right copy: the card is centered in the texture, so a 180 deg
    # rotation about the image center maps the top-left number to the
    # symmetric bottom-right corner, upside-down.
    img = Image.alpha_composite(img, corner.rotate(180))

    return img.resize((out_w, out_h), Image.LANCZOS)


def render_back(out_w=OUT_W, out_h=OUT_H, scale=SCALE,
                card_w=CARD_W, card_h=CARD_H) -> Image.Image:
    W, H = out_w * scale, out_h * scale
    cw, ch = card_w * scale, card_h * scale

    img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    x0, y0, x1, y1 = card_box(W, H, cw, ch)
    radius = int(0.14 * cw)
    border_w = max(1, int(0.03 * cw))
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2

    # Solid slate face + outer border.
    draw.rounded_rectangle([x0, y0, x1 - 1, y1 - 1], radius=radius,
                           fill=BACK_COLOR + (255,),
                           outline=darker(BACK_COLOR, 0.5) + (255,),
                           width=border_w)

    # Inset second outline -> double-border look.
    inset = int(0.07 * cw)
    draw.rounded_rectangle(
        [x0 + inset, y0 + inset, x1 - 1 - inset, y1 - 1 - inset],
        radius=max(1, radius - inset),
        outline=BACK_ACCENT + (160,), width=max(1, border_w // 2))

    # Centered hexagon emblem, tying into the project's hex theme.
    hex_r = 0.30 * cw
    draw.polygon(hex_corners(cx, cy, hex_r),
                 outline=BACK_ACCENT + (220,), width=max(1, border_w))
    draw.polygon(hex_corners(cx, cy, hex_r * 0.55),
                 outline=BACK_ACCENT + (150,), width=max(1, border_w // 2))

    return img.resize((out_w, out_h), Image.LANCZOS)


def value_filename(value: int) -> str:
    """FAT-safe name: 'n' prefix for negatives so no minus sign in the path."""
    return f'card_n{-value}.png' if value < 0 else f'card_{value}.png'


def main() -> None:
    parser = argparse.ArgumentParser(description='Generate Skyjo card textures')
    parser.add_argument('--value', type=int, default=None,
                        help=f'render a single card ({MIN_VALUE}..{MAX_VALUE})')
    parser.add_argument('--back', action='store_true',
                        help='render only the face-down card back')
    parser.add_argument('--all', action='store_true',
                        help='render all 15 cards + back (the default)')
    parser.add_argument('--outdir', default=None,
                        help='output directory (default: ./png next to this script)')
    parser.add_argument('--out_w', type=int, default=OUT_W)
    parser.add_argument('--out_h', type=int, default=OUT_H)
    parser.add_argument('--scale', type=int, default=SCALE)
    parser.add_argument('--card_w', type=int, default=CARD_W)
    parser.add_argument('--card_h', type=int, default=CARD_H)
    args = parser.parse_args()

    if args.value is not None and not (MIN_VALUE <= args.value <= MAX_VALUE):
        parser.error(f'--value must be in {MIN_VALUE}..{MAX_VALUE}')

    outdir = args.outdir or os.path.join(
        os.path.dirname(os.path.abspath(__file__)), 'png')
    os.makedirs(outdir, exist_ok=True)

    dims = dict(out_w=args.out_w, out_h=args.out_h, scale=args.scale,
                card_w=args.card_w, card_h=args.card_h)

    def save(img, name):
        path = os.path.join(outdir, name)
        img.save(path)
        print(f'Wrote {path} ({img.width}x{img.height})')

    # No specific selection -> render the whole deck.
    render_all = args.all or (args.value is None and not args.back)

    if render_all:
        for v in range(MIN_VALUE, MAX_VALUE + 1):
            save(render_card(v, **dims), value_filename(v))
        save(render_back(**dims), 'card_back.png')
    else:
        if args.value is not None:
            save(render_card(args.value, **dims), value_filename(args.value))
        if args.back:
            save(render_back(**dims), 'card_back.png')


if __name__ == '__main__':
    main()
