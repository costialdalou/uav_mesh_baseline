# UAV Mesh Baseline — OMNeT++ / INET

Normal-operation baseline for UAV-mesh cybersecurity / GNN research.
**No attack is included.** The point of the model is that this chain is produced
by the simulation itself, not scripted anywhere:

```
UAV movement
   -> changing radio conditions
      -> changing wireless connectivity
         -> AODV route discovery / route changes
            -> changing multi-hop packet paths
               -> UDP communication between the UAVs and the GCS
```

Built and verified with **OMNeT++ 6.4.0** and **INET 4.7.0**.

---

## 1. The network

Eight nodes, declared in [UavMeshBaseline.ned](UavMeshBaseline.ned):

| Node | Type | Interfaces | Address | Moves |
|---|---|---|---|---|
| `gcs` | `StandardHost` | `eth0` | `10.0.0.1/30` | no |
| `gateway` | `AodvRouter` | `eth0`, `wlan0` | `10.0.0.2/30`, `10.1.0.1/24` | no |
| `uav1`..`uav6` | `AodvRouter` | `wlan0` | `10.1.0.11` .. `10.1.0.16` /24 | yes |

* `10.0.0.0/30` — fixed wired GCS↔gateway network.
* `10.1.0.0/24` — airborne IEEE 802.11 ad hoc network (no access point).
* The GCS is **not** part of the dynamic wireless graph; it reaches the airborne
  network only over the fixed wired link.

The `.ned` file contains exactly **one** connection — the Ethernet link between
the GCS and the gateway. Not a single wireless connection is declared, scheduled
or torn down anywhere in this project.

### How UAV traffic reaches the GCS

INET 4.7's `Aodv` module has a built-in external-gateway feature
(`Aodv.ned`, parameter `gatewayAddress`; implementation in `Aodv.cc`,
`ensureRouteForDatagram`). Each UAV classifies any destination outside its own
`wlan0` subnet as *external* and forwards it along an AODV-maintained
`0.0.0.0/0` route whose next hop is the current next hop towards
`10.1.0.1`. The gateway recognises itself as the gateway and hands the packet to
normal IPv4 forwarding, which sends it out `eth0`. In the reverse direction the
gateway simply performs an ordinary AODV route discovery for the target UAV.
INET ships the same pattern in `showcases/routing/aodvexternal`.

> **Do not remove the `10.1.0.0/24` netmask route from the wireless nodes.**
> `0.0.0.0/0` also matches airborne addresses. Without a more specific `/24`
> entry, a UAV forwarding a GCS→UAV packet selects the default route and sends
> the packet *back* to the gateway, which sends it out again — the packet loops
> until its TTL expires. This was measured: it inflated the run from 232 k to
> 1.75 M events over the first 60 s. The `/24` carries no `AodvRouteData`, so
> AODV's pre-routing hook still treats it as "no active route", buffers the
> packet and starts a route discovery — IPv4 never bypasses AODV.

---

## 2. Radio model

Commodity 2.4 GHz IEEE 802.11g hardware. Nothing is tuned to reach a target
range; the range is a *consequence* of the hardware parameters.

| Parameter | Value | Why |
|---|---|---|
| opMode / bitrate | `g(erp)` @ 48 Mbps | standard ERP-OFDM rate |
| Transmit power | 100 mW (20 dBm) | typical 2.4 GHz module / EIRP ceiling |
| Antenna | `IsotropicAntenna`, 0 dBi | small omni whip on an airframe |
| Receiver sensitivity | −74 dBm | typical 802.11g chipset spec at 48 Mbps |
| Energy detection | −85 dBm | carrier sense wider than the decode range |
| Background noise | −96 dBm | kTB over 20 MHz plus ~5 dB noise figure |
| Path loss | `FreeSpacePathLoss`, α = 2 | unobstructed air-to-air / air-to-ground |

```
free-space path-loss constant at 2.412 GHz .... 40.1 dB
link budget      20 dBm - (-74 dBm) ........... 94.0 dB
=> communication range ........................ 495.7 m
```

