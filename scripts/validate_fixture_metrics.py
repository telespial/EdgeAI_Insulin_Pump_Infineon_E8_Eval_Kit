#!/usr/bin/env python3
import json
import os
import re
import sys


SUMMARY_PATTERNS = {
    "baseline_mae_rmse": re.compile(r"^  baseline 15m MAE/RMSE: ([0-9.]+) / ([0-9.]+)$"),
    "ml_mae_rmse": re.compile(r"^  ML 15m MAE/RMSE: ([0-9.]+) / ([0-9.]+)$"),
    "tir": re.compile(r"^  time in range: ([0-9.]+)%$"),
    "tbr": re.compile(r"^  time below range: ([0-9.]+)%$"),
    "tar": re.compile(r"^  time above range: ([0-9.]+)%$"),
    "safety": re.compile(r"^  safety intervention count: ([0-9]+)$"),
    "disagreement": re.compile(r"^  controller disagreement count: ([0-9]+)$"),
}


def load_expectations(path):
    with open(path, "r", encoding="utf-8") as handle:
        return json.load(handle)


def parse_summary(path):
    metrics = {}
    with open(path, "r", encoding="utf-8") as handle:
        for line in handle:
            line = line.rstrip("\n")
            match = SUMMARY_PATTERNS["baseline_mae_rmse"].match(line)
            if match:
                metrics["baseline_mae_15"] = float(match.group(1))
                metrics["baseline_rmse_15"] = float(match.group(2))
                continue
            match = SUMMARY_PATTERNS["ml_mae_rmse"].match(line)
            if match:
                metrics["ml_mae_15"] = float(match.group(1))
                metrics["ml_rmse_15"] = float(match.group(2))
                continue
            match = SUMMARY_PATTERNS["tir"].match(line)
            if match:
                metrics["time_in_range"] = float(match.group(1))
                continue
            match = SUMMARY_PATTERNS["tbr"].match(line)
            if match:
                metrics["time_below_range"] = float(match.group(1))
                continue
            match = SUMMARY_PATTERNS["tar"].match(line)
            if match:
                metrics["time_above_range"] = float(match.group(1))
                continue
            match = SUMMARY_PATTERNS["safety"].match(line)
            if match:
                metrics["safety_interventions"] = int(match.group(1))
                continue
            match = SUMMARY_PATTERNS["disagreement"].match(line)
            if match:
                metrics["controller_disagreement"] = int(match.group(1))
                continue
    return metrics


def audit_contains_reason(path, reason_code):
    with open(path, "r", encoding="utf-8") as handle:
        for line in handle:
            if reason_code in line:
                return True
    return False


def check_bounds(name, metrics, expectations, failures):
    numeric_map = [
        ("baseline_mae_15", "baseline_mae_15_min", "baseline_mae_15_max"),
        ("baseline_rmse_15", "baseline_rmse_15_min", "baseline_rmse_15_max"),
        ("ml_mae_15", "ml_mae_15_min", "ml_mae_15_max"),
        ("ml_rmse_15", "ml_rmse_15_min", "ml_rmse_15_max"),
        ("time_in_range", "time_in_range_min_pct", "time_in_range_max_pct"),
        ("time_below_range", "time_below_range_min_pct", "time_below_range_max_pct"),
        ("time_above_range", "time_above_range_min_pct", "time_above_range_max_pct"),
        ("safety_interventions", "safety_interventions_min", "safety_interventions_max"),
        ("controller_disagreement", "controller_disagreement_min", "controller_disagreement_max"),
    ]

    print(f"[fixture] {name}")
    for metric_name, min_key, max_key in numeric_map:
        if metric_name not in metrics:
            failures.append(f"{name}: missing metric {metric_name}")
            print(f"  FAIL {metric_name}: missing")
            continue

        value = metrics[metric_name]
        minimum = expectations.get(min_key)
        maximum = expectations.get(max_key)
        if minimum is not None and value < minimum:
            failures.append(f"{name}: {metric_name}={value} < {minimum}")
            print(f"  FAIL {metric_name}: {value} < {minimum}")
            continue
        if maximum is not None and value > maximum:
            failures.append(f"{name}: {metric_name}={value} > {maximum}")
            print(f"  FAIL {metric_name}: {value} > {maximum}")
            continue
        print(f"  PASS {metric_name}: {value}")


def main():
    if len(sys.argv) != 3:
        print("Usage: validate_fixture_metrics.py <fixture_dir> <expectations_json>", file=sys.stderr)
        return 2

    fixture_dir = sys.argv[1]
    expectations_path = sys.argv[2]
    expectations = load_expectations(expectations_path)
    failures = []

    for name, fixture_expectations in expectations.items():
        audit_path = os.path.join(fixture_dir, f"{name}_audit.csv")
        summary_path = os.path.join(fixture_dir, f"{name}_summary.txt")

        if not os.path.isfile(audit_path):
            failures.append(f"{name}: missing audit file {audit_path}")
            print(f"[fixture] {name} FAIL missing audit file")
            continue
        if not os.path.isfile(summary_path):
            failures.append(f"{name}: missing summary file {summary_path}")
            print(f"[fixture] {name} FAIL missing summary file")
            continue

        metrics = parse_summary(summary_path)
        check_bounds(name, metrics, fixture_expectations, failures)

        for reason_code in fixture_expectations.get("required_reason_codes", []):
            if not audit_contains_reason(audit_path, reason_code):
                failures.append(f"{name}: missing reason code {reason_code}")
                print(f"  FAIL required reason code: {reason_code}")
            else:
                print(f"  PASS required reason code: {reason_code}")

    if failures:
        print("\nFixture metric validation failed:", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1

    print("\nFixture metric validation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
