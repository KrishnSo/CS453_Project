#!/bin/bash
# Usage:
#   ./tools/graph_export/run.sh <config.json> <out.json> [synthetic|import]
#
# synthetic (default): reads nodes, extra_edges, seed from config JSON.
# import: reads seed from config; uses raw_input from config or RAW_JSON env
#         (default outputs/raw/raw_netgamesim_graph.json).
set -euo pipefail

CONFIG_FILE="$1"
OUT_FILE="$2"
MODE="${3:-synthetic}"

if [ "$MODE" = "import" ]; then
  RAW_INPUT="${RAW_JSON:-$(python3 -c "import json; print(json.load(open('$CONFIG_FILE')).get('raw_input','outputs/raw/raw_netgamesim_graph.json'))")}"
  SEED="$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['seed'])")"
  python3 tools/graph_export/export_graph.py \
    --raw-netgamesim "$RAW_INPUT" \
    --seed "$SEED" \
    --out "$OUT_FILE"
else
  python3 tools/graph_export/export_graph.py \
    --synthetic \
    --nodes "$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['nodes'])")" \
    --extra-edges "$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['extra_edges'])")" \
    --seed "$(python3 -c "import json; print(json.load(open('$CONFIG_FILE'))['seed'])")" \
    --out "$OUT_FILE"
fi
