#!/usr/bin/env python3
import json
import math
import os
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST_PATH = ROOT / "tracks_manifest.json"
GEOJSON_PATH = Path("/tmp/f1-circuits.geojson")
ASSETS_DIR = ROOT / "assets" / "tracks"
LAYOUTS_PATH = ROOT / "generated" / "track_layouts.json"
GENERATED_HEADER_PATH = ROOT / "generated" / "track_layouts_generated.h"

IMG_SIZE = 900
PADDING = 40
TRACK_RADIUS = 18
WALL_THICKNESS = 4
CHECKPOINT_COUNT = 8

GRASS = (34, 177, 76)
TRACK = (35, 35, 35)
WALL = (15, 15, 15)


def dist(a, b):
    dx = a[0] - b[0]
    dy = a[1] - b[1]
    return math.hypot(dx, dy)


def densify(points, step=3.0):
    if len(points) < 2:
        return points[:]
    out = [points[0]]
    for i in range(1, len(points)):
        x1, y1 = points[i - 1]
        x2, y2 = points[i]
        d = dist((x1, y1), (x2, y2))
        if d <= 1e-6:
            continue
        n = max(1, int(d / step))
        for k in range(1, n + 1):
            t = k / n
            out.append((x1 + (x2 - x1) * t, y1 + (y2 - y1) * t))
    return out


def project_coords(coords):
    lons = [c[0] for c in coords]
    lats = [c[1] for c in coords]
    min_lon, max_lon = min(lons), max(lons)
    min_lat, max_lat = min(lats), max(lats)

    span_lon = max(max_lon - min_lon, 1e-9)
    span_lat = max(max_lat - min_lat, 1e-9)
    scale = min((IMG_SIZE - 2 * PADDING) / span_lon, (IMG_SIZE - 2 * PADDING) / span_lat)

    pts = []
    for lon, lat in coords:
        x = PADDING + (lon - min_lon) * scale
        y = IMG_SIZE - PADDING - (lat - min_lat) * scale
        pts.append((x, y))
    return pts


def draw_disc(buf, cx, cy, r, color):
    x0 = max(0, int(cx - r))
    x1 = min(IMG_SIZE - 1, int(cx + r))
    y0 = max(0, int(cy - r))
    y1 = min(IMG_SIZE - 1, int(cy + r))
    rr = r * r
    for y in range(y0, y1 + 1):
        dy = y - cy
        for x in range(x0, x1 + 1):
            dx = x - cx
            if dx * dx + dy * dy <= rr:
                idx = (y * IMG_SIZE + x) * 3
                buf[idx] = color[0]
                buf[idx + 1] = color[1]
                buf[idx + 2] = color[2]


def draw_polyline(buf, points, radius, color):
    for i in range(1, len(points)):
        x1, y1 = points[i - 1]
        x2, y2 = points[i]
        d = dist((x1, y1), (x2, y2))
        n = max(1, int(d))
        for k in range(n + 1):
            t = k / n
            x = x1 + (x2 - x1) * t
            y = y1 + (y2 - y1) * t
            draw_disc(buf, x, y, radius, color)


def write_ppm(path, buf):
    with open(path, "wb") as f:
        f.write(f"P6\n{IMG_SIZE} {IMG_SIZE}\n255\n".encode("ascii"))
        f.write(buf)


