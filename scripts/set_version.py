#!/usr/bin/env python3

import argparse
import json
import re
import sys
from pathlib import Path


SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")


def read_version_arg_or_file(project_root: Path, cli_version: str | None) -> str:
    if cli_version:
        version = cli_version.strip()
    else:
        version_file = project_root / "VERSION"
        if not version_file.exists():
            raise SystemExit("No version provided and VERSION file not found.")
        version = version_file.read_text(encoding="utf-8").strip()
    if not SEMVER_RE.match(version):
        raise SystemExit(f"Invalid version '{version}'. Expected semantic version like 1.0.9")
    return version


def update_library_json(path: Path, version: str) -> bool:
    if not path.exists():
        return False
    data = json.loads(path.read_text(encoding="utf-8"))
    data["version"] = version
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return True


def update_library_properties(path: Path, version: str) -> bool:
    if not path.exists():
        return False
    lines = path.read_text(encoding="utf-8").splitlines()
    updated = False
    for i, line in enumerate(lines):
        if line.startswith("version="):
            lines[i] = f"version={version}"
            updated = True
            break
    if not updated:
        lines.append(f"version={version}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return True


def update_header_define(path: Path, version: str) -> bool:
    if not path.exists():
        return False
    content = path.read_text(encoding="utf-8")
    pattern = re.compile(r"(#define\s+IS31FL373X_VERSION\s+)\"[^\"]*\"")
    if not pattern.search(content):
        raise SystemExit("Could not find IS31FL373X_VERSION define in header.")
    # Use a callable replacement to avoid escape semantics in replacement strings
    def repl(m: re.Match) -> str:
        return f"{m.group(1)}\"{version}\""
    new_content = pattern.sub(repl, content)
    path.write_text(new_content, encoding="utf-8")
    return True


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Set project version across files.")
    parser.add_argument("version", nargs="?", help="Semantic version, e.g., 1.0.9. If omitted, read VERSION file.")
    args = parser.parse_args(argv)

    project_root = Path(__file__).resolve().parents[1]
    version = read_version_arg_or_file(project_root, args.version)

    # Write VERSION file
    (project_root / "VERSION").write_text(version + "\n", encoding="utf-8")

    updated_any = False
    updated_any |= update_library_json(project_root / "library.json", version)
    updated_any |= update_library_properties(project_root / "library.properties", version)
    updated_any |= update_header_define(project_root / "src" / "IS31FL373x.h", version)

    if not updated_any:
        print("Warning: No target files were updated.")
    print(f"Version set to {version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))


