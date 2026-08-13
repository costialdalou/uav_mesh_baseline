
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

        visualizer: <default("IntegratedMultiCanvasVisualizer")> like IIntegratedVisualizer {
            @display("p=60,40;is=s");
        }

        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=60,110;is=s");
        }

        configurator: Ipv4NetworkConfigurator {
            @display("p=60,180;is=s");
        }

        routingTableRecorder: RoutingTableRecorder { //it records events whenever a route or interface changes.
            @display("p=60,250;is=s");
        }
        linkStats: LinkStatsCollector {
            parameters:
            @display("p=400,40;is=s");
        }


        gcs: StandardHost {
            @display("p=60,360;i=device/server");
        }

        gateway: GatewayAwareAodvRouter {
            @display("p=190,360;i=device/antennatower");
        }

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

        gcs.ethg++ <--> Eth100M <--> gateway.ethg++;
}
