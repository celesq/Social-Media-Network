#!/usr/bin/env python3
"""Deterministic stress-workload generator for celesq/Social-Media-Network.

The generator deliberately does not modify the project's own users.db.  It creates
an isolated synthetic users.db next to the generated workload files; benchmark
runs execute with that directory as CWD because the application loads users.db
relative to the working directory.
"""
from __future__ import annotations

import argparse
import hashlib
import itertools
import json
import math
import random
import re
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, Sequence

SEED = 20260901


@dataclass
class Case:
    name: str
    category: str
    executable: str
    input_file: str
    users: int = 0
    friendships: int = 0
    posts: int = 0
    reposts: int = 0
    likes: int = 0
    queries: int = 0
    operations: int = 0
    expected_contains: list[str] | None = None
    expected_tail: list[str] | None = None
    timeout_s: int = 120
    notes: str = ""


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except FileNotFoundError as exc:
        raise SystemExit(f"Required source file missing: {path}") from exc


def parse_define(text: str, name: str) -> int:
    m = re.search(rf"^\s*#\s*define\s+{re.escape(name)}\s+(\d+)\b", text, re.MULTILINE)
    if not m:
        raise SystemExit(f"Could not find numeric #define {name}")
    return int(m.group(1))


def inspect_repo(repo: Path) -> dict:
    friends_h = read_text(repo / "friends.h")
    social = read_text(repo / "social_media.c")
    makefile = read_text(repo / "Makefile")
    users_c = read_text(repo / "users.c")

    max_people = parse_define(friends_h, "MAX_PEOPLE")
    max_command_len = parse_define(friends_h, "MAX_COMMAND_LEN")

    required_fragments = {
        "social_media.c": ["init_users();", "lg_create(MAX_PEOPLE)", "fgets(input, MAX_COMMAND_LEN, stdin)"],
        "Makefile": ["build: friends posts feed", "-D TASK_1 -D TASK_2 -D TASK_3"],
        "users.c": ['static const char *db_path = "users.db"', "fscanf(users_db, \"%hu\", &users_number)"],
    }
    source_map = {"social_media.c": social, "Makefile": makefile, "users.c": users_c}
    for filename, fragments in required_fragments.items():
        for fragment in fragments:
            if fragment not in source_map[filename]:
                raise SystemExit(
                    f"Repository layout/source contract changed: {filename} no longer contains {fragment!r}. "
                    "Review benchmark assumptions before running."
                )

    if max_people < 2:
        raise SystemExit(f"MAX_PEOPLE={max_people} is invalid")
    if max_command_len < 64:
        raise SystemExit(f"MAX_COMMAND_LEN={max_command_len} is unexpectedly small")

    return {
        "max_people": max_people,
        "max_command_len": max_command_len,
        "source_sha256": {
            name: sha256_file(repo / name)
            for name in ["friends.h", "friends.c", "posts.c", "feed.c", "users.c", "social_media.c", "Makefile"]
            if (repo / name).exists()
        },
    }


def uname(i: int) -> str:
    # users.c reads usernames into char temp[32], so keep names comfortably below 31 chars.
    return f"bench{i:03d}"


def write_users_db(path: Path, n: int) -> None:
    path.write_text(str(n) + "\n" + "\n".join(uname(i) for i in range(n)) + "\n", encoding="utf-8")


def all_pairs(n: int) -> list[tuple[int, int]]:
    return list(itertools.combinations(range(n), 2))


def choose_edges(n: int, edge_count: int, seed: int) -> list[tuple[int, int]]:
    """Choose unique undirected edges in O(edge_count) expected memory/time.

    For sparse/moderately dense graphs, sample pairs directly without materializing
    all n*(n-1)/2 combinations. For dense graphs, fall back to streaming
    combinations so memory remains O(edge_count), not O(n^2).
    """
    max_edges = n * (n - 1) // 2
    if edge_count < 0 or edge_count > max_edges:
        raise ValueError(
            f"Cannot choose {edge_count} unique undirected edges for {n} users (max {max_edges})"
        )

    chosen: list[tuple[int, int]] = []
    chosen_set: set[tuple[int, int]] = set()

    # Seed with a ring whenever possible so distance/suggestion workloads are not
    # accidentally disconnected.
    if edge_count >= n and n > 2:
        for i in range(n):
            e = (i, i + 1) if i + 1 < n else (0, n - 1)
            if e not in chosen_set:
                chosen.append(e)
                chosen_set.add(e)
                if len(chosen) == edge_count:
                    return chosen

    remaining = edge_count - len(chosen)
    if remaining <= 0:
        return chosen

    rng = random.Random(seed)

    # Rejection sampling is efficient while the requested graph is well below
    # complete density. Crucially, it never constructs the full O(n^2) pair set.
    if edge_count <= max_edges // 3:
        while len(chosen) < edge_count:
            a = rng.randrange(n)
            b = rng.randrange(n - 1)
            if b >= a:
                b += 1
            if a > b:
                a, b = b, a
            e = (a, b)
            if e not in chosen_set:
                chosen_set.add(e)
                chosen.append(e)
    else:
        # Dense fallback: stream combinations rather than materializing them.
        # The ring is already present, so skip duplicates.
        for e in itertools.combinations(range(n), 2):
            if e in chosen_set:
                continue
            chosen_set.add(e)
            chosen.append(e)
            if len(chosen) == edge_count:
                break

    assert len(chosen) == edge_count
    assert len(chosen_set) == edge_count
    assert all(a != b and 0 <= a < n and 0 <= b < n for a, b in chosen)
    return chosen



