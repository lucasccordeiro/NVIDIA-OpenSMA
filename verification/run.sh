#!/usr/bin/env bash
# Thin wrapper. Use:  ./run.sh mctp_packet     (language-level)
#                     ./run.sh mctp_packet_func (functional / k-induction)
set -euo pipefail
cd "$(dirname "$0")"
make "${1:-mctp_packet}"
