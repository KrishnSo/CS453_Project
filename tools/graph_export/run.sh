#!/bin/bash
set -euo pipefail

CONFIG_FILE="$1"
OUT_FILE="$2"

python3 tools/graph_export/export_graph.py \
  --synthetic \
  --nodes "$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['nodes'])")" \
  --extra-edges "$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['extra_edges'])")" \
  --seed "$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['seed'])")" \
  --out "$OUT_FILE"