#!/bin/bash
set -euo pipefail

GRAPH_FILE="$1"
shift

python3 tools/partition/partition.py "$GRAPH_FILE" "$@"