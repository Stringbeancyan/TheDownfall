"""Reproducible original, seamless packed PBR variation textures. No dependencies.

R: albedo variation; G: dry roughness; B: wet surface breakup. These are authored
procedural source textures, not photographs or scan data. Regenerate explicitly.
"""
from pathlib import Path
import math
import random
import struct

SIZE = 512
ROOT = Path(__file__).resolve().parents[1] / "ArtSource/Surfaces"


def make_pixels(kind, size=SIZE):
    rng = random.Random({"Concrete": 8721, "Asphalt": 291, "Metal": 74}[kind])
    pixels = bytearray()
    for y in range(size):
        for x in range(size):
            u, v = x / size, y / size
            # Integer frequency waves tile exactly. Fine grain remains stochastic.
            grain = rng.random()
            broad = (math.sin(2 * math.pi * (3 * u + 2 * v))
                     + math.cos(2 * math.pi * (u - 5 * v))) * .5
            if kind == "Concrete":
                tone = .68 + .13 * broad + .17 * (grain - .5)
                if grain < .035:
                    tone *= .55
                roughness = .8 + .12 * (grain - .5)
            elif kind == "Asphalt":
                tone = .54 + .09 * broad + .38 * (grain - .5)
                roughness = .85 + .10 * (grain - .5)
            else:
                tone = .76 + .06 * broad + .09 * math.sin(2 * math.pi * 97 * v)
                roughness = .43 + .17 * (grain - .5)
            wet_mask = .5 + .4 * broad
            # Uncompressed TGA stores BGR. Top-left origin bit below.
            pixels.extend(round(max(0, min(1, channel)) * 255)
                          for channel in (wet_mask, roughness, tone))
    return bytes(pixels)


def main():
    ROOT.mkdir(parents=True, exist_ok=True)
    for kind in ("Concrete", "Asphalt", "Metal"):
        header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, SIZE, SIZE, 24, 32)
        path = ROOT / f"T_{kind}_Surface.tga"
        path.write_bytes(header + make_pixels(kind))
        print(f"Wrote {path.name}: {SIZE} x {SIZE}")


if __name__ == "__main__":
    main()
