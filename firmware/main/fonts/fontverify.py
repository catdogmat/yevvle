#!/usr/bin/env python3
"""
Font verification tool for Adafruit_GFX font headers.

Compares two font .h files (typically original and deduplicated) and verifies
that every glyph produces identical bitmap data in both files.

Usage:
    python3 fontverify.py original.h deduplicated.h

Exit code 0 if all glyphs match, 1 if any differ.
"""

import re
import sys


def parse_font_bitmaps_and_glyphs(filepath):
    """Parse a font .h file and return (bitmaps_bytes, glyphs_list).

    glyphs_list items are dicts with:
        bitmapOffset, width, height, xAdvance, xOffset, yOffset, comment
    """
    with open(filepath, 'r') as f:
        content = f.read()

    # Extract bitmap bytes
    bm_match = re.search(
        r'const\s+uint8_t\s+\w+Bitmaps\s*\[\]\s*PROGMEM\s*=\s*\{', content)
    if not bm_match:
        raise ValueError(f"{filepath}: Could not find Bitmaps[] array declaration")

    bm_start = content.index('{', bm_match.start()) + 1
    depth = 1
    pos = bm_start
    while depth > 0 and pos < len(content):
        if content[pos] == '{':
            depth += 1
        elif content[pos] == '}':
            depth -= 1
        pos += 1
    bm_end = pos - 1
    bm_body = content[bm_start:bm_end]
    # Extract hex byte values using regex (handles inline comments, whitespace, etc.)
    bitmaps = bytes(int(h, 16) for h in re.findall(r'0x[0-9A-Fa-f]+', bm_body))

    # Extract glyph entries
    glyph_match = re.search(
        r'const\s+GFXglyph\s+\w+Glyphs\s*\[\]\s*PROGMEM\s*=\s*\{', content)
    if not glyph_match:
        raise ValueError(f"{filepath}: Could not find Glyphs[] array declaration")

    glyph_start = content.index('{', glyph_match.start()) + 1
    depth = 1
    pos = glyph_start
    while depth > 0 and pos < len(content):
        if content[pos] == '{':
            depth += 1
        elif content[pos] == '}':
            depth -= 1
        pos += 1
    glyph_end = pos - 1
    glyph_body = content[glyph_start:glyph_end]

    glyph_pattern = re.compile(
        r'\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\}'
        r'(?:\s*,?\s*//\s*(.*?))?$',
        re.MULTILINE
    )
    glyphs = []
    for m in glyph_pattern.finditer(glyph_body):
        glyphs.append({
            'bitmapOffset': int(m.group(1)),
            'width': int(m.group(2)),
            'height': int(m.group(3)),
            'xAdvance': int(m.group(4)),
            'xOffset': int(m.group(5)),
            'yOffset': int(m.group(6)),
            'comment': m.group(7).strip() if m.group(7) else '',
        })

    return bitmaps, glyphs


def glyph_byte_count(width, height):
    """Number of packed bytes for a glyph bitmap of given dimensions."""
    return (width * height + 7) // 8


def verify_fonts(file_a, file_b):
    """Verify two font files produce identical glyph bitmaps.

    Returns (pass, results) where pass is True if all match,
    and results is a list of (glyph_index, comment, match, detail).
    """
    bitmaps_a, glyphs_a = parse_font_bitmaps_and_glyphs(file_a)
    bitmaps_b, glyphs_b = parse_font_bitmaps_and_glyphs(file_b)

    if len(glyphs_a) != len(glyphs_b):
        print(f"ERROR: glyph count mismatch: {len(glyphs_a)} vs {len(glyphs_b)}",
              file=sys.stderr)
        return False, []

    all_pass = True
    results = []

    for i, (ga, gb) in enumerate(zip(glyphs_a, glyphs_b)):
        # Check that non-bitmap fields match
        field_mismatches = []
        for field in ('width', 'height', 'xAdvance', 'xOffset', 'yOffset'):
            if ga[field] != gb[field]:
                field_mismatches.append(f"{field}: {ga[field]} vs {gb[field]}")

        # Extract and compare bitmap data
        n_bytes = glyph_byte_count(ga['width'], ga['height'])
        data_a = bitmaps_a[ga['bitmapOffset']:ga['bitmapOffset'] + n_bytes]
        data_b = bitmaps_b[gb['bitmapOffset']:gb['bitmapOffset'] + n_bytes]

        bitmap_match = (data_a == data_b)
        detail = ""
        if not bitmap_match:
            detail = f"bitmap data differs ({n_bytes} bytes)"
        if field_mismatches:
            detail += ("; " if detail else "") + f"field mismatches: {', '.join(field_mismatches)}"

        match = bitmap_match and len(field_mismatches) == 0
        if not match:
            all_pass = False

        results.append((i, ga['comment'], match, detail))

    return all_pass, results


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} original.h deduplicated.h", file=sys.stderr)
        sys.exit(2)

    file_a, file_b = sys.argv[1], sys.argv[2]
    all_pass, results = verify_fonts(file_a, file_b)

    for idx, comment, match, detail in results:
        status = "OK  " if match else "FAIL"
        char_info = f"'{comment}'" if comment else f"glyph #{idx}"
        line = f"  [{status}] {char_info}"
        if detail:
            line += f" — {detail}"
        print(line)

    print()
    if all_pass:
        print(f"PASS: All {len(results)} glyphs match between {file_a} and {file_b}")
        sys.exit(0)
    else:
        fails = sum(1 for _, _, m, _ in results if not m)
        print(f"FAIL: {fails}/{len(results)} glyphs differ between {file_a} and {file_b}")
        sys.exit(1)


if __name__ == '__main__':
    main()
