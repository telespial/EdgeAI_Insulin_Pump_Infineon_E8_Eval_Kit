#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
  echo "Usage: $0 --change \"<summary>\" [--binary <path>] [--command \"<cmd>\"] [--result <ok|fail>]" >&2
  exit 1
}

CHANGE=""
BINARY=""
COMMAND=""
RESULT=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --change) CHANGE="$2"; shift 2 ;;
    --binary) BINARY="$2"; shift 2 ;;
    --command) COMMAND="$2"; shift 2 ;;
    --result) RESULT="$2"; shift 2 ;;
    *) usage ;;
  esac
done

if [[ -z "$CHANGE" ]]; then
  usage
fi

NOW_DATE=$(date +"%Y-%m-%d")
NOW_TS=$(date +"%Y-%m-%d %H:%M:%S")

{
  echo ""
  echo "## Update ${NOW_DATE}"
  echo "- Change: ${CHANGE}"
  if [[ -n "$BINARY" ]]; then
    echo "- Binary: ${BINARY}"
  fi
  if [[ -n "$COMMAND" ]]; then
    echo "- Command: ${COMMAND}"
  fi
  if [[ -n "$RESULT" ]]; then
    echo "- Result: ${RESULT}"
  fi
} >> "$ROOT_DIR/docs/PROJECT_STATE.md"

awk -v now="Last updated: $NOW_DATE" '
BEGIN {done=0}
{
  if ($0 ~ /^Last updated: / && !done) {
    print now
    done=1
  } else {
    print $0
  }
}
END {
  if (!done) print now
}
' "$ROOT_DIR/docs/PROJECT_STATE.md" > "$ROOT_DIR/docs/.PROJECT_STATE.tmp"
mv "$ROOT_DIR/docs/.PROJECT_STATE.tmp" "$ROOT_DIR/docs/PROJECT_STATE.md"

if [[ -n "$COMMAND" ]]; then
  echo "- ${NOW_TS} | ${COMMAND} | ${CHANGE}" >> "$ROOT_DIR/docs/COMMAND_LOG.md"
fi
