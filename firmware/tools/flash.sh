#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT_DIR/tools/preflight.sh"

FLASH_CMD="${FLASH_CMD:-}"
BIN_PATH="${BIN_PATH:-}"

if [[ -z "$BIN_PATH" ]]; then
  BIN_PATH=$(awk -F': ' '/^\- Binary: /{print $2}' "$ROOT_DIR/docs/PROJECT_STATE.md" | tail -n 1)
fi

if [[ -z "$FLASH_CMD" ]]; then
  FLASH_CMD=$(awk -F': ' '/^\- Flash Command: /{print $2}' "$ROOT_DIR/docs/PROJECT_STATE.md" | tail -n 1)
fi

if [[ -z "$FLASH_CMD" ]]; then
  echo "No flash command configured." >&2
  echo "Set FLASH_CMD or '- Flash Command:' in docs/PROJECT_STATE.md" >&2
  exit 1
fi

if [[ -n "$BIN_PATH" ]]; then
  FLASH_CMD="${FLASH_CMD//\{BIN_PATH\}/$BIN_PATH}"
fi

STATUS_FILE="${STATUS_FILE:-}"
if [[ -z "$STATUS_FILE" ]]; then
  STATUS_FILE=$(awk -F': ' '/^\- Status File: /{print $2}' "$ROOT_DIR/docs/PROJECT_STATE.md" | tail -n 1)
fi

if [[ -n "$STATUS_FILE" ]]; then
  trap '"$ROOT_DIR/tools/update_project_status.sh" --status "$STATUS_FILE" --action flash --result fail --binary "$BIN_PATH" --notes "flash failed"' ERR
fi

bash -lc "$FLASH_CMD"

if [[ -n "$STATUS_FILE" ]]; then
  "$ROOT_DIR/tools/update_project_status.sh" --status "$STATUS_FILE" --action flash --result ok --binary "$BIN_PATH" --notes "flash ok"
fi