### What that produces

Distances between the gateway (`0, 0, 2`) and the six airborne regions, and
between the regions themselves:

```
  P4-P5 155   P5-P6 155   P2-P3 160   GW-P1 179   P1-P2 197   P1-P3 197
  P2-P4 202   P3-P6 202   P2-P5 244   P2-P6 298   P3-P4 298   P4-P6 300
  GW-P2 353   GW-P3 353   P1-P4 399   P1-P6 399   P1-P5 410
--------------------------------- 495.7 m ---------------------------------
  GW-P4 550   GW-P6 550   GW-P5 569
```

So **the gateway hears whoever occupies the P1/P2/P3 regions, and whoever
occupies P4/P5/P6 must relay through an airborne UAV.** All UAV-to-UAV pairs
are in range of each other, so the relay choice is genuinely free and AODV
picks it — which is what makes the routing graph interesting.

### Why the range must stay in [353 m, 550 m]

* **Below ~353 m** the gateway goes *completely deaf* for ~20 s at a time during
  phases 3/4/5: the P1 region is empty while two UAVs are in transit, and
  P2/P3 sit at 353 m. The scenario would lose end-to-end connectivity entirely.
* **Above ~569 m** the gateway reaches every region directly and the network
  becomes single-hop — no multi-hop behaviour to observe.

495.7 m sits mid-window with roughly 1 dB of margin on each side.

---

## 3. Mobility

Deterministic 3D BonnMotion trace, [mobility/uav_mobility.trace](mobility/uav_mobility.trace) —
six lines, one per UAV, format `t x y z t x y z …`, selected per node with
`nodeId`. Identical in every run.

Regions: `P1 (150,0)`, `P2 (330,−80)`, `P3 (330,80)`, `P4 (520,−150)`,
`P5 (560,0)`, `P6 (520,150)`, all at 100 m altitude.

| Phase | Window | Leaves P1 → | Arrives at P1 ← |
|---|---|---|---|
| 0 | 0–30 s | stationary at P1..P6 | |
| 1 | 30–90 s | UAV1 → P2 | UAV2 ← P2 |
| 2 | 90–150 s | UAV2 → P3 | UAV3 ← P3 |
| 3 | 150–210 s | UAV3 → P4 | UAV4 ← P4 |
| 4 | 210–270 s | UAV4 → P5 | UAV5 ← P5 |
| 5 | 270–330 s | UAV5 → P6 | UAV6 ← P6 |
| 6 | 330–390 s | UAV6 → P2 | UAV1 ← P2 |
| 7 | 390–420 s | stationary; final occupancy P1=UAV1 P2=UAV6 P3=UAV2 P4=UAV3 P5=UAV4 P6=UAV5 | |

The six swaps form a clean cyclic permutation: every region ends up occupied
exactly once.

### Altitude layering (collision avoidance)

Reciprocal flights would otherwise pass through the same point, so each phase
separates them vertically:

* parked UAVs cruise at **100 m**
* the UAV *leaving* P1 climbs to **120 m** for the crossing
* the UAV *arriving* at P1 descends to **80 m**
* both are back at 100 m at the phase boundary

Each 60 s phase `[T, T+60]` therefore uses four waypoints:

```
outbound:  (T, P1,100) (T+6, P1,120) (T+54, Pk,120) (T+60, Pk,100)
inbound:   (T, Pk,100) (T+6, Pk, 80) (T+54, P1, 80) (T+60, P1,100)
```

Measured over the whole trace at 1 ms resolution: **minimum UAV–UAV separation
21.03 m** (at t = 179.67 s, between UAV1 parked at P2 and UAV3 transiting the
P1→P4 leg, which passes 6.5 m from P2 horizontally but 20 m below it) and
**maximum speed 8.54 m/s**. Increase the ±20 m offsets if you want more room.

### Why the trace starts at t = 1, not t = 0

