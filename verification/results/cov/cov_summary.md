## Phase 4 — Coverage summary

| target | phase | metric | reached | total | ratio | eff. | unwind | flags | exit_uncov |
|---|---|---|---:|---:|---:|---:|---:|---|---:|
| `nsm_dcd_event_handlers` | p1 | k-path | 12 | 126 | 0.095 | 0.095 | 10 | LANG | 0 |
| `nsm_dcd_event_handlers` | p2 | k-path | 1 | 110 | 0.009 | 0.009 | k≤6 | FUNC | 0 |
| `soc_sma_filter` | p1 | branch-fn | 4 | 4 | 1.000 | 1.000 | 4 | LANG | 0 |
| `soc_sma_filter` | p2 | branch-fn | 4 | 4 | 1.000 | 1.000 | k≤6 | FUNC | 0 |
| `debug_telemetry_sma` | p1 | branch-fn | 4 | 4 | 1.000 | 1.000 | 4 | LANG | 0 |
| `debug_telemetry_sma` | p2 | branch-fn | 4 | 4 | 1.000 | 1.000 | k≤6 | FUNC | 0 |
| `pca9555` | p1 | k-path | 24 | 132 | 0.182 | 0.182 | 4 | LANG | 0 |
| `pca9555` | p2 | k-path | 24 | 132 | 0.182 | 0.182 | k≤6 | FUNC | 0 |
| `ntc_table` | p1 | k-path | 11 | 22 | 0.500 | 0.500 | 9 | LANG | 0 |
| `ntc_table` | p2 | k-path | 14 | 118 | 0.119 | 0.119 | k≤4 | FUNC | 0 |
| `mctp_validator` | p1 | k-path | 1 | 136 | 0.007 | 0.008 | 4 | LANG | 16 |
| `mctp_validator` | p2 | k-path | 3 | 138 | 0.022 | 0.025 | k≤16 | FUNC | 16 |
| `nsm_bitmask` | p1 | branch-fn | 11 | 11 | 1.000 | 1.000 | 4 | LANG | 0 |
| `nsm_bitmask` | p2 | branch-fn | 11 | 11 | 1.000 | 1.000 | k≤6 | FUNC | 0 |
| `mctp_packet` | p1 | branch-fn | 8 | 12 | 0.667 | 0.667 | 4 | LANG | 0 |
| `mctp_packet` | p2 | branch-fn | 10 | 12 | 0.833 | 0.833 | k≤6 | FUNC | 0 |
| `mctp_router` | p1 | k-path | 2 | 2 | 1.000 | 1.000 | 4 | LANG | 0 |
| `mctp_router` | p2 | k-path | 2 | 2 | 1.000 | 1.000 | k≤6 | FUNC | 0 |
| `nsm_type5_validate` | p1 | k-path | 14 | 22 | 0.636 | 0.636 | 4 | LANG | 0 |
| `nsm_type5_validate` | p2 | k-path | 14 | 22 | 0.636 | 0.636 | k≤6 | FUNC | 0 |
| `c2c_mailbox` | p1 | k-path | 2 | 4 | 0.500 | 0.500 | 4 | LANG | 0 |
| `c2c_mailbox` | p2 | n/a | 0 | 0 | 0.000 | 0.000 | — | — | 0 |
| `ssif_safety` | p1 | k-path | 23 | 342 | 0.067 | 0.067 | 36 | LANG | 0 |
| `ssif_safety` | p2 | n/a | 0 | 0 | 0.000 | 0.000 | — | — | 0 |

### p1 → p2 deltas (|Δratio| ≥ 0.30 noted)

| target | ratio_p1 | ratio_p2 | Δratio | note |
|---|---:|---:|---:|---|
| `nsm_dcd_event_handlers` | 0.095 | 0.009 | -0.086 |  |
| `soc_sma_filter` | 1.000 | 1.000 | +0.000 |  |
| `debug_telemetry_sma` | 1.000 | 1.000 | +0.000 |  |
| `pca9555` | 0.182 | 0.182 | +0.000 |  |
| `ntc_table` | 0.500 | 0.119 | -0.381 | k-induction collapsed paths |
| `mctp_validator` | 0.007 | 0.022 | +0.014 |  |
| `nsm_bitmask` | 1.000 | 1.000 | +0.000 |  |
| `mctp_packet` | 0.667 | 0.833 | +0.167 |  |
| `mctp_router` | 1.000 | 1.000 | +0.000 |  |
| `nsm_type5_validate` | 0.636 | 0.636 | +0.000 |  |

_p1-only modules (`c2c_mailbox`, `ssif_safety`) are omitted from the Δ table — no p2 target exists. See Phase 4 commentary for the per-module rationale._

### Per-function rollup (top uncovered)

- `nsm_dcd_event_handlers` (p1, k-path):
    - `main` — 12/126 (10%)
- `nsm_dcd_event_handlers` (p2, k-path):
    - `main` — 1/110 (1%)
- `pca9555` (p1, k-path):
    - `i2c_read` — 3/46 (7%)
    - `i2c_write` — 13/78 (17%)
- `pca9555` (p2, k-path):
    - `i2c_read` — 3/46 (7%)
    - `i2c_write` — 13/78 (17%)
- `ntc_table` (p1, k-path):
    - `ntc_resistance_to_temperature` — 5/14 (36%)
    - `voltage_to_resistance` — 4/6 (67%)
- `ntc_table` (p2, k-path):
    - `ntc_resistance_to_temperature` — 8/110 (7%)
    - `voltage_to_resistance` — 4/6 (67%)
- `mctp_validator` (p1, k-path):
    - `get_cur_eid` — 0/2 (0%)
    - `validate` — 1/134 (1%)
- `mctp_validator` (p2, k-path):
    - `get_cur_eid` — 0/2 (0%)
    - `validate` — 1/134 (1%)
- `mctp_packet` (p1, branch-fn):
    - `get_packet_interface` — 1/2 (50%)
    - `get_packet_length` — 1/2 (50%)
    - `set_packet_interface` — 1/2 (50%)
- `mctp_packet` (p2, branch-fn):
    - `set_packet_interface` — 1/2 (50%)
    - `set_packet_length` — 1/2 (50%)
- `nsm_type5_validate` (p1, k-path):
    - `validateFatalErrorInjectionPayload` — 6/14 (43%)
- `nsm_type5_validate` (p2, k-path):
    - `validateFatalErrorInjectionPayload` — 6/14 (43%)
- `c2c_mailbox` (p1, k-path):
    - `set_value` — 1/2 (50%)
    - `get_value` — 1/2 (50%)
- `ssif_safety` (p1, k-path):
    - `i2c_callback` — 0/2 (0%)
    - `smbus_block_write` — 0/70 (0%)
    - `smbus_block_read` — 0/78 (0%)
