#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "Usage: $0 --status <path> --action <build|flash> --result <ok|fail> [--binary <path>] [--notes <text>]" >&2
  exit 1
}

STATUS_FILE=""
ACTION=""
RESULT=""
BINARY=""
NOTES=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --status) STATUS_FILE="$2"; shift 2 ;;
    --action) ACTION="$2"; shift 2 ;;
    --result) RESULT="$2"; shift 2 ;;
    --binary) BINARY="$2"; shift 2 ;;
    --notes) NOTES="$2"; shift 2 ;;
    *) usage ;;
  esac
done

if [[ -z "$STATUS_FILE" || -z "$ACTION" || -z "$RESULT" ]]; then
  usage
fi

if [[ -z "$NOTES" ]]; then
  NOTES="Action: ${ACTION}"
fi

NOW_TS=$(date +"%Y-%m-%d %H:%M:%S")
mkdir -p "$(dirname "$STATUS_FILE")"

if [[ ! -f "$STATUS_FILE" ]]; then
  cat > "$STATUS_FILE" << 'BASE'
# Project Status

## Last Run
- Date:
- Result:
- Binary:
- Notes:
BASE
fi

awk -v ts="$NOW_TS" -v action="$ACTION" -v result="$RESULT" -v binary="$BINARY" -v notes="$NOTES" '
BEGIN {in_last=0}
{
  if ($0 == "## Last Run") { in_last=1; print; next }
  if (in_last && $0 ~ /^- Date:/)   { print "- Date: " ts; next }
  if (in_last && $0 ~ /^- Result:/) { print "- Result: " action " " result; next }
  if (in_last && $0 ~ /^- Binary:/) { print "- Binary: " binary; next }
  if (in_last && $0 ~ /^- Notes:/)  { print "- Notes: " notes; in_last=0; next }
  print
}
' "$STATUS_FILE" > "$STATUS_FILE.tmp"
mv "$STATUS_FILE.tmp" "$STATUS_FILE"