def to_png(ppm_path, png_path):
    subprocess.run(
        ["sips", "-s", "format", "png", str(ppm_path), "--out", str(png_path)],
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def close_loop(points):
    if len(points) < 2:
        return points
    if dist(points[0], points[-1]) > 1.0:
        return points + [points[0]]
    return points


def cumulative_lengths(points):
    acc = [0.0]
    total = 0.0
    for i in range(1, len(points)):
        total += dist(points[i - 1], points[i])
        acc.append(total)
    return acc, total


def point_at_length(points, acc, target):
    if target <= 0:
        p0, p1 = points[0], points[1]
        return p0, (p1[0] - p0[0], p1[1] - p0[1])
    if target >= acc[-1]:
        p0, p1 = points[-2], points[-1]
        return p1, (p1[0] - p0[0], p1[1] - p0[1])

    lo, hi = 0, len(acc) - 1
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if acc[mid] <= target:
            lo = mid
        else:
            hi = mid

    i = lo
    seg_len = max(1e-9, acc[i + 1] - acc[i])
    t = (target - acc[i]) / seg_len
    x1, y1 = points[i]
    x2, y2 = points[i + 1]
    x = x1 + (x2 - x1) * t
    y = y1 + (y2 - y1) * t
    return (x, y), (x2 - x1, y2 - y1)


def unit(vx, vy):
    n = math.hypot(vx, vy)
    if n < 1e-9:
        return (1.0, 0.0)
    return (vx / n, vy / n)


def clamp_point(p):
    x = min(max(p[0], 0.0), float(IMG_SIZE - 1))
    y = min(max(p[1], 0.0), float(IMG_SIZE - 1))
    return (x, y)


def build_layout(points):
    acc, total = cumulative_lengths(points)

    cp_half = TRACK_RADIUS + WALL_THICKNESS + 2
    checkpoints = []
    for i in range(CHECKPOINT_COUNT):
        target = (i * total) / CHECKPOINT_COUNT
        center, tangent = point_at_length(points, acc, target)
        tx, ty = unit(*tangent)
        nx, ny = -ty, tx
        s = clamp_point((center[0] + nx * cp_half, center[1] + ny * cp_half))
        e = clamp_point((center[0] - nx * cp_half, center[1] - ny * cp_half))
        checkpoints.append({
            "start": [round(s[0], 2), round(s[1], 2)],
            "end": [round(e[0], 2), round(e[1], 2)],
        })

    spawn_center, spawn_tangent = point_at_length(points, acc, total * 0.02)
    tx, ty = unit(*spawn_tangent)
    spawn = clamp_point((spawn_center[0] - tx * 6.0, spawn_center[1] - ty * 6.0))
    spawn_angle = math.atan2(ty, tx)

    return {
        "spawn_position": [round(spawn[0], 2), round(spawn[1], 2)],
        "spawn_angle": round(spawn_angle, 6),
        "checkpoints": checkpoints,
    }


def main():
    if not MANIFEST_PATH.exists():
        raise SystemExit(f"Missing manifest: {MANIFEST_PATH}")
    if not GEOJSON_PATH.exists():
        raise SystemExit(f"Missing geojson: {GEOJSON_PATH}")

    ASSETS_DIR.mkdir(parents=True, exist_ok=True)
    LAYOUTS_PATH.parent.mkdir(parents=True, exist_ok=True)

    manifest = json.loads(MANIFEST_PATH.read_text())
    geo = json.loads(GEOJSON_PATH.read_text())

    by_id = {}
    for f in geo["features"]:
        pid = f.get("properties", {}).get("id")
        if pid:
            by_id[pid] = f

    layouts = {}

    for t in manifest["tracks"]:
        slug = t["slug"]
        repo_id = t["repo_id"]
        if repo_id not in by_id:
            raise SystemExit(f"repo_id {repo_id} not found for slug {slug}")

        coords = by_id[repo_id]["geometry"]["coordinates"]
        points = project_coords(coords)
        points = close_loop(points)
        points = densify(points, step=3.0)

        buf = bytearray([0] * (IMG_SIZE * IMG_SIZE * 3))
        for i in range(0, len(buf), 3):
            buf[i] = GRASS[0]
            buf[i + 1] = GRASS[1]
            buf[i + 2] = GRASS[2]

        draw_polyline(buf, points, TRACK_RADIUS + WALL_THICKNESS, WALL)
        draw_polyline(buf, points, TRACK_RADIUS, TRACK)

        ppm_path = ASSETS_DIR / f"{slug}.ppm"
        png_path = ASSETS_DIR / f"{slug}.png"
        write_ppm(ppm_path, buf)
        to_png(ppm_path, png_path)
        ppm_path.unlink(missing_ok=True)

        layout = build_layout(points)
        layout["image_path"] = f"assets/tracks/{slug}.png"
        layout["screen_width"] = IMG_SIZE
        layout["screen_height"] = IMG_SIZE
        layout["repo_id"] = repo_id
        layouts[slug] = layout

    payload = {
        "generated_from": {
            "manifest": str(MANIFEST_PATH),
            "geojson": str(GEOJSON_PATH),
        },
        "tracks": layouts,
    }
    LAYOUTS_PATH.write_text(json.dumps(payload, indent=2))
    write_generated_header(layouts)
    print(f"Generated {len(layouts)} tracks")
    print(f"Assets: {ASSETS_DIR}")
    print(f"Layouts: {LAYOUTS_PATH}")
    print(f"Header: {GENERATED_HEADER_PATH}")


def write_generated_header(layouts):
    lines = []
    lines.append("#pragma once")
    lines.append("")
    lines.append('#include <string>')
    lines.append("")
    lines.append("inline bool ApplyGeneratedLayout(const std::string& slug, TrackConfig& track) {")

    for slug in sorted(layouts.keys()):
        layout = layouts[slug]
        lines.append(f'    if (slug == "{slug}") {{')
        lines.append("        track.is_stub = false;")
        lines.append(f'        track.image_path = "{layout["image_path"]}";')
        lines.append(f'        track.screen_width = {int(layout["screen_width"])};')
        lines.append(f'        track.screen_height = {int(layout["screen_height"])};')
        sx, sy = layout["spawn_position"]
        lines.append(f"        track.spawn_position = {{{sx:.2f}f, {sy:.2f}f}};")
        lines.append(f'        track.spawn_angle = {float(layout["spawn_angle"]):.6f}f;')
        lines.append("        track.checkpoints = {")
        for cp in layout["checkpoints"]:
            x1, y1 = cp["start"]
            x2, y2 = cp["end"]
            lines.append(
                f"            {{{{{x1:.2f}f, {y1:.2f}f}}, {{{x2:.2f}f, {y2:.2f}f}}}},"
            )
        lines.append("        };")
        lines.append("        return true;")
        lines.append("    }")
        lines.append("")

    lines.append("    return false;")
    lines.append("}")
    lines.append("")

    GENERATED_HEADER_PATH.write_text("\n".join(lines))


if __name__ == "__main__":
    main()
