# /// script
# requires-python = ">=3.10"
# dependencies = ["fonttools[woff]==4.67.0"]
# ///

import argparse
import json
import re
import shutil
from pathlib import Path
from tempfile import TemporaryDirectory
from urllib.request import Request, urlopen

from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont


REPOSITORY = "google/material-design-icons"
SOURCE_NAME = "MaterialSymbolsRounded[FILL,GRAD,opsz,wght]"
FONT_NAME = "MaterialSymbolsRounded.wght_400.opsz_24"
FIXED_AXES = {"wght": 400, "opsz": 24, "GRAD": 0}
ROOT = Path(__file__).resolve().parent.parent


def download(url):
    request = Request(url, headers={"User-Agent": "QmlMaterial icon updater"})
    with urlopen(request, timeout=120) as response:
        return response.read()


def resolve_revision(revision):
    if revision == "master":
        ref = json.loads(download(f"https://api.github.com/repos/{REPOSITORY}/git/ref/heads/master"))
        revision = ref["object"]["sha"]
    if not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise ValueError("revision must be a full commit SHA or master")
    return revision


def parse_codepoints(text):
    entries = {}
    for line in text.splitlines():
        name, value = line.split()
        codepoint = int(value, 16)
        if not re.fullmatch(r"[a-z0-9_]+", name) or name in entries:
            raise ValueError(f"invalid or duplicate icon name: {name}")
        if not 0 <= codepoint <= 0x10FFFF or 0xD800 <= codepoint <= 0xDFFF:
            raise ValueError(f"invalid Unicode codepoint: {value}")
        entries[name] = codepoint
    if not entries:
        raise ValueError("empty codepoints file")
    return entries


def write_codepoints(path, entries, revision):
    lines = [f"// Generated from {REPOSITORY} at {revision}.\n"]
    for name, codepoint in sorted(entries.items()):
        escape = f"\\u{codepoint:04x}" if codepoint <= 0xFFFF else f"\\U{codepoint:08x}"
        lines.append(f'CODE({name}, "{escape}");\n')
    path.write_text("".join(lines), encoding="ascii")


def set_static_names(font, fill):
    family = f"Material Symbols Rounded wght_400 opsz_24 FILL_{fill} GRAD_0"
    postscript = f"MaterialSymbolsRounded-wght400-opsz24-FILL{fill}-GRAD0"
    names = {
        1: family,
        2: "Regular",
        3: f"{font['head'].fontRevision:.3f};GOOG;{postscript}",
        4: family,
        6: postscript,
        16: family,
        17: "Regular",
    }
    table = font["name"]
    for record in list(table.names):
        if record.nameID in names:
            table.setName(names[record.nameID], record.nameID,
                          record.platformID, record.platEncID, record.langID)
    for name_id, value in names.items():
        table.setName(value, name_id, 3, 1, 0x409)


def verify_font(path, entries, expected_axes):
    with TTFont(path) as font:
        axes = {axis.axisTag for axis in font["fvar"].axes} if "fvar" in font else set()
        if axes != expected_axes:
            raise ValueError(f"{path.name}: unexpected axes {axes}")
        if not expected_axes and any(tag in font for tag in ("gvar", "HVAR", "MVAR")):
            raise ValueError(f"{path.name}: variation tables remain in static font")
        if "GSUB" not in font:
            raise ValueError(f"{path.name}: missing ligature substitutions")
        cmap = font.getBestCmap()
        missing = [name for name, codepoint in entries.items() if codepoint not in cmap]
        if missing:
            raise ValueError(f"{path.name}: missing glyphs: {', '.join(missing)}")


def generate_fonts(source, output, entries):
    with TTFont(source, recalcTimestamp=False) as variable:
        axes = {axis.axisTag for axis in variable["fvar"].axes}
        if axes != set(FIXED_AXES) | {"FILL"}:
            raise ValueError(f"unexpected source axes: {axes}")
        partial = instantiateVariableFont(variable, FIXED_AXES)
    try:
        partial.flavor = "woff2"
        partial.save(output / f"{FONT_NAME}.woff2")
        for fill in (0, 1):
            instance = instantiateVariableFont(partial, {"FILL": fill})
            try:
                set_static_names(instance, fill)
                for flavor in (None, "woff2"):
                    instance.flavor = flavor
                    extension = flavor or "ttf"
                    instance.save(output / f"{FONT_NAME}.fill_{fill}.{extension}")
            finally:
                instance.close()
    finally:
        partial.close()
    for path in sorted(output.iterdir()):
        verify_font(path, entries, {"FILL"} if path.name == f"{FONT_NAME}.woff2" else set())
        print(f"  {path.name}: {path.stat().st_size:,} bytes", flush=True)


def main():
    parser = argparse.ArgumentParser(description="Update Material Symbols Rounded fonts and icon codes.")
    parser.add_argument("--revision", default="master", help="upstream commit SHA or master")
    args = parser.parse_args()
    revision = resolve_revision(args.revision)
    print(f"Material Symbols source: {revision}", flush=True)
    source_url = f"https://raw.githubusercontent.com/{REPOSITORY}/{revision}/variablefont/"
    source_url += SOURCE_NAME.replace("[", "%5B").replace("]", "%5D")
    with TemporaryDirectory(prefix="qml-material-icons-") as temporary:
        temporary = Path(temporary)
        entries = parse_codepoints(download(source_url + ".codepoints").decode("ascii"))
        source = temporary / "source.ttf"
        source.write_bytes(download(source_url + ".ttf"))
        output = temporary / "fonts"
        output.mkdir()
        print(f"Generating fonts for {len(entries):,} icon names...", flush=True)
        generate_fonts(source, output, entries)
        codepoints = temporary / "icon_code.inl"
        write_codepoints(codepoints, entries, revision)
        for path in output.iterdir():
            shutil.copyfile(path, ROOT / "assets" / path.name)
        shutil.copyfile(codepoints, ROOT / "include/qml_material/token/icon_code.inl")
    print("Updated fonts and icon_code.inl.", flush=True)


if __name__ == "__main__":
    main()
