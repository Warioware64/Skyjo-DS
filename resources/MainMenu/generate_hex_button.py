#!/usr/bin/env python3
"""Generate an elegant Skyjo hex-themed button.

The button shape is an elongated hexagon — the same 60-deg geometry as the
background tiles, just stretched horizontally to fit a label. Rendered at
4x then downsampled with LANCZOS for vector-clean anti-aliased edges, with
a vertical face gradient + upper sheen + soft drop shadow for depth.

Output: 128x32 RGBA PNG next to this script. Designed for NDS tex4x4.

Usage:
    source env/bin/activate
    python resources/MainMenu/generate_hex_button.py --label PLAY --color teal
    python resources/MainMenu/generate_hex_button.py --label OPTIONS --color blue --pressed
"""

import argparse
import math
import os
from PIL import Image, ImageDraw, ImageFilter, ImageFont

OUT_W, OUT_H = 128, 32
SCALE = 4
W, H = OUT_W * SCALE, OUT_H * SCALE

# Skyjo card-derived button palette. Pick whichever matches the menu action.
COLORS: dict[str, tuple[int, int, int]] = {
    'teal':   ( 38, 145, 110),
    'green':  ( 60, 150,  50),
    'red':    (200,  60,  55),
    'blue':   ( 45, 130, 215),
    'yellow': (235, 195,  55),
    'grey':   (110, 115, 125),
}

EDGE_COLOR = (28, 24, 26)

# End-cap slope length. H/2 gives 45-deg caps (chunky button look). Use
# H/(2*sqrt(3)) ≈ 0.289*H for true 60-deg hex angles (subtler, more "hex").
END_CUT = int(H * 0.5)


def lerp_rgb(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def lighter(rgb, f=0.30):
    return tuple(min(255, int(c + (255 - c) * f)) for c in rgb)


def darker(rgb, f=0.70):
    return tuple(max(0, int(c * f)) for c in rgb)


def button_pts(w, h, end_cut, ax=0, ay=0):
    """Elongated-hex outline, clockwise from top-left."""
    return [
        (ax + end_cut,     ay),
        (ax + w - end_cut, ay),
        (ax + w,           ay + h / 2),
        (ax + w - end_cut, ay + h),
        (ax + end_cut,     ay + h),
        (ax,               ay + h / 2),
    ]


def make_vertical_gradient(w, h, top, bot):
    img = Image.new('RGB', (w, h))
    draw = ImageDraw.Draw(img)
    for y in range(h):
        t = y / max(h - 1, 1)
        draw.line([(0, y), (w, y)], fill=lerp_rgb(top, bot, t))
    return img


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


def render_button(label: str, base_color, pressed: bool = False) -> Image.Image:
    img = Image.new('RGBA', (W, H), (0, 0, 0, 0))

    inset = 2 * SCALE
    shadow_drop = 2 * SCALE
    press_shift = SCALE if pressed else 0

    # 1) Soft drop shadow underneath (skipped when pressed → "settled" feel).
    if not pressed:
        shadow = Image.new('RGBA', (W, H), (0, 0, 0, 0))
        sd = ImageDraw.Draw(shadow)
        sd.polygon(
            button_pts(W, H - shadow_drop - 1, END_CUT, ay=shadow_drop),
            fill=(0, 0, 0, 90),
        )
        shadow = shadow.filter(ImageFilter.GaussianBlur(radius=SCALE))
        img = Image.alpha_composite(img, shadow)

    # 2) Dark outer edge (the full hex silhouette).
    draw = ImageDraw.Draw(img)
    body_h = H - shadow_drop - 1
    draw.polygon(
        button_pts(W, body_h, END_CUT, ay=press_shift),
        fill=EDGE_COLOR + (255,),
    )

    # 3) Main face with vertical gradient, masked to the inset hex shape.
    face_color = darker(base_color, 0.85) if pressed else base_color
    grad = make_vertical_gradient(
        W, H,
        lighter(face_color, 0.32),
        darker(face_color, 0.78),
    )
    inner_pts = button_pts(
        W - 2 * inset, body_h - 2 * inset, END_CUT - inset,
        ax=inset, ay=press_shift + inset,
    )
    mask = Image.new('L', (W, H), 0)
    ImageDraw.Draw(mask).polygon(inner_pts, fill=255)
    img.paste(grad, (0, 0), mask)

    # 4) Upper-half sheen — a smaller hex covering only the top portion,
    # white at low alpha for a soft specular highlight.
    sheen_inset = inset + 2 * SCALE
    sheen_h = (body_h // 2) - sheen_inset
    sheen_alpha = 55 if pressed else 110
    sheen_layer = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(sheen_layer).polygon(
        button_pts(
            W - 2 * sheen_inset, sheen_h, END_CUT - sheen_inset,
            ax=sheen_inset, ay=press_shift + sheen_inset,
        ),
        fill=(255, 255, 255, sheen_alpha),
    )
    img = Image.alpha_composite(img, sheen_layer)

    # 5) Centered label, with a subtle drop shadow underneath the text.
    if label:
        font = load_font(int(16 * SCALE))
        text_layer = Image.new('RGBA', (W, H), (0, 0, 0, 0))
        td = ImageDraw.Draw(text_layer)
        bbox = td.textbbox((0, 0), label, font=font)
        tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
        tx = (W - tw) / 2 - bbox[0]
        ty = (body_h - th) / 2 - bbox[1] + press_shift
        td.text((tx, ty + SCALE), label, font=font, fill=(0, 0, 0, 170))
        td.text((tx, ty), label, font=font, fill=(255, 255, 255, 255))
        img = Image.alpha_composite(img, text_layer)

    return img


def main() -> None:
    parser = argparse.ArgumentParser(description='Generate a Skyjo hex button')
    parser.add_argument('--label', default='PLAY')
    parser.add_argument('--color', default='teal', choices=list(COLORS.keys()))
    parser.add_argument('--pressed', action='store_true')
    parser.add_argument('--output', default=None,
                        help='Output PNG path (default: hex_button_<color>[_pressed].png)')
    args = parser.parse_args()

    big = render_button(args.label, COLORS[args.color], pressed=args.pressed)
    out = big.resize((OUT_W, OUT_H), Image.LANCZOS)

    if args.output:
        out_path = args.output
    else:
        suffix = '_pressed' if args.pressed else ''
        out_path = os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            f'hex_button_{args.color}{suffix}.png',
        )
    out.save(out_path)
    print(f'Wrote {out_path} ({OUT_W}x{OUT_H}, SSAA {SCALE}x)')


if __name__ == '__main__':
    main()
