"""Integration smoke test: MPI leader + Dijkstra (skipped if mpirun fails)."""
import os
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _have_mpirun():
    try:
        subprocess.run(["mpirun", "--version"], capture_output=True, check=True)
        return True
    except (OSError, subprocess.CalledProcessError):
        return False


@unittest.skipUnless(_have_mpirun(), "mpirun not available")
class TestMpiSmoke(unittest.TestCase):
    def test_small_graph_leader_and_dijkstra(self):
        exe = ROOT / "build/ngs_mpi"
        if not exe.is_file():
            self.skipTest("build/ngs_mpi missing; run cmake --build build")
        graph = ROOT / "outputs/graphs/graph_small.json"
        part = ROOT / "outputs/partitions/part_small_r4_mod.json"
        env = os.environ.copy()
        env.setdefault("OMPI_MCA_btl", "tcp,self")
        r = subprocess.run(
            [
                "mpirun",
                "-n",
                "4",
                str(exe),
                "--graph",
                str(graph),
                "--part",
                str(part),
                "--algo",
                "both",
                "--source",
                "0",
            ],
            capture_output=True,
            text=True,
            cwd=str(ROOT),
            env=env,
            timeout=120,
        )
        self.assertEqual(r.returncode, 0, msg=r.stderr + r.stdout)
        self.assertIn("Leader: 7", r.stdout)
        self.assertIn("0 -> 0", r.stdout)


if __name__ == "__main__":
    unittest.main()
