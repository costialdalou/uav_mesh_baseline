package uav_mesh_baseline.simulations;

import inet.routing.aodv.Aodv;

// Uses the project-local C++ adaptation while retaining every parameter,
// signal, and gate of INET's standard AODV module.
simple GatewayAwareAodv extends Aodv
{
    parameters:
        @class(GatewayAwareAodv);
}
