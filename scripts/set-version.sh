#!/usr/bin/env bash

set -euo pipefail

PROJECT_ROOT=$(cd "$(dirname "$0")/.." && pwd)

usage() {
    echo "Usage: $0 <version>"
    echo "       $0 1.0.9"
    echo "If no version is given, reads VERSION file in project root."
}

if [[ ${1:-} == "-h" || ${1:-} == "--help" ]]; then
    usage
    exit 0
fi

VERSION_INPUT=${1:-}

if [[ -z "$VERSION_INPUT" ]]; then
    if [[ -f "$PROJECT_ROOT/VERSION" ]]; then
        VERSION_INPUT=$(cat "$PROJECT_ROOT/VERSION" | tr -d '\n' | tr -d '\r')
    else
        echo "ERROR: No version provided and VERSION file not found." >&2
        usage
        exit 1
    fi
fi

if [[ ! "$VERSION_INPUT" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "ERROR: Version must be semantic version (e.g., 1.0.9). Got: $VERSION_INPUT" >&2
    exit 1
fi

VERSION="$VERSION_INPUT"

# Cross-platform sed -i
sedi() {
  # args: pattern file
  if sed --version >/dev/null 2>&1; then
    sed -i "$1" "$2"
  else
    sed -i '' "$1" "$2"
  fi
}

echo "$VERSION" > "$PROJECT_ROOT/VERSION"

# Update library.json
LIB_JSON="$PROJECT_ROOT/library.json"
if [[ -f "$LIB_JSON" ]]; then
  sedi "s/\(\"version\"[[:space:]]*:[[:space:]]*\)\"[^"]*\"/\1\"$VERSION\"/" "$LIB_JSON"
  echo "Updated $LIB_JSON"
fi

# Update library.properties
LIB_PROP="$PROJECT_ROOT/library.properties"
if [[ -f "$LIB_PROP" ]]; then
  sedi "s/^version=.*/version=$VERSION/" "$LIB_PROP"
  echo "Updated $LIB_PROP"
fi

# Update header define
HDR="$PROJECT_ROOT/src/IS31FL373x.h"
if [[ -f "$HDR" ]]; then
  sedi "s/^#define[[:space:]]\+IS31FL373X_VERSION[[:space:]]\+\"[^"]*\"/#define IS31FL373X_VERSION \"$VERSION\"/" "$HDR"
  echo "Updated $HDR"
fi

echo "Version set to $VERSION"


