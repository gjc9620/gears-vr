"""Read-only snapshot analysis. Input is top-to-bottom desktop Z order.

No window operations, titles, or process discovery. Callers supply verified test
PIDs and shell HWND exclusions; unknown windows must not be silently excluded.
Rectangles are physical screen coordinates and may have negative origins.
"""


def intersection(a, b):
    r = (max(a[0], b[0]), max(a[1], b[1]), min(a[2], b[2]), min(a[3], b[3]))
    return r if r[0] < r[2] and r[1] < r[3] else None


def subtract(rect, cover):
    hit = intersection(rect, cover)
    if hit is None:
        return [rect]
    x, y, right, bottom = rect
    a, b, c, d = hit
    return [r for r in ((x, y, right, b), (x, d, right, bottom),
                         (x, b, a, d), (c, b, right, d))
            if r[0] < r[2] and r[1] < r[3]]


def find_occlusion(windows, test_pids, excluded_hwnds=()):
    """Report visible test pixels covering other visible application windows.

    Rectangular approximation: transparency/window regions are not represented.
    This is sampled evidence, never proof of every instant or actual pixels.
    Minimized, hidden and DWM-cloaked windows contribute no visible area.
    """
    rows = [w for w in windows if w['visible'] and not w['minimized']
            and not w['cloaked'] and w['hwnd'] not in excluded_hwnds]
    results = []
    for index, test in enumerate(rows):
        if test['pid'] not in test_pids:
            continue
        exposed = [test['rect']]
        for above in rows[:index]:
            exposed = [part for rect in exposed for part in subtract(rect, above['rect'])]
        for other in rows[index + 1:]:
            if other['pid'] not in test_pids:
                hits = [hit for rect in exposed
                        if (hit := intersection(rect, other['rect'])) is not None]
                if hits:
                    results.append(dict(test_hwnd=test['hwnd'], covered_hwnd=other['hwnd'],
                                        overlap_rects=hits))
            # Attribute each exposed pixel to the first underlying window only.
            exposed = [part for rect in exposed for part in subtract(rect, other['rect'])]
    return results
