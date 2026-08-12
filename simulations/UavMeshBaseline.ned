//
// UAV mesh baseline network for cybersecurity / GNN research.
//
// Eight network nodes:
//
//   gcs      - Ground Control Station, application endpoint, wired only
//   gateway  - stationary ground mesh gateway: wired towards the GCS,
//              IEEE 802.11 ad hoc towards the airborne mesh
//   uav1..6  - airborne IEEE 802.11 ad hoc nodes running AODV
//
// The GCS is deliberately NOT part of the dynamic wireless graph: it reaches
// the airborne network only through the fixed wired link to the gateway.
//
// IMPORTANT: no wireless connection is declared anywhere in this file. The
// airborne topology is produced exclusively by the radio medium model and the
// instantaneous positions of the nodes, and the routing graph on top of it is
// produced exclusively by AODV. Nothing in this project creates, removes or
// schedules a wireless link or a route.
//

package uav_mesh_baseline.simulations;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.aodv.AodvRouter;
import inet.node.ethernet.Eth100M;
import inet.node.inet.StandardHost;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.contract.IIntegratedVisualizer;


network UavMeshBaseline
{
    parameters:
        @display("bgb=820,460");

    submodules:

        // -------------------------------------------------------------
        // Visualization
        // -------------------------------------------------------------
        //
        // The "Multi" variant allows several dataLinkVisualizer
        // instances to display AODV RREQ / RREP / RERR and UDP traffic
        // differently.
        //
        visualizer: <default("IntegratedMultiCanvasVisualizer")> like IIntegratedVisualizer {
            @display("p=60,40;is=s");
        }


        // -------------------------------------------------------------
        // Wireless radio medium
        // -------------------------------------------------------------
        //
        // Scalar IEEE 802.11 radio medium.
        // Wireless connectivity results from the radio model and the
        // instantaneous positions of the gateway and UAVs.
        //
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=60,110;is=s");
        }


        // -------------------------------------------------------------
        // IPv4 configuration
        // -------------------------------------------------------------
        //
        // Assigns the configured IPv4 addresses and the required
        // ground-side static route.
        //
        // AODV manages the dynamic wireless routes.
        //
        configurator: Ipv4NetworkConfigurator {
            @display("p=60,180;is=s");
        }


        // -------------------------------------------------------------
        // Routing-table recorder
        // -------------------------------------------------------------
        //
        // Records routing-table changes so that AODV route creation,
        // replacement and removal can later be analyzed.
        //
        routingTableRecorder: RoutingTableRecorder {
            @display("p=60,250;is=s");
        }


        // =============================================================
        // Ground side
        // =============================================================

        // Ground Control Station.
        // Wired only.
        gcs: StandardHost {
            @display("p=60,360;i=device/server");
        }


        // Ground mesh gateway.
        //
        // Ethernet side:
        //     gateway <--> GCS
        //
        // Wireless side:
        //     gateway <--> UAV mesh
        //
        gateway: GatewayAwareAodvRouter {
            @display("p=190,360;i=device/antennatower");
        }


        // =============================================================
        // Airborne mesh
        // =============================================================
        //
        // Positions are controlled by the mobility trace configured
        // in omnetpp.ini.
        //

        uav1: AodvRouter {
            @display("i=misc/drone");
        }

        uav2: AodvRouter {
            @display("i=misc/drone");
        }

        uav3: AodvRouter {
            @display("i=misc/drone");
        }

        uav4: AodvRouter {
            @display("i=misc/drone");
        }

        uav5: AodvRouter {
            @display("i=misc/drone");
        }

        uav6: AodvRouter {
            @display("i=misc/drone");
        }


        // =============================================================
        // UAV statistics collectors
        // =============================================================
        //
        // Each collector observes one UAV.
        //
        // These collectors do NOT modify packets, routes, mobility,
        // or wireless connectivity.
        //
        // They will be used to record:
        //
        //   - AODV RREQ sent / received
        //   - AODV RREP sent / received
        //   - AODV RERR sent / received
        //   - data packets forwarded for other nodes
        //

        statsUav1: UavStatsCollector {
            parameters:
                targetNode = "uav1";
                @display("p=700,40;is=s");
        }

        statsUav2: UavStatsCollector {
            parameters:
                targetNode = "uav2";
                @display("p=700,70;is=s");
        }

        statsUav3: UavStatsCollector {
            parameters:
                targetNode = "uav3";
                @display("p=700,100;is=s");
        }

        statsUav4: UavStatsCollector {
            parameters:
                targetNode = "uav4";
                @display("p=700,130;is=s");
        }

        statsUav5: UavStatsCollector {
            parameters:
                targetNode = "uav5";
                @display("p=700,160;is=s");
        }

        statsUav6: UavStatsCollector {
            parameters:
                targetNode = "uav6";
                @display("p=700,190;is=s");
        }
        statsGateway: UavStatsCollector {
            parameters:
                targetNode = "gateway";
                @display("p=700,220;is=s");
        }

    connections:

        // -------------------------------------------------------------
        // The only explicitly declared network link in this scenario.
        // -------------------------------------------------------------
        //
        // All UAV/gateway wireless relationships are determined by
        // radio propagation and mobility, not NED connections.
        //
        gcs.ethg++ <--> Eth100M <--> gateway.ethg++;
}