`BonnMotionMobility::setInitialPosition()` in INET 4.7 sets only *x* and *y* —
it ignores *z* (`BonnMotionMobility.cc`). The z coordinate is applied by the
first `move()`, so the first waypoint must have a timestamp greater than zero.
INET's own trace files start at t = 1 for the same reason. During `[0, 1] s`
each UAV rises from z = 0 to its cruise altitude; nothing transmits before
t = 30 s, so this is invisible to the network. Verified: at t = 0.6 s UAV1 is at
`(150, 0, 60)`, and at t = 33 s it is at `(150, 0, 110)`, mid-climb from its
100 m cruise to the 120 m crossing altitude.

---

## 4. Traffic

Generic UDP payloads. They carry **no** MAVLink, no encoded telemetry fields
(position, attitude, battery, velocity, health) and no flight commands — only
the size, direction and rate of such traffic. Real UAV position and velocity
come from the mobility model instead.

| Direction | Rate | Payload | Port | Start |
|---|---|---|---|---|
| UAV → GCS (telemetry-like) | 10 pkt/s per UAV | 200 B | 5000 | 30 s |
| GCS → UAV (command-like) | 2 pkt/s per UAV | 100 B | 5001 | 30 s |

Start times are staggered 10 ms per UAV so the six periodic streams do not fire
on identical timestamps forever. The GCS uses one `UdpSink` plus six separate
`UdpBasicApp` instances — a single app with a destination list would pick a
*random* destination per packet and would not give 2 pkt/s to each UAV.

---

## 5. Running it

```bash
cd ~/costi_omnet
opp_env shell
```

Then either open the IDE:

```bash
omnetpp          # File > Open Projects from File System > uav_mesh_baseline
                 # run simulations/omnetpp.ini, config "Baseline", Qtenv
```

or run it directly:

```bash
cd uav_mesh_baseline/simulations
opp_run -u Qtenv  -n ..:$INET_ROOT/src -l $INET_ROOT/src/INET -c Baseline omnetpp.ini
opp_run -u Cmdenv -n ..:$INET_ROOT/src -l $INET_ROOT/src/INET -c Baseline omnetpp.ini
```

`-n ..` points at the project root because the NED files live in the
`uav_mesh_baseline.simulations` package (see `../package.ned`).

In the IDE, set the launch configuration's **working directory to
`/uav_mesh_baseline/simulations`** (where `omnetpp.ini` lives). The easiest way
is to right-click `simulations/omnetpp.ini` itself rather than the project.

There is no C++ in this project, so nothing needs compiling; `src/` is an empty
placeholder for later custom modules. `../.project` lists `inet-4.7.0` as a
referenced project, which is what lets the IDE resolve INET's NED types and link
its library.

> **Why `.project` has no CDT natures.** The project wizard adds C/C++ natures,
> but this project has no `.cproject` to go with them. The OMNeT++ launcher
> checks every involved project that has the CDT C++ nature and requires them
> all to be consistently debug or release
> (`OmnetppLaunchUtils.isReleaseBinaryRequired`). A project with the nature but
> no `.cproject` reports `unknown`, which cannot match INET's `release`, and the
> launch aborts with *"Error within Debug UI"* — Eclipse's way of hiding the
> real message. Dropping the natures makes the launcher skip this project
> entirely. Re-add C++ support (Project ▸ New ▸ Convert to a C/C++ Project, or
> recreate the project with C++ support) only when custom modules appear under
> `src/`, and make sure its active build configuration then matches INET's.

**Configs**

* `Baseline` — the 420 s scenario, top-down view.
* `BaselineIsometric` — identical, but the scene is drawn from an isometric
  angle so the 80/100/120 m altitude layering is visible.

The view is busy because section 5 of `omnetpp.ini` switches on every
visualizer. Each of those lines is an independent `true` / `false` toggle —
set the ones you do not want to `false` and rerun. See section 6.

A full 420 s run takes about **19 s** of wall time and produces 2 918 456 events.

---

## 6. What to look for in Qtenv

### Reading the picture

