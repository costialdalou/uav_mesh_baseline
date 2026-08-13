# UAV Mesh Baseline

An OMNeT++/INET simulation of six mobile UAVs communicating with a wired Ground Control Station (GCS) through an AODV wireless mesh gateway.

The scenario models normal operation only. It is intended to generate baseline networking data for routing analysis, anomaly detection, cybersecurity experiments, and graph-based machine-learning datasets.

## Requirements

- OMNeT++ 6.4.0
- INET 4.7.0
- A C++ toolchain supported by OMNeT++

The project expects this directory layout:

```text
costi_omnet/
├── inet-4.7.0/
├── omnetpp-6.4.0/
└── uav_mesh_baseline/
```

## Scenario

The network contains eight nodes:

| Node | Role | Interfaces | Address |
|---|---|---|---|
| `gcs` | Ground Control Station | Ethernet | `10.0.0.1/30` |
| `gateway` | Mesh-to-Ethernet gateway | Ethernet and Wi-Fi | `10.0.0.2/30`, `10.1.0.1/24` |
| `uav1`–`uav6` | Mobile AODV routers | Wi-Fi | `10.1.0.11`–`10.1.0.16` |

The GCS and gateway are connected by a 100 Mbps Ethernet link. UAV-to-UAV and UAV-to-gateway connectivity is determined dynamically by node positions, radio propagation, and receiver sensitivity. No wireless links are explicitly created in NED.

The UAVs follow a deterministic 3D BonnMotion trace for 420 seconds. Their movement changes radio connectivity and causes AODV to discover and replace multi-hop routes.

### Traffic

| Direction | Name | Rate per UAV | Payload | UDP port | Start |
|---|---|---:|---:|---:|---:|
| UAV → GCS | `UavTelemetry` | 10 packets/s | 200 B | 5000 | 30 s |
| GCS → UAV | `GcsCommand` | 2 packets/s | 100 B | 5001 | 30 s |

These are generic UDP payloads representing traffic patterns. They do not contain MAVLink fields or real command/telemetry content.

### Radio configuration

- IEEE 802.11g ERP at 48 Mbps
- 2.412 GHz carrier frequency
- 100 mW transmit power
- Isotropic antennas
- −74 dBm receiver sensitivity
- Free-space path loss
- −96 dBm background noise

The resulting communication range is approximately 496 m. Some UAV regions are outside direct gateway range, forcing multi-hop forwarding through other UAVs.

## Project structure

```text
uav_mesh_baseline/
├── src/
│   ├── GatewayAwareAodv.cc
│   ├── GatewayAwareAodv.h
│   ├── UavStatsCollector.cc
│   └── UavStatsCollector.h
├── simulations/
│   ├── mobility/uav_mobility.trace
│   ├── GatewayAwareAodv.ned
│   ├── GatewayAwareAodvRouter.ned
│   ├── UavMeshBaseline.ned
│   ├── UavStatsCollector.ned
│   ├── omnetpp.ini
│   └── results/
├── Makefile
└── package.ned
```

`UavStatsCollector` observes each UAV and the gateway without changing packet handling. It records AODV control messages and forwarded experiment data.

`GatewayAwareAodv` is used only by the gateway. It prevents INET AODV from generating a false RERR when the gateway forwards a packet through a valid non-AODV Ethernet route to the GCS. The shared INET source remains unchanged, and the UAVs continue to use INET's standard AODV module.

## Build

### Command line

Enter the configured OMNeT++/INET environment, then build the project:

```bash
cd ~/costi_omnet
opp_env shell
cd uav_mesh_baseline
make MODE=release -j4
```

The release library is generated at:

```text
out/clang-release/libuav_mesh_baseline.so
```

### OMNeT++ IDE

1. Import `inet-4.7.0` and `uav_mesh_baseline` into the workspace.
2. Ensure both projects use compatible build modes, such as release.
3. Build `inet-4.7.0` if it is not already built.
4. Select `uav_mesh_baseline`.
5. Choose **Project → Build Project**.

## Run

### Headless data collection

From the OMNeT++/INET environment:

```bash
cd ~/costi_omnet/uav_mesh_baseline/simulations

opp_run -u Cmdenv \
  -n ..:$INET_ROOT/src \
  -l $INET_ROOT/src/INET \
  -l ../out/clang-release/uav_mesh_baseline \
  -c Baseline \
  omnetpp.ini
```

This runs the full 420-second simulation without opening the graphical runtime.

### Graphical runtime

Replace `Cmdenv` with `Qtenv`:

```bash
opp_run -u Qtenv \
  -n ..:$INET_ROOT/src \
  -l $INET_ROOT/src/INET \
  -l ../out/clang-release/uav_mesh_baseline \
  -c Baseline \
  omnetpp.ini
```

Available configurations:

- `Baseline`: top-down view.
- `BaselineIsometric`: isometric view showing altitude separation.

In the IDE, right-click `simulations/omnetpp.ini`, choose **Run As → OMNeT++ Simulation**, and select the desired configuration. The working directory must be `uav_mesh_baseline/simulations`.

## Recorded data

Results are written to `simulations/results/`:

| File | Purpose |
|---|---|
| `Baseline-#0.sca` | Final scalar values and counts |
| `Baseline-#0.vec` | Timestamped vector samples |
| `Baseline-#0.vci` | Index for the vector file |
| `Baseline-#0.rt` | Timestamped routing-table changes |

The project records:

- Application packets sent and received
- Application end-to-end delay vectors provided by INET
- Wireless MAC activity
- Minimum SNIR values
- AODV RREQ sent and received
- AODV RREP sent and received
- AODV RERR sent and received
- Experiment data packets forwarded by every UAV and the gateway

Measurements are event-driven, not sampled every second or millisecond. A vector entry is written when the corresponding event occurs. Mobility state is updated every 0.1 seconds according to `omnetpp.ini`, while the original trajectory is stored in the mobility trace.

### Vector-file format

A vector declaration maps a generated numeric ID to a signal:

```text
vector <id> UavMeshBaseline.statsGateway rerrSent:vector ETV
```

A measurement line contains:

```text
vector-id    event-number    simulation-time    value
```

OMNeT++ separates these fields with tabs. Vector IDs may change between runs, so find the ID by signal name instead of assuming a fixed number:

```bash
grep '^vector ' results/*.vec | grep 'statsGateway.*rerr'
grep '^<id>[[:space:]]' results/*.vec
```

Replace `<id>` with the number shown in the declaration.

Scalar routing statistics can be inspected with:

```bash
grep -E 'rreqSent|rreqReceived|rrepSent|rrepReceived|rerrSent|rerrReceived|dataForwarded' results/*.sca
```

## Reproducibility

- The mobility trace is deterministic.
- The simulation uses a fixed seed set unless overridden.
- Traffic start times are staggered to avoid identical periodic send times.
- Mobility and MAC operations use separate random-number streams.
- Generated binaries, vector data, and vector indexes are excluded from Git.

Large `.vec` files should be published through Git LFS or a research-data repository rather than normal GitHub storage.
