#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

required_files=(
  "$ROOT_DIR/docs/START_HERE.md"
  "$ROOT_DIR/docs/PROJECT_STATE.md"
  "$ROOT_DIR/docs/OPS_RUNBOOK.md"
  "$ROOT_DIR/docs/HARDWARE_SETUP.md"
  "$ROOT_DIR/docs/COMMAND_LOG.md"
)

missing=0
for f in "${required_files[@]}"; do
  if [[ ! -f "$f" ]]; then
    echo "Missing required file: $f" >&2
    missing=1
  fi
done

if [[ $missing -ne 0 ]]; then
  echo "Preflight failed." >&2
  exit 1
fi

echo "Preflight OK."