The `Baseline` config draws everything at once, which is a lot. Each element:

| On screen | Meaning |
|---|---|
| text next to a node (`wlan0 10.1.0.11`) | interface name and IP address |
| large circles | the 496 m communication range, one per node |
| expanding coloured shapes | radio signals propagating through the air |
| small icons above a node | a signal departing or arriving |
| short green / blue / red / black arrows | one-hop frame exchanges: green RREQ, blue RREP, red RERR, black UDP data |
| thick labelled arrows | routing table state — each node's next hop towards the GCS |
| further arrows along several hops | the end-to-end path a data packet actually took |
| faint lines behind the drones | movement trails; plus a heading cone and a velocity arrow |

Three separate arrow systems overlap in that view. Use `BaselineClean` to see
just the routing-table arrows, or `BaselineTraffic` for just the packet paths.

### Visualizers

| Visualizer | Shows |
|---|---|
| `mobilityVisualizer` | position, velocity arrow, orientation and movement trail of every UAV |
| `mediumVisualizer` | live signal propagation, and a communication-range circle per node |
| `dataLinkVisualizer[0..3]` | one-hop frame exchanges, coloured: **green** RREQ, **blue** RREP, **red** RERR, **black** UDP data |
| `networkRouteVisualizer` | the end-to-end path each data packet actually takes — this is the multi-hop path changing |
| `routingTableVisualizer` | each node's current AODV next hop towards the GCS; the arrows re-point when AODV replaces a route |
| `interfaceTableVisualizer` | node names and addresses |

Every one of these only *reads* state. None of them creates or modifies a
wireless link or a route.

Timeline worth watching:

* **0–30 s** — stationary, no traffic, stable topology.
* **≈30 s** — green RREQ floods spread outward, blue RREPs come back, then data
  starts. On-demand discovery, exactly as AODV should behave.
* **30–150 s** — the swaps happen inside the gateway's range (P1/P2/P3), so the
  *set* of directly-attached UAVs does not change but their *identity* does.
* **150–330 s** — the P4/P5/P6 swaps cross the range boundary; watch the
  routing-table arrows flip between "straight to the gateway" and "via another
  UAV" (see the exact times in §7).

---

## 7. Verification — measured results

Everything below is from an actual 420 s run (`opp_run` exits 0, zero warnings).

### Connectivity follows geometry

Times at which the gateway's set of direct radio neighbours changes, computed
from the trace and the 495.7 m range:

```
t = 162.25 s   +uav4    -> {uav1, uav2, uav3, uav4}
t = 196.78 s   -uav3    -> {uav1, uav2, uav4}
t = 224.25 s   +uav5    -> {uav1, uav2, uav4, uav5}
t = 254.81 s   -uav4    -> {uav1, uav2, uav5}
t = 282.25 s   +uav6    -> {uav1, uav2, uav5, uav6}
t = 316.77 s   -uav5    -> {uav1, uav2, uav6}
```

### AODV follows connectivity

Next hop towards the GCS, taken from the routing log, at the matching moments:

| Physical event | AODV reaction | Lag |
|---|---|---|
| uav4 enters range at 162.25 s | `uav4` uplink → gateway at **163.83 s** | 1.6 s |
| uav3 leaves range at 196.78 s | `uav3` uplink → via `uav4` at **197.12 s** | 0.34 s |
| uav5 enters range at 224.25 s | `uav5` uplink → gateway at **229.84 s** | 5.6 s |
| uav4 leaves range at 254.81 s | `uav4` uplink → via `uav2` at **254.94 s** | 0.13 s |

At t = 30 s, when traffic first starts, AODV discovers exactly the topology the
radio model predicts: `uav1`, `uav2`, `uav3` route **directly** to `10.1.0.1`,
while `uav4` → `uav1`, `uav5` → `uav3`, `uav6` → `uav2` relay.

Uplink (next-hop-towards-GCS) changes over the run:

```
uav1 27   uav2 6   uav3 68   uav4 85   uav5 86   uav6 90    total 362
```

