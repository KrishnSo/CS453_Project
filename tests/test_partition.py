import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class TestPartition(unittest.TestCase):
    def test_partition_mod_covers_all_nodes(self):
        gpath = ROOT / "outputs/graphs/graph_small.json"
        out = ROOT / "outputs/test_tmp_part.json"
        subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools/partition/partition.py"),
                str(gpath),
                "--ranks",
                "4",
                "--strategy",
                "mod",
                "--out",
                str(out),
            ],
            check=True,
        )
        g = json.loads(gpath.read_text())
        p = json.loads(out.read_text())
        n = g["num_nodes"]
        owners = {int(k): v for k, v in p["owner"].items()}
        self.assertEqual(len(owners), n)
        self.assertEqual(set(owners.keys()), set(range(n)))
        out.unlink(missing_ok=True)

    def test_partition_block_has_ghost_metadata(self):
        gpath = ROOT / "outputs/graphs/graph_small.json"
        out = ROOT / "outputs/test_tmp_part_b.json"
        subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools/partition/partition.py"),
                str(gpath),
                "--ranks",
                "4",
                "--strategy",
                "block",
                "--out",
                str(out),
            ],
            check=True,
        )
        p = json.loads(out.read_text())
        self.assertIn("ghost_nodes", p)
        self.assertGreaterEqual(p["cut_edges"], 0)
        out.unlink(missing_ok=True)


if __name__ == "__main__":
    unittest.main()
