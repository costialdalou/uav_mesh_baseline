package uav_mesh_baseline.simulations;

import inet.node.inet.AdhocHost;

module GatewayAwareAodvRouter extends AdhocHost
{
    submodules:
        aodv: GatewayAwareAodv {
            @display("p=825,226");
        }
    connections:
        aodv.socketOut --> at.in++;
        aodv.socketIn <-- at.out++;
}