Roles genuinely reverse: `uav3` is gateway-attached until ~197 s and relayed
afterwards; `uav4` is relayed, becomes gateway-attached at ~164 s, then relayed
again after ~255 s.

### Delivery

```
UAV -> GCS telemetry :  23 401 sent   21 798 received   PDR 93.15 %
GCS -> UAV commands  :   4 681 sent    4 677 received   PDR 99.91 %
end-to-end delay (telemetry): mean 212 us, max 1.23 ms
```

The telemetry direction is the lossy one, as expected: it is six times the
packet rate, it is the direction that crosses the congested one-hop bottleneck
into the gateway, and losses concentrate around the moments when a route is
being rediscovered.

### Routing log

`results/Baseline-#0.rt` holds 1 778 routing-table events (79 `+R` additions,
1 679 `*R` changes, 20 `-R` deletions) spread across the whole run, not just at
startup.

### Reproducibility

Mobility is a fixed trace and the seed set is fixed, so repeated runs are
identical — the event count is 2 918 456 every time.

### Quick self-check

To prove the z coordinate really is being applied, constrain it and watch the
run stop where it should:

```bash
opp_run -u Cmdenv -n ..:$INET_ROOT/src -l $INET_ROOT/src/INET -c Baseline \
  --sim-time-limit=40s "--*.uav*.mobility.constraintAreaMinZ=-1m" \
  "--*.uav*.mobility.constraintAreaMaxZ=110m" omnetpp.ini
# expected: error at t=33 s with uav1 at (x=150, y=0, z=110)
```

---

## 8. Outputs, and building 1-second graph snapshots later

Results stay **event-driven** — nothing is sampled at 1 Hz inside OMNeT++.
Post-processing converts them into snapshots afterwards.

| File | Size (420 s) | Contents |
|---|---|---|
| `results/Baseline-#0.rt` | ~360 KB | every routing-table add / change / delete, timestamped |
| `results/Baseline-#0.sca` | ~720 KB | all scalars |
| `results/Baseline-#0.vec` + `.vci` | ~5 MB | application-level vectors |

Mapping to snapshot layers:

| Layer | Source |
|---|---|
| UAV positions / velocities | `mobility/uav_mobility.trace` — deterministic, linearly interpolable at any t, no need to record it |
| Physical radio links | positions plus the 495.7 m threshold documented above |
| AODV routing graph | `Baseline-#0.rt`, replayed forward in time |
| Traffic and delivery | `Baseline-#0.vec` (`packetSent`, `packetReceived`, `endToEndDelay`) |

`.rt` line format (from `RoutingTableRecorder`):

```
<+R|*R|-R>  #<event>  <t>s  <node>  <routerId>  <dest>/<prefixlen>  <nextHop>  <iface> <AodvRouteData>
```

Two practical notes for the replay code:

* A UAV's uplink towards the GCS is the next hop of its `<unspec>/0` route —
  that is AODV's external-gateway default route. Intra-mesh routes appear as
  `/32` entries.
* AODV creates the default route *inactive* with an unspecified next hop and
  fills it in microseconds later, so ignore `<unspec>` next hops.

Multiple 420 s mobility traces can be generated with the same phase structure to
provide different topology sequences for training / validation / unseen-topology
testing.

---

## 9. Tuning knobs

| Want | Change |
|---|---|
| different link structure | `**.wlan[0].radio.transmitter.power` or `receiver.sensitivity` — keep the resulting range inside **[353 m, 550 m]** (§2) |
| realistic fading | `*.radioMedium.pathLoss.typename = "LogNormalShadowing"` — costs determinism |
| less link-layer noise | `**.arp.typename = "GlobalArp"` — removes ARP frames from the air, which is what INET's own MANET showcase does |
| per-frame radio state vectors | uncomment the `*.*.wlan[0].radio.*.vector-recording` line in `omnetpp.ini` (~55 MB per run) |
| see the altitude layering | run the `BaselineIsometric` config |
