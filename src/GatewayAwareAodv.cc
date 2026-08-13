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

    if (hasExternalGateway && isExternalAddress(destination)) {
        if (gatewayAddress.isUnspecified())
            gatewayAddress = L3AddressResolver().resolve(par("gatewayAddress"));

        if (getSelfIPAddress() == gatewayAddress)
            return ACCEPT;
    }

    return Aodv::datagramForwardHook(datagram);
}
