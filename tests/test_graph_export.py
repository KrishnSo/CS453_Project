import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class TestGraphExport(unittest.TestCase):
    def test_synthetic_export_connected_and_seed(self):
        out = ROOT / "outputs/test_tmp_graph.json"
        subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools/graph_export/export_graph.py"),
                "--synthetic",
                "--nodes",
                "8",
                "--extra-edges",
                "5",
                "--seed",
                "999",
                "--out",
                str(out),
            ],
            check=True,
        )
        g = json.loads(out.read_text())
        self.assertEqual(g["seed"], 999)
        self.assertEqual(g["num_nodes"], 8)
        self.assertTrue(all(e["w"] > 0 for e in g["edges"]))
        out.unlink(missing_ok=True)

    def test_netgamesim_import_remaps_ids(self):
        raw = ROOT / "outputs/raw/raw_netgamesim_graph.json"
        out = ROOT / "outputs/test_tmp_ngs.json"
        subprocess.run(
            [
                sys.executable,
                str(ROOT / "tools/graph_export/export_graph.py"),
                "--raw-netgamesim",
                str(raw),
                "--seed",
                "1",
                "--out",
                str(out),
            ],
            check=True,
        )
        g = json.loads(out.read_text())
        self.assertEqual(g["source_format"], "netgamesim_json")
        self.assertEqual(g["num_nodes"], 4)
        ids = {n["id"] for n in g["nodes"]}
        self.assertEqual(ids, set(range(4)))
        out.unlink(missing_ok=True)


if __name__ == "__main__":
    unittest.main()
