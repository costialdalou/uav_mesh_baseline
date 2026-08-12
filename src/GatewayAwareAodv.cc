#include "GatewayAwareAodv.h"

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3Tools.h"

using namespace omnetpp;
using namespace inet;

Define_Module(GatewayAwareAodv);

INetfilter::IHook::Result GatewayAwareAodv::datagramForwardHook(Packet *datagram)
{
    Enter_Method("datagramForwardHook");

    const auto& networkHeader = getNetworkProtocolHeader(datagram);
    const L3Address& destination = networkHeader->getDestinationAddress();

    // AODV manages only the wireless mesh subnet. At the configured gateway,
    // packets for an external address are forwarded by the normal routing
    // table (for this scenario, over Ethernet to the GCS). That valid route was
    // not created by AODV, so the base forwarding hook would misclassify it as
    // missing and broadcast an RERR for every forwarded telemetry packet.
    if (hasExternalGateway && isExternalAddress(destination)) {
        if (gatewayAddress.isUnspecified())
            gatewayAddress = L3AddressResolver().resolve(par("gatewayAddress"));

        if (getSelfIPAddress() == gatewayAddress)
            return ACCEPT;
    }

    // Preserve the original INET behavior for mesh destinations and for all
    // genuine route failures.
    return Aodv::datagramForwardHook(datagram);
}
