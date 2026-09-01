#!/usr/bin/env python3
"""Benchmark runner for celesq/Social-Media-Network.

Pipeline:
  1. source-contract/preflight checks
  2. normal build
  3. existing .in/.ref regression tests when the original users.db is available
  4. deterministic generated-case correctness sentinels
  5. repeated uninstrumented timing + peak RSS
  6. optional Valgrind checks
  7. optional ASan/UBSan checks
  8. atomic JSON checkpoints throughout

"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import platform
import re
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any

from generate_stress import generate, inspect_repo, sha256_file

SCHEMA_VERSION = 2


def atomic_json(path: Path, obj: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=str(path.parent))
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            json.dump(obj, f, indent=2)
            f.write("\n")
            f.flush()
            os.fsync(f.fileno())
        os.replace(tmp, path)
    finally:
        if os.path.exists(tmp):
            os.unlink(tmp)


def run_cmd(cmd: list[str], cwd: Path, timeout: int = 120,
            stdout=None, stderr=None, env: dict[str, str] | None = None,
            check: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(
        cmd, cwd=str(cwd), timeout=timeout, stdout=stdout, stderr=stderr,
        env=env, check=check,
    )


def capture_text(cmd: list[str], cwd: Path, timeout: int = 30) -> str | None:
    try:
        p = subprocess.run(cmd, cwd=str(cwd), timeout=timeout, text=True,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        return p.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return None


def git_commit(repo: Path) -> str | None:
    if not shutil.which("git") or not (repo / ".git").exists():
        return None
    out = capture_text(["git", "rev-parse", "HEAD"], repo)
    return out.splitlines()[0] if out else None


def cpu_model() -> str | None:
    p = Path("/proc/cpuinfo")
    if not p.exists():
        return None
    for line in p.read_text(errors="replace").splitlines():
        if line.lower().startswith("model name") and ":" in line:
            return line.split(":", 1)[1].strip()
    return None


def normalize_output(text: str) -> str:
    # Existing checker references are text output. Ignore CRLF/trailing whitespace only;
    # do not reorder lines or otherwise weaken correctness checks.
    return "\n".join(line.rstrip() for line in text.replace("\r\n", "\n").split("\n")).rstrip() + "\n"


def build(repo: Path, cflags: str | None = None) -> dict:
    env = os.environ.copy()
    clean = subprocess.run(["make", "clean"], cwd=repo, text=True,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
    cmd = ["make", "-j"]
    if cflags is not None:
        cmd.append(f"CFLAGS={cflags}")
    p = subprocess.run(cmd, cwd=repo, text=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, env=env)
    executables = {name: (repo / name).is_file() for name in ("friends", "posts", "feed")}
    return {
        "clean_returncode": clean.returncode,
        "returncode": p.returncode,
        "output": p.stdout[-20000:],
        "executables": executables,
        "ok": p.returncode == 0 and all(executables.values()),
        "cflags_override": cflags,
    }


def infer_executable(stem: str) -> str | None:
    s = stem.lower()
    # Task-3/feed executable includes all three handlers, but original checker fixture
    # naming convention contains the target task in the filename.
    if "friends" in s:
        return "friends"
    if "posts" in s:
        return "posts"
    if "feed" in s:
        return "feed"
    return None


def discover_regression_pairs(repo: Path) -> list[tuple[Path, Path, str]]:
    inp = repo / "checker" / "input"
    ref = repo / "checker" / "ref"
    if not inp.is_dir() or not ref.is_dir():
        return []
    pairs: list[tuple[Path, Path, str]] = []
    for input_path in sorted(inp.glob("*.in")):
        ref_path = ref / (input_path.stem + ".ref")
        exe = infer_executable(input_path.stem)
        if ref_path.exists() and exe:
            pairs.append((input_path, ref_path, exe))
    return pairs


def run_regressions(repo: Path, timeout_s: int = 30) -> dict:
    pairs = discover_regression_pairs(repo)
    if not pairs:
        return {"status": "skipped", "reason": "No checker/input + checker/ref .in/.ref pairs discovered", "tests": []}
    if not (repo / "users.db").exists():
        return {
            "status": "skipped",
            "reason": "Repository root has no users.db; original reference fixtures cannot be reproduced safely with synthetic usernames",
            "discovered_pairs": len(pairs), "tests": [],
        }

    results = []
    for input_path, ref_path, exe in pairs:
        with input_path.open("rb") as fin:
            try:
                p = subprocess.run([str((repo / exe).resolve())], cwd=repo, stdin=fin,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   timeout=timeout_s)
                actual = p.stdout.decode("utf-8", errors="replace")
                expected = ref_path.read_text(encoding="utf-8", errors="replace")
                ok = p.returncode == 0 and normalize_output(actual) == normalize_output(expected)
                result = {
                    "name": input_path.name, "executable": exe, "returncode": p.returncode,
                    "passed": ok, "stderr": p.stderr.decode("utf-8", errors="replace")[-4000:],
                    "input_sha256": sha256_file(input_path), "ref_sha256": sha256_file(ref_path),
                }
                if not ok:
                    result["expected_tail"] = normalize_output(expected).splitlines()[-10:]
                    result["actual_tail"] = normalize_output(actual).splitlines()[-10:]
                results.append(result)
            except subprocess.TimeoutExpired:
                results.append({"name": input_path.name, "executable": exe, "passed": False, "timeout": True})
    passed = sum(bool(x.get("passed")) for x in results)
    return {
        "status": "passed" if passed == len(results) else "failed",
        "passed": passed, "total": len(results), "tests": results,
    }


def verify_manifest(manifest_path: Path, generated_dir: Path, repo: Path) -> dict:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema_version") != 2:
        raise SystemExit("Unsupported generated manifest schema")
    current = inspect_repo(repo)
    generated_contract = manifest.get("repo_contract", {})
    if current["max_people"] != generated_contract.get("max_people"):
        raise SystemExit("MAX_PEOPLE changed after workload generation; regenerate benchmarks")
    if current["max_command_len"] != generated_contract.get("max_command_len"):
        raise SystemExit("MAX_COMMAND_LEN changed after workload generation; regenerate benchmarks")

    db = generated_dir / manifest["synthetic_users_db"]["file"]
    if sha256_file(db) != manifest["synthetic_users_db"]["sha256"]:
        raise SystemExit("Synthetic users.db hash mismatch; regenerate benchmarks")
    for c in manifest["cases"]:
        p = generated_dir / "data" / c["input_file"]
        if not p.exists() or sha256_file(p) != c["sha256"]:
            raise SystemExit(f"Generated input hash mismatch: {p}")
    return manifest


def tail_lines(path: Path, n: int) -> list[str]:
    # Outputs are only a few MB even for the largest profile. Read once for reliable
    # exact-tail sentinel comparison and simplicity.
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    return lines[-n:] if n else []


def validate_case(repo: Path, generated_dir: Path, case: dict) -> dict:
    exe = (repo / case["executable"]).resolve()
    input_path = generated_dir / "data" / case["input_file"]
    data_dir = generated_dir / "data"
    with tempfile.TemporaryDirectory(prefix="socialbench_validate_") as td:
        out_path = Path(td) / "stdout.txt"
        err_path = Path(td) / "stderr.txt"
        start = time.perf_counter()
        try:
            with input_path.open("rb") as fin, out_path.open("wb") as fout, err_path.open("wb") as ferr:
                p = subprocess.run([str(exe)], cwd=data_dir, stdin=fin, stdout=fout, stderr=ferr,
                                   timeout=int(case.get("timeout_s", 120)))
            wall = time.perf_counter() - start
        except subprocess.TimeoutExpired:
            return {"passed": False, "timeout": True, "wall_s": time.perf_counter() - start}

        stderr = err_path.read_text(encoding="utf-8", errors="replace")[-8000:]
        problems: list[str] = []
        if p.returncode != 0:
            problems.append(f"returncode={p.returncode}")
        if stderr.strip():
            # stderr is unexpected in normal generated cases; record it and fail validation.
            problems.append("non-empty stderr")

        contains = case.get("expected_contains") or []
        if contains:
            text = out_path.read_text(encoding="utf-8", errors="replace")
            for marker in contains:
                if marker not in text:
                    problems.append(f"missing marker: {marker!r}")

        expected_tail = case.get("expected_tail") or []
        actual_tail = tail_lines(out_path, len(expected_tail)) if expected_tail else []
        if expected_tail and actual_tail != expected_tail:
            problems.append("tail sentinel mismatch")

        return {
            "passed": not problems,
            "returncode": p.returncode,
            "wall_s": wall,
            "stdout_bytes": out_path.stat().st_size,
            "stderr": stderr,
            "problems": problems,
            "expected_tail": expected_tail,
            "actual_tail": actual_tail,
        }


def percentile(values: list[float], p: float) -> float | None:
    if not values:
        return None
    xs = sorted(values)
    if len(xs) == 1:
        return xs[0]
    rank = (len(xs) - 1) * p
    lo = math.floor(rank)
    hi = math.ceil(rank)
    if lo == hi:
        return xs[lo]
    frac = rank - lo
    return xs[lo] * (1 - frac) + xs[hi] * frac


def timed_once(exe: Path, input_path: Path, cwd: Path, timeout_s: int) -> dict:
    gnu_time = Path("/usr/bin/time")
    time_file = None
    try:
        with tempfile.NamedTemporaryFile(prefix="socialbench_time_", delete=False) as tf:
            time_file = Path(tf.name)
        if gnu_time.exists():
            cmd = [str(gnu_time), "-f", "%e\t%U\t%S\t%M", "-o", str(time_file), str(exe)]
        else:
            cmd = [str(exe)]

        start = time.perf_counter()
        with input_path.open("rb") as fin, open(os.devnull, "wb") as nullout:
            try:
                p = subprocess.run(cmd, cwd=cwd, stdin=fin, stdout=nullout, stderr=subprocess.PIPE,
                                   timeout=timeout_s)
                wall_py = time.perf_counter() - start
            except subprocess.TimeoutExpired:
                return {"ok": False, "timeout": True, "wall_s": time.perf_counter() - start}

        result: dict[str, Any] = {
            "ok": p.returncode == 0 and not p.stderr.strip(),
            "returncode": p.returncode,
            "wall_s": wall_py,
            "stderr": p.stderr.decode("utf-8", errors="replace")[-4000:],
            "user_s": None, "sys_s": None, "peak_rss_mb": None,
        }
        if gnu_time.exists() and time_file and time_file.exists():
            raw = time_file.read_text().strip().split("\t")
            if len(raw) == 4:
                try:
                    # GNU time %e can have coarse precision. Keep perf_counter for wall;
                    # use GNU time only for CPU and peak RSS.
                    result["user_s"] = float(raw[1])
                    result["sys_s"] = float(raw[2])
                    result["peak_rss_mb"] = float(raw[3]) / 1024.0
                except ValueError:
                    pass
        return result
    finally:
        if time_file:
            time_file.unlink(missing_ok=True)


def benchmark_case(repo: Path, generated_dir: Path, case: dict, repeats: int) -> dict:
    validation = validate_case(repo, generated_dir, case)
    result: dict[str, Any] = {"case": case, "validation": validation, "timing": None}
    if not validation.get("passed"):
        result["status"] = "invalid"
        return result

    exe = (repo / case["executable"]).resolve()
    inp = generated_dir / "data" / case["input_file"]
    runs = [timed_once(exe, inp, generated_dir / "data", int(case.get("timeout_s", 120))) for _ in range(repeats)]
    good = [r for r in runs if r.get("ok")]
    walls = [float(r["wall_s"]) for r in good]
    rss = [float(r["peak_rss_mb"]) for r in good if r.get("peak_rss_mb") is not None]
    operations = int(case.get("operations", 0))
    med = statistics.median(walls) if walls else None
    result["timing"] = {
        "repeats_requested": repeats,
        "successful_runs": len(good),
        "runs": runs,
        "median_wall_s": med,
        "min_wall_s": min(walls) if walls else None,
        "p95_wall_s": percentile(walls, 0.95),
        "median_ops_per_s": (operations / med) if med and med > 0 else None,
        "peak_rss_mb_max": max(rss) if rss else None,
    }
    result["status"] = "passed" if len(good) == repeats else "timing_failed"
    return result


def pick_memory_cases(cases: list[dict]) -> list[dict]:
    # One bounded representative per major subsystem. Avoid huge Bron-Kerbosch cases
    # under Valgrind/sanitizers because instrumentation changes exponential runtimes.
    selected = []
    for cat in ["graph", "content", "mixed", "clique", "repost_chain"]:
        candidates = [c for c in cases if c["category"] == cat]
        if candidates:
            candidates.sort(key=lambda x: x.get("operations", 0))
            selected.append(candidates[0])
    return selected


def run_valgrind(repo: Path, generated_dir: Path, cases: list[dict]) -> dict:
    if not shutil.which("valgrind"):
        return {"status": "skipped", "reason": "valgrind not installed", "tests": []}
    tests = []
    for case in pick_memory_cases(cases):
        exe = (repo / case["executable"]).resolve()
        inp = generated_dir / "data" / case["input_file"]
        cmd = [
            "valgrind", "--leak-check=full", "--show-leak-kinds=all",
            "--errors-for-leak-kinds=all", "--track-origins=yes", "--error-exitcode=99",
            str(exe),
        ]
        start = time.perf_counter()
        try:
            with inp.open("rb") as fin, open(os.devnull, "wb") as nullout:
                p = subprocess.run(cmd, cwd=generated_dir / "data", stdin=fin, stdout=nullout,
                                   stderr=subprocess.PIPE, timeout=min(int(case.get("timeout_s", 120)) * 8, 1200))
            stderr = p.stderr.decode("utf-8", errors="replace")
            err_m = re.search(r"ERROR SUMMARY:\s*(\d+) errors", stderr)
            lost_m = re.search(r"definitely lost:\s*([\d,]+) bytes", stderr)
            error_count = int(err_m.group(1)) if err_m else None
            definitely_lost = int(lost_m.group(1).replace(",", "")) if lost_m else None
            clean = p.returncode == 0 and error_count == 0 and (definitely_lost in (None, 0))
            tests.append({
                "case": case["name"], "returncode": p.returncode, "clean": clean,
                "error_count": error_count, "definitely_lost_bytes": definitely_lost,
                "wall_s": time.perf_counter() - start,
                "stderr_tail": stderr.splitlines()[-30:],
            })
        except subprocess.TimeoutExpired:
            tests.append({"case": case["name"], "clean": False, "timeout": True,
                          "wall_s": time.perf_counter() - start})
    return {"status": "passed" if tests and all(t.get("clean") for t in tests) else "failed", "tests": tests}


def run_sanitizers(repo: Path, generated_dir: Path, cases: list[dict]) -> dict:
    flags = "-Wall -Wextra -Werror -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer"
    build_result = build(repo, flags)
    if not build_result["ok"]:
        return {"status": "build_failed", "build": build_result, "tests": []}

    env = os.environ.copy()
    env.update({
        "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1:abort_on_error=1",
        "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
    })
    tests = []
    for case in pick_memory_cases(cases):
        exe = (repo / case["executable"]).resolve()
        inp = generated_dir / "data" / case["input_file"]
        try:
            with inp.open("rb") as fin, open(os.devnull, "wb") as nullout:
                p = subprocess.run([str(exe)], cwd=generated_dir / "data", stdin=fin,
                                   stdout=nullout, stderr=subprocess.PIPE, env=env,
                                   timeout=min(int(case.get("timeout_s", 120)) * 3, 600))
            stderr = p.stderr.decode("utf-8", errors="replace")
            clean = p.returncode == 0 and "ERROR: AddressSanitizer" not in stderr and "runtime error:" not in stderr
            tests.append({"case": case["name"], "returncode": p.returncode,
                          "clean": clean, "stderr_tail": stderr.splitlines()[-30:]})
        except subprocess.TimeoutExpired:
            tests.append({"case": case["name"], "clean": False, "timeout": True})
    return {"status": "passed" if tests and all(t.get("clean") for t in tests) else "failed",
            "build": build_result, "tests": tests}


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    script_dir = Path(__file__).resolve().parent
    ap.add_argument("--repo", type=Path, default=script_dir.parent,
                    help="Social-Media-Network root (default assumes benchmarks/ is inside repo)")
    ap.add_argument("--profile", choices=["quick", "cv", "scale10k", "max"], default="cv")
    ap.add_argument("--repeats", type=int, default=5)
    ap.add_argument("--generated-dir", type=Path, default=script_dir / "generated")
    ap.add_argument("--output", type=Path, default=script_dir / "results" / "benchmark_results.json")
    ap.add_argument("--no-regression", action="store_true")
    ap.add_argument("--no-valgrind", action="store_true")
    ap.add_argument("--no-sanitizers", action="store_true")
    args = ap.parse_args()

    if args.repeats < 1:
        raise SystemExit("--repeats must be >= 1")
    repo = args.repo.resolve()
    generated_dir = args.generated_dir.resolve()
    output = args.output.resolve()

    # Hard preflight. Fail before creating misleading result files.
    repo_contract = inspect_repo(repo)
    if not shutil.which("make") or not shutil.which("gcc"):
        raise SystemExit("Required tools missing: gcc and make must be installed")

    print(f"[1/7] Generating deterministic {args.profile!r} workloads...")
    generate(repo, generated_dir, args.profile)
    manifest = verify_manifest(generated_dir / "manifest.json", generated_dir, repo)

    print("[2/7] Building normal binaries...")
    normal_build = build(repo)
    if not normal_build["ok"]:
        print(normal_build["output"], file=sys.stderr)
        raise SystemExit("Normal build failed; no benchmark results are valid")

    results: dict[str, Any] = {
        "schema_version": SCHEMA_VERSION,
        "complete": False,
        "metadata": {
            "timestamp_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "profile": args.profile,
            "repeats": args.repeats,
            "repo": str(repo),
            "git_commit": git_commit(repo),
            "platform": platform.platform(),
            "python": sys.version,
            "gcc": capture_text(["gcc", "--version"], repo),
            "cpu_model": cpu_model(),
            "repo_contract": repo_contract,
            "manifest_sha256": sha256_file(generated_dir / "manifest.json"),
        },
        "build": normal_build,
        "regression": None,
        "stress": [],
        "valgrind": None,
        "sanitizers": None,
        "restore_build": None,
    }
    atomic_json(output, results)

    print("[3/7] Running existing regression suite...")
    results["regression"] = ({"status": "skipped", "reason": "disabled"}
                             if args.no_regression else run_regressions(repo))
    atomic_json(output, results)

    cases = manifest["cases"]
    print(f"[4/7] Validating + timing {len(cases)} generated workloads ({args.repeats} repeats each)...")
    for i, case in enumerate(cases, 1):
        print(f"  [{i:02d}/{len(cases):02d}] {case['name']}")
        r = benchmark_case(repo, generated_dir, case, args.repeats)
        results["stress"].append(r)
        atomic_json(output, results)
        status = r["status"]
        timing = r.get("timing") or {}
        med = timing.get("median_wall_s")
        if med is not None:
            print(f"       {status}: median={med:.4f}s, ops/s={timing.get('median_ops_per_s', 0):,.0f}")
        else:
            print(f"       {status}")

    # Only memory-test cases whose sentinel validation passed.
    valid_cases = [r["case"] for r in results["stress"] if r.get("validation", {}).get("passed")]

    print("[5/7] Running Valgrind representative checks...")
    results["valgrind"] = ({"status": "skipped", "reason": "disabled", "tests": []}
                           if args.no_valgrind else run_valgrind(repo, generated_dir, valid_cases))
    atomic_json(output, results)

    print("[6/7] Running ASan/UBSan representative checks...")
    sanitizer_built = False
    try:
        if args.no_sanitizers:
            results["sanitizers"] = {"status": "skipped", "reason": "disabled", "tests": []}
        else:
            sanitizer_built = True
            results["sanitizers"] = run_sanitizers(repo, generated_dir, valid_cases)
        atomic_json(output, results)
    finally:
        # Sanitizer build modifies only build products, not source. Always restore normal
        # executables so the repository is left in its ordinary state.
        if sanitizer_built:
            print("[7/7] Restoring normal build...")
            results["restore_build"] = build(repo)
        else:
            results["restore_build"] = {"ok": True, "not_needed": True}
        results["complete"] = True
        atomic_json(output, results)

    print(f"Done. Raw evidence: {output}")


if __name__ == "__main__":
    main()
