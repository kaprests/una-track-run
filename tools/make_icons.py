#!/usr/bin/env python3
"""Generate the app icons in Resources/: a teal runner in front of an amber track.

icon_store.png  512x512 for the app store, full colour.
icon_60x60.png  On-watch icon.
icon_30x30.png  On-watch icon with a single track line.

The watch stores icons as ABGR2222 (each channel truncated to 2 bits), so the
on-watch icons use only colours on that grid and alpha levels of 0/85/170/255.
Run: mise run icons
"""

import os

from PIL import Image, ImageChops, ImageDraw

RES = os.path.join(os.path.dirname(__file__), "..", "Resources")

TEAL_WATCH = (0, 170, 170)
AMBER_WATCH = (255, 170, 0)
TEAL_STORE = (0, 128, 128)
AMBER_STORE = (192, 128, 0)

HEAD = ((0.635, 0.165), 0.078)
LIMB_STROKE = 0.085
LIMBS = (
    ((0.575, 0.295), (0.465, 0.545)),                     # torso
    ((0.560, 0.330), (0.705, 0.415), (0.800, 0.320)),     # front arm
    ((0.560, 0.330), (0.415, 0.390), (0.315, 0.475)),     # back arm
    ((0.465, 0.545), (0.640, 0.650), (0.600, 0.860)),     # front leg
    ((0.465, 0.545), (0.370, 0.715), (0.195, 0.735)),     # back leg
)
TRACK = (0.05, 0.50, 0.95, 0.94)   # outer stadium box


def runner(size, stroke):
    """Runner mask; `stroke` is the limb width as a fraction of the icon."""
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)
    width = stroke * size
    for limb in LIMBS:
        points = [(x * size, y * size) for x, y in limb]
        draw.line(points, fill=255, width=round(width), joint="curve")
        for x, y in points:
            draw.ellipse((x - width / 2, y - width / 2, x + width / 2, y + width / 2), fill=255)
    (cx, cy), r = HEAD
    r += (stroke - LIMB_STROKE) / 2
    draw.ellipse(((cx - r) * size, (cy - r) * size, (cx + r) * size, (cy + r) * size), fill=255)
    return mask


def track(size, lines, line, gap):
    """Stadium outlines, nested inward; widths are fractions of the icon."""
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)
    x0, y0, x1, y1 = (v * size for v in TRACK)
    for i in range(lines):
        inset = i * (line + gap) * size
        box = (x0 + inset, y0 + inset, x1 - inset, y1 - inset)
        radius = (box[3] - box[1]) / 2
        draw.rounded_rectangle(box, radius=radius, outline=255, width=round(line * size))
    return mask


def layers(size, lines, line, gap, cut):
    """Runner and track masks; the track is cut away around the runner."""
    body = runner(size, LIMB_STROKE)
    halo = runner(size, LIMB_STROKE + 2 * cut)
    return body, ImageChops.subtract(track(size, lines, line, gap), halo)


def watch_icon(px, lines, line, gap, cut):
    big = px * 8
    body, ring = (m.resize((px, px), Image.BOX) for m in layers(big, lines, line, gap, cut))
    out = Image.new("RGBA", (px, px), (0, 0, 0, 0))
    for y in range(px):
        for x in range(px):
            b, t = body.getpixel((x, y)), ring.getpixel((x, y))
            alpha = round(min(255, b + t) / 85) * 85
            if alpha:
                out.putpixel((x, y), (*(TEAL_WATCH if b >= t else AMBER_WATCH), alpha))
    return out


def store_icon(px):
    big = px * 4
    body, ring = layers(big, lines=2, line=0.035, gap=0.035, cut=0.025)
    out = Image.new("RGBA", (big, big), (0, 0, 0, 0))
    out.paste(Image.new("RGBA", (big, big), (*AMBER_STORE, 255)), mask=ring)
    out.paste(Image.new("RGBA", (big, big), (*TEAL_STORE, 255)), mask=body)
    return out.resize((px, px), Image.LANCZOS)


store_icon(512).save(os.path.join(RES, "icon_store.png"))
watch_icon(60, lines=2, line=2 / 60, gap=2 / 60, cut=1 / 60).save(os.path.join(RES, "icon_60x60.png"))
watch_icon(30, lines=1, line=2 / 30, gap=0.0, cut=1 / 30).save(os.path.join(RES, "icon_30x30.png"))
