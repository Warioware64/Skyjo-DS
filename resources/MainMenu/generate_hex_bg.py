#!/usr/bin/env python3
"""Generate a 256x256 Skyjo rainbow-and-hexagon background.

Design language (inspired by clean game splash backgrounds like UNO):
    - Smooth rainbow gradient through the Skyjo card palette as the base.
    - Thin white hexagonal wireframe overlay, low alpha — structure without
      noise, like a faint watermark.
    - Sparse accent hexes lightly tinted with a Skyjo card color, also at
      low alpha — gives the rainbow some "texture" without dominating.
    - Rendered at 4x then downsampled with LANCZOS for vector-clean edges.

Output: hex_background.png next to this script. Tiles seamlessly as a
256x256 NDS background. Designed for tex4x4.

Run from the project venv:
    source env/bin/activate
    python resources/MainMenu/generate_hex_bg.py
"""

import math
import os
from PIL import Image, ImageDraw

OUT_SIZE = 256
SCALE = 4
SIZE = OUT_SIZE * SCALE

# Skyjo card palette, -2 -> 12, ordered as the natural rainbow.
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

HEX_SIZE        = 26 * SCALE    # circumradius; bigger = airier wireframe
WIREFRAME_ALPHA = 70            # 0-255, opacity of the white hex wireframe
ACCENT_ALPHA    = 90            # opacity of the lightly tinted accent hexes
LINE_WIDTH      = max(1, int(0.5 * SCALE))


def lerp_rgb(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def sample_palette(t: float) -> tuple[int, int, int]:
    """Closed-loop sample across the Skyjo palette so the gradient tiles."""
    n = len(SKYJO_CARDS)
    pos = (t % 1.0) * n
    i = int(pos) % n
    j = (i + 1) % n
    return lerp_rgb(SKYJO_CARDS[i], SKYJO_CARDS[j], pos - int(pos))


def hex_corners(cx: float, cy: float, size: float):
    return [(cx + size * math.cos(math.radians(a)),
             cy + size * math.sin(math.radians(a)))
            for a in (0, 60, 120, 180, 240, 300)]


def render_gradient(canvas: Image.Image) -> None:
    """Vertical rainbow gradient base. Tiles top<->bottom because the
    palette is sampled as a closed loop (last color blends back to first)."""
    draw = ImageDraw.Draw(canvas)
    for y in range(SIZE):
        t = (y / SCALE) / OUT_SIZE
        draw.line([(0, y), (SIZE, y)], fill=sample_palette(t))


def render_wireframe() -> Image.Image:
    """Thin white hex wireframe + sparse Skyjo-tinted accent hexes."""
    overlay = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    dx = 1.5 * HEX_SIZE
    dy = math.sqrt(3) * HEX_SIZE
    half_dy = dy / 2
    cols = int(SIZE / dx) + 4
    rows = int(SIZE / dy) + 4

    for col in range(-2, cols):
        for row in range(-2, rows):
            cx = col * dx
            cy = row * dy + (half_dy if col % 2 else 0)

            # Sparse, deterministic accent placement — feels intentional
            # rather than random.
            k = (col * 7 + row * 11) % 17
            is_accent = k < 3

            if is_accent:
                palette_idx = (col * 3 + row * 5) % len(SKYJO_CARDS)
                tint = SKYJO_CARDS[palette_idx]
            else:
                tint = None

            for ox in (-SIZE, 0, SIZE):
                for oy in (-SIZE, 0, SIZE):
                    ccx, ccy = cx + ox, cy + oy
                    pts = hex_corners(ccx, ccy, HEX_SIZE - LINE_WIDTH)

                    if tint is not None:
                        draw.polygon(pts, fill=(*tint, ACCENT_ALPHA))

                    draw.polygon(
                        pts,
                        outline=(255, 255, 255, WIREFRAME_ALPHA),
                        width=LINE_WIDTH,
                    )

    return overlay


def main() -> None:
    canvas = Image.new('RGB', (SIZE, SIZE))
    render_gradient(canvas)

    overlay = render_wireframe()
    composed = Image.alpha_composite(canvas.convert('RGBA'), overlay)

    out = composed.convert('RGB').resize((OUT_SIZE, OUT_SIZE), Image.LANCZOS)
    out_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            'hex_background.png')
    out.save(out_path)
    print(f'Wrote {out_path} ({OUT_SIZE}x{OUT_SIZE}, SSAA {SCALE}x)')


if __name__ == '__main__':
    main()