def iter_balanced_edges(n: int, edge_count: int):
    """Yield deterministic unique undirected edges with balanced degree growth.

    This is designed for large scalability sweeps. It uses a circulant construction
    and O(1) auxiliary memory, so 10K users / 1M edges does not require storing a
    million Python tuples or materializing the ~50M possible user pairs.
    """
    max_edges = n * (n - 1) // 2
    if n < 2:
        raise ValueError("n must be >= 2")
    if edge_count < 0 or edge_count > max_edges:
        raise ValueError(
            f"Cannot choose {edge_count} unique undirected edges for {n} users (max {max_edges})"
        )

    remaining = edge_count
    # For d < n/2, each offset contributes exactly n distinct undirected edges.
    # Restricting d to <= floor((n-1)/2) avoids the symmetric duplicate offset n-d.
    for d in range(1, (n - 1) // 2 + 1):
        take = min(remaining, n)
        for i in range(take):
            a = i
            b = (i + d) % n
            if a > b:
                a, b = b, a
            yield a, b
        remaining -= take
        if remaining == 0:
            return

    # Even n has one final opposite-vertex offset. Each undirected edge would
    # otherwise appear twice, so only i in [0, n/2) is emitted.
    if n % 2 == 0 and remaining:
        d = n // 2
        take = min(remaining, n // 2)
        for i in range(take):
            yield i, i + d
        remaining -= take

    if remaining != 0:
        raise AssertionError(f"Edge generator ended with {remaining} edges remaining")


def gen_scale_graph_case(data_dir: Path, n: int, e: int, max_command_len: int, timeout: int) -> Case:
    """Generate one large graph-ingestion case without retaining the workload in RAM."""
    if n < 2:
        raise ValueError("n must be >= 2")
    max_edges = n * (n - 1) // 2
    if e < 0 or e > max_edges:
        raise ValueError(f"Requested {e} edges for {n} users; maximum is {max_edges}")

    input_file = f"scale_graph_u{n}_e{e}.in"
    path = data_dir / input_file
    degree0 = 0
    with path.open("w", encoding="utf-8", buffering=1024 * 1024) as f:
        count = 0
        for a, b in iter_balanced_edges(n, e):
            line = f"add {uname(a)} {uname(b)}"
            if len(line.encode("utf-8")) + 1 >= max_command_len:
                raise ValueError(
                    f"Generated command exceeds MAX_COMMAND_LEN={max_command_len}: {line!r}"
                )
            f.write(line + "\n")
            degree0 += int(a == 0 or b == 0)
            count += 1
        if count != e:
            raise AssertionError(f"Generated {count} edges, expected {e}")
        sentinel = f"friends {uname(0)}"
        if len(sentinel.encode("utf-8")) + 1 >= max_command_len:
            raise ValueError("Sentinel command exceeds MAX_COMMAND_LEN")
        f.write(sentinel + "\n")

    return Case(
        name=f"scale_graph_u{n}_e{e}",
        category="graph",
        executable="friends",
        input_file=input_file,
        users=n,
        friendships=e,
        queries=1,
        operations=e + 1,
        expected_tail=[f"{uname(0)} has {degree0} friends"],
        timeout_s=timeout,
        notes=(
            "Deterministic balanced/circulant graph ingestion with unique undirected "
            "edges and an exact degree sentinel; generated in streaming mode."
        ),
    )

def edge_lines(edges: Sequence[tuple[int, int]]) -> list[str]:
    return [f"add {uname(a)} {uname(b)}" for a, b in edges]


def degree_of(edges: Sequence[tuple[int, int]], node: int) -> int:
    return sum(1 for a, b in edges if a == node or b == node)


def ensure_line_lengths(lines: Iterable[str], max_command_len: int) -> None:
    longest = max((len(line.encode("utf-8")) + 1 for line in lines), default=0)  # + newline
    if longest >= max_command_len:
        raise ValueError(
            f"Generated command is {longest} bytes including newline, but fgets buffer is MAX_COMMAND_LEN={max_command_len}"
        )


def write_case(data_dir: Path, case: Case, lines: list[str], max_command_len: int) -> Case:
    ensure_line_lengths(lines, max_command_len)
    path = data_dir / case.input_file
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    case.operations = len(lines)
    return case


def gen_graph_case(data_dir: Path, n: int, e: int, seed: int, max_command_len: int, timeout: int) -> Case:
    edges = choose_edges(n, e, seed)
    degree0 = degree_of(edges, 0)
    lines = edge_lines(edges)
    lines.append(f"friends {uname(0)}")
    case = Case(
        name=f"graph_u{n}_e{e}", category="graph", executable="friends",
        input_file=f"graph_u{n}_e{e}.in", users=n, friendships=e, queries=1,
        expected_tail=[f"{uname(0)} has {degree0} friends"], timeout_s=timeout,
        notes="Unique undirected friendship ingestion + degree sentinel.",
    )
    return write_case(data_dir, case, lines, max_command_len)


def gen_churn_case(data_dir: Path, n: int, base_e: int, cycles: int, seed: int,
                   max_command_len: int, timeout: int) -> Case:
    edges = choose_edges(n, base_e, seed)
    target = edges[0]
    lines = edge_lines(edges)
    for _ in range(cycles):
        lines.append(f"remove {uname(target[0])} {uname(target[1])}")
        lines.append(f"add {uname(target[0])} {uname(target[1])}")
    lines.append(f"friends {uname(0)}")
    degree0 = degree_of(edges, 0)
    case = Case(
        name=f"churn_u{n}_e{base_e}_c{cycles}", category="churn", executable="friends",
        input_file=f"churn_u{n}_e{base_e}_c{cycles}.in", users=n, friendships=base_e, queries=1,
        expected_tail=[f"{uname(0)} has {degree0} friends"], timeout_s=timeout,
        notes=f"{cycles} remove/add cycles after initial graph build.",
    )
    return write_case(data_dir, case, lines, max_command_len)


def gen_content_case(data_dir: Path, users: int, posts: int, reposts: int, likes: int,
                     max_command_len: int, timeout: int) -> Case:
    if likes > users * posts:
        raise ValueError("Requested likes exceed unique user/post pairs; duplicates would toggle likes off")
    lines: list[str] = []
    for i in range(posts):
        lines.append(f'create {uname(i % users)} "post_{i + 1}"')

    # IDs are global across originals and reposts. Since all originals are created first,
    # original IDs are 1..posts and generated repost IDs are posts+1...
    for i in range(reposts):
        pid = (i % posts) + 1
        lines.append(f"repost {uname((i + 1) % users)} {pid}")

    for i in range(likes):
        user_id = i % users
        pid = (i // users) + 1
        lines.append(f"like {uname(user_id)} {pid}")

    post1_likes = min(users, likes)
    lines.append("get-likes 1")
    case = Case(
        name=f"content_p{posts}_r{reposts}_l{likes}", category="content", executable="posts",
        input_file=f"content_p{posts}_r{reposts}_l{likes}.in", users=users,
        posts=posts, reposts=reposts, likes=likes, queries=1,
        expected_tail=[f'Post "post_1" has {post1_likes} likes'], timeout_s=timeout,
        notes="Dynamic post/repost growth + unique likes + correctness sentinel.",
    )
    return write_case(data_dir, case, lines, max_command_len)


def gen_mixed_case(data_dir: Path, n: int, e: int, posts: int, reposts: int, likes: int,
                   feed_queries: int, seed: int, max_command_len: int, timeout: int) -> Case:
    if likes > n * posts:
        raise ValueError("Requested likes exceed unique user/post pairs")
    edges = choose_edges(n, e, seed)
    lines = edge_lines(edges)
    for i in range(posts):
        lines.append(f'create {uname(i % n)} "mixed_{i + 1}"')
    for i in range(reposts):
        pid = (i % posts) + 1
        lines.append(f"repost {uname((i + 3) % n)} {pid}")
    for i in range(likes):
        uid = i % n
        pid = (i // n) + 1
        lines.append(f"like {uname(uid)} {pid}")
    for i in range(feed_queries):
        lines.append(f"feed {uname(i % n)} 1")

    degree0 = degree_of(edges, 0)
    post1_likes = min(n, likes)
    lines.append(f"friends {uname(0)}")
    lines.append("get-likes 1")
    case = Case(
        name=f"mixed_u{n}_e{e}_p{posts}_r{reposts}_l{likes}_q{feed_queries}",
        category="mixed", executable="feed",
        input_file=f"mixed_u{n}_e{e}_p{posts}_r{reposts}_l{likes}_q{feed_queries}.in",
        users=n, friendships=e, posts=posts, reposts=reposts, likes=likes,
        queries=feed_queries + 2,
        expected_tail=[f"{uname(0)} has {degree0} friends", f'Post "mixed_1" has {post1_likes} likes'],
        timeout_s=timeout,
        notes="Same-process graph + content + feed workload; suitable for combined CV scale claims.",
    )
    return write_case(data_dir, case, lines, max_command_len)


def gen_clique_case(data_dir: Path, k: int, max_command_len: int, timeout: int) -> Case:
    edges = list(itertools.combinations(range(k), 2))
    lines = edge_lines(edges)
    lines.append(f"common-group {uname(0)}")
    expected_tail = [f"The closest friend group of {uname(0)} is:"] + [uname(i) for i in range(k)]
    case = Case(
        name=f"clique_k{k}", category="clique", executable="feed",
        input_file=f"clique_k{k}.in", users=k, friendships=len(edges), queries=1,
        expected_tail=expected_tail, timeout_s=timeout,
        notes="Controlled complete subgraph for Bron-Kerbosch; intentionally isolated from large dense graph tests.",
    )
    return write_case(data_dir, case, lines, max_command_len)


def gen_chain_case(data_dir: Path, users: int, depth: int, max_command_len: int, timeout: int) -> Case:
    lines = [f'create {uname(0)} "chain_root"']
    if depth < 2:
        raise ValueError("chain depth must be >= 2")

    # First repost gets ID 2 and is directly under original post 1.
    lines.append(f"repost {uname(1 % users)} 1")
    last_id = 2
    for i in range(2, depth + 1):
        # New ID is last_id + 1; rid argument is parent repost ID.
        lines.append(f"repost {uname(i % users)} 1 {last_id}")
        last_id += 1

    rid1 = last_id
    rid2 = last_id - 1
    lines.append(f"common-repost 1 {rid1} {rid2}")
    lines.append("get-likes 1")
    case = Case(
        name=f"repost_chain_d{depth}", category="repost_chain", executable="posts",
        input_file=f"repost_chain_d{depth}.in", users=users, posts=1, reposts=depth, queries=2,
        expected_tail=[
            f"The first common repost of {rid1} and {rid2} is {rid2}",
            'Post "chain_root" has 0 likes',
        ],
        timeout_s=timeout,
        notes="Deep hierarchical repost chain + LCA sentinel.",
    )
    return write_case(data_dir, case, lines, max_command_len)


def profile_specs(profile: str, max_people: int) -> dict:
    if max_people < 100:
        raise SystemExit(f"MAX_PEOPLE={max_people}; benchmark profiles require at least 100 graph slots")

    u100 = min(100, max_people)
    u250 = min(250, max_people)
    u500 = min(500, max_people)
    umax = max_people

    if profile == "quick":
        return {
            "graph": [(u100, min(1_000, u100 * (u100 - 1) // 2))],
            "churn": [(u100, 500, 250)],
            "content": [(u100, 500, 500, 500)],
            "mixed": [(u100, 1_000, 500, 500, 500, 25)],
            "cliques": [6, 8],
            "chains": [100],
            "timeout": 60,
        }
    if profile == "cv":
        graph_specs = [
            (u100, min(2_000, u100 * (u100 - 1) // 2)),
            (u250, min(10_000, u250 * (u250 - 1) // 2)),
            (u500, min(50_000, u500 * (u500 - 1) // 2)),
            (umax, min(100_000, umax * (umax - 1) // 2)),
        ]
        return {
            "graph": graph_specs,
            "churn": [(u500, min(25_000, u500 * (u500 - 1) // 2), 5_000)],
            "content": [(u500, 5_000, 5_000, 5_000), (u500, 10_000, 10_000, 10_000)],
            "mixed": [(u500, min(25_000, u500 * (u500 - 1) // 2), 5_000, 5_000, 5_000, 250)],
            "cliques": [8, 10, 12, 14],
            "chains": [250, 1_000],
            "timeout": 300,
        }
    if profile == "scale10k":
        if max_people < 10_000:
            raise SystemExit(
                f"scale10k requires MAX_PEOPLE >= 10000, found {max_people}. "
                "Set #define MAX_PEOPLE 10000 (or higher) in friends.h and rebuild."
            )
        n = 10_000
        return {
            "scale_graph": [(n, 250_000), (n, 500_000), (n, 1_000_000)],
            "graph": [],
            "churn": [],
            "content": [],
            "mixed": [],
            "cliques": [],
            "chains": [],
            "timeout": 1_800,
        }
    if profile == "max":
        max_edges = umax * (umax - 1) // 2
        return {
            "graph": [
                (u250, min(25_000, u250 * (u250 - 1) // 2)),
                (u500, min(75_000, u500 * (u500 - 1) // 2)),
                (umax, min(125_000, max_edges)),
                (umax, max_edges),
            ],
            "churn": [(umax, min(50_000, max_edges), 10_000)],
            "content": [(umax, 20_000, 20_000, 20_000)],
            "mixed": [(umax, min(100_000, max_edges), 10_000, 10_000, 10_000, 500)],
            "cliques": [10, 12, 14, 16, 18],
            "chains": [1_000, 2_000],
            "timeout": 900,
        }
    raise SystemExit(f"Unknown profile: {profile}")


def generate(repo: Path, out: Path, profile: str) -> dict:
    repo = repo.resolve()
    out = out.resolve()
    info = inspect_repo(repo)
    data_dir = out / "data"
    data_dir.mkdir(parents=True, exist_ok=True)
    write_users_db(data_dir / "users.db", info["max_people"])

    specs = profile_specs(profile, info["max_people"])
    timeout = specs["timeout"]
    cases: list[Case] = []
    seed = SEED

    for n, e in specs.get("scale_graph", []):
        cases.append(gen_scale_graph_case(data_dir, n, e, info["max_command_len"], timeout))

    for n, e in specs["graph"]:
        cases.append(gen_graph_case(data_dir, n, e, seed, info["max_command_len"], timeout))
        seed += 1
    for n, e, cycles in specs["churn"]:
        cases.append(gen_churn_case(data_dir, n, e, cycles, seed, info["max_command_len"], timeout))
        seed += 1
    for users, posts, reposts, likes in specs["content"]:
        cases.append(gen_content_case(data_dir, users, posts, reposts, likes, info["max_command_len"], timeout))
    for n, e, posts, reposts, likes, q in specs["mixed"]:
        cases.append(gen_mixed_case(data_dir, n, e, posts, reposts, likes, q, seed,
                                    info["max_command_len"], timeout))
        seed += 1
    for k in specs["cliques"]:
        if k <= info["max_people"]:
            cases.append(gen_clique_case(data_dir, k, info["max_command_len"], min(timeout, 180)))
    for depth in specs["chains"]:
        cases.append(gen_chain_case(data_dir, min(info["max_people"], max(2, min(depth + 1, 100))), depth,
                                    info["max_command_len"], timeout))

    # Manifest includes file hashes so results can prove exactly what was executed.
    manifest = {
        "schema_version": 2,
        "profile": profile,
        "seed": SEED,
        "repo": str(repo),
        "repo_contract": info,
        "synthetic_users_db": {
            "count": info["max_people"],
            "file": "data/users.db",
            "sha256": sha256_file(data_dir / "users.db"),
        },
        "cases": [],
    }
    for case in cases:
        d = asdict(case)
        d["sha256"] = sha256_file(data_dir / case.input_file)
        d["bytes"] = (data_dir / case.input_file).stat().st_size
        manifest["cases"].append(d)

    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1],
                    help="Social-Media-Network repository root (default: parent of benchmarks/)")
    ap.add_argument("--output", type=Path, default=Path(__file__).resolve().parent / "generated",
                    help="Generated benchmark directory")
    ap.add_argument("--profile", choices=["quick", "cv", "scale10k", "max"], default="cv")
    args = ap.parse_args()
    manifest = generate(args.repo, args.output, args.profile)
    print(f"Generated {len(manifest['cases'])} deterministic cases in {args.output}")
    print(f"Synthetic users: {manifest['synthetic_users_db']['count']}")
    print(f"MAX graph capacity from source: {manifest['repo_contract']['max_people']}")


if __name__ == "__main__":
    main()
