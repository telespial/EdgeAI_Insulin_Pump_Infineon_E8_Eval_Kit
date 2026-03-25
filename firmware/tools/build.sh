#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT_DIR/tools/preflight.sh"

BUILD_CMD="${BUILD_CMD:-}"
if [[ -z "$BUILD_CMD" ]]; then
  BUILD_CMD=$(awk -F': ' '/^\- Build Command: /{print $2}' "$ROOT_DIR/docs/PROJECT_STATE.md" | tail -n 1)
fi

if [[ -z "$BUILD_CMD" ]]; then
  echo "No build command configured." >&2
  echo "Set BUILD_CMD or '- Build Command:' in docs/PROJECT_STATE.md" >&2
  exit 1
fi

STATUS_FILE="${STATUS_FILE:-}"
if [[ -z "$STATUS_FILE" ]]; then
  STATUS_FILE=$(awk -F': ' '/^\- Status File: /{print $2}' "$ROOT_DIR/docs/PROJECT_STATE.md" | tail -n 1)
fi

BIN_PATH="${BIN_PATH:-}"
if [[ -z "$BIN_PATH" ]]; then
  BIN_PATH=$(awk -F': ' '/^\- Binary: /{print $2}' "$ROOT_DIR/docs/PROJECT_STATE.md" | tail -n 1)
fi

if [[ -n "$STATUS_FILE" ]]; then
  trap '"$ROOT_DIR/tools/update_project_status.sh" --status "$STATUS_FILE" --action build --result fail --binary "$BIN_PATH" --notes "build failed"' ERR
fi

bash -lc "$BUILD_CMD"

if [[ -n "$STATUS_FILE" ]]; then
  "$ROOT_DIR/tools/update_project_status.sh" --status "$STATUS_FILE" --action build --result ok --binary "$BIN_PATH" --notes "build ok"
fi
