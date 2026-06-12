#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

audit_schema='timestamp,glucose_mgdl,sqi_pct,iob_u,cob_g,baseline_pred_15,baseline_pred_30,baseline_pred_60,ml_pred_15,ml_pred_30,ml_pred_60,actual_15,actual_30,actual_60,baseline_error_15,baseline_error_30,baseline_error_60,ml_error_15,ml_error_30,ml_error_60,controller_without_ml,controller_with_ml,safety_final_action,safety_reason_codes'
output_dir="out/fixture_matrix"
fixtures=(data/sample_replay_*.csv)

make -f host.mk build
rm -rf "$output_dir"
mkdir -p "$output_dir"

for fixture in "${fixtures[@]}"; do
    fixture_name="$(basename "$fixture" .csv)"
    fixture_key="${fixture_name#sample_replay_}"
    audit_path="$output_dir/${fixture_key}_audit.csv"
    summary_path="$output_dir/${fixture_key}_summary.txt"

    if [[ ! -f "$fixture" ]]; then
        echo "missing fixture: $fixture" >&2
        exit 1
    fi

    ./host_build/e84_aps_sim --replay "$fixture" --out "$audit_path" > "$summary_path"

    if [[ ! -s "$audit_path" ]]; then
        echo "missing audit output: $audit_path" >&2
        exit 1
    fi
    if [[ ! -s "$summary_path" ]]; then
        echo "missing summary output: $summary_path" >&2
        exit 1
    fi

    if ! grep -Fxq "$audit_schema" "$audit_path"; then
        echo "audit schema mismatch for $fixture" >&2
        exit 1
    fi

    for expected in \
        "baseline 15m MAE/RMSE:" \
        "ML 15m MAE/RMSE:" \
        "time in range:" \
        "safety intervention count:" \
        "controller disagreement count:"; do
        if ! grep -Fq "$expected" "$summary_path"; then
            echo "missing summary field '$expected' for $fixture" >&2
            exit 1
        fi
    done

    case "$fixture_key" in
        bad_sqi)
            if ! grep -Eq 'BAD_SQI|CONTROLLER_BLOCKED' "$audit_path"; then
                echo "missing bad SQI safety behavior for $fixture" >&2
                exit 1
            fi
            ;;
        stale_cgm)
            if ! grep -Eq 'STALE_CGM|CONTROLLER_BLOCKED' "$audit_path"; then
                echo "missing stale CGM safety behavior for $fixture" >&2
                exit 1
            fi
            ;;
        falling_bolus)
            if ! grep -Eq 'RAPID_FALL|PREDICTED_LOW_15M|PREDICTED_LOW_30M' "$audit_path"; then
                echo "missing falling bolus safety behavior for $fixture" >&2
                exit 1
            fi
            ;;
        meal_rise)
            if ! awk -F, 'NR > 1 { if ($9 > $6 || $10 > $7) { found = 1 } } END { exit(found ? 0 : 1) }' "$audit_path"; then
                if ! grep -Eq 'controller disagreement count: [1-9]' "$summary_path"; then
                    echo "meal rise did not show elevated prediction or disagreement behavior" >&2
                    exit 1
                fi
            fi
            ;;
    esac

    echo "fixture ok: $fixture_key"
done
