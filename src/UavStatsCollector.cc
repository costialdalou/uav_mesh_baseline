#include "UavStatsCollector.h"

#include <string>

#include "inet/common/packet/Packet.h"

using namespace omnetpp;
using namespace inet;


Define_Module(UavStatsCollector);


void UavStatsCollector::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    // =========================================================
    // Stage 1: register the signals used by this collector
    // =========================================================

    if (stage == INITSTAGE_LOCAL)
    {
        // Signals emitted by INET UDP
        packetSentToLowerSignal =
            cComponent::registerSignal("packetSentToLower");

    packetSentToUpperSignal =
        cComponent::registerSignal("packetSentToUpper");


        // Signals emitted by OUR collector
        rreqSentSignal =
            registerSignal("rreqSent");

        rreqReceivedSignal =
            registerSignal("rreqReceived");

        rrepSentSignal =
            registerSignal("rrepSent");

        rrepReceivedSignal =
            registerSignal("rrepReceived");

        rerrSentSignal =
            registerSignal("rerrSent");

        rerrReceivedSignal =
            registerSignal("rerrReceived");

        dataForwardedSignal =
            registerSignal("dataForwarded");
    }


    // =========================================================
    // Stage 2: connect the collector to its target UAV
    // =========================================================

    else if (stage == INITSTAGE_ROUTING_PROTOCOLS)
    {
        const char *targetNodeName =
            par("targetNode").stringValue();


        // -----------------------------------------------------
        // Find the target UAV.
        //
        // Example:
        // statsUav4 has targetNode = "uav4"
        // -----------------------------------------------------

        cModule *node =
            getParentModule()->getSubmodule(targetNodeName);

        if (node == nullptr)
        {
            throw cRuntimeError(
                "UavStatsCollector cannot find target node '%s'",
                targetNodeName
            );
        }


        // =====================================================
        // AODV CONTROL-MESSAGE OBSERVATION
        // =====================================================
        //
        // AODV uses UDP.
        //
        // We therefore listen to the UAV's UDP module for:
        //
        //     packets sent downward
        //     packets received from below
        //
        // =====================================================

        cModule *udp =
            node->getSubmodule("udp");

        if (udp == nullptr)
        {
            throw cRuntimeError(
                "Cannot find UDP module inside '%s'",
                node->getFullPath().c_str()
            );
        }

        udp->subscribe(
            packetSentToLowerSignal,
            this
        );

        udp->subscribe(
            packetSentToUpperSignal,
            this
        );


        // =====================================================
        // FORWARDING OBSERVATION
        // =====================================================
        //
        // Find:
        //
        // target UAV
        //    |
        //    +-- ipv4
        //          |
        //          +-- ip
        //
        // The 'ip' module implements INetfilter.
        // =====================================================

        cModule *ipv4 =
            node->getSubmodule("ipv4");

        if (ipv4 == nullptr)
        {
            throw cRuntimeError(
                "Cannot find ipv4 module inside '%s'",
                node->getFullPath().c_str()
            );
        }


        cModule *ip =
            ipv4->getSubmodule("ip");

        if (ip == nullptr)
        {
            throw cRuntimeError(
                "Cannot find ipv4.ip inside '%s'",
                node->getFullPath().c_str()
            );
        }


        networkProtocol =
            dynamic_cast<INetfilter *>(ip);

        if (networkProtocol == nullptr)
        {
            throw cRuntimeError(
                "'%s.ipv4.ip' does not implement INetfilter",
                targetNodeName
            );
        }


        // AODV itself registers at priority 0.
        //
        // Our collector is read-only, so register it later.
        networkProtocol->registerHook(
            100,
            this
        );
    }
}


// =============================================================
// UDP SIGNAL LISTENER
//
// Used ONLY for AODV control-message counts.
// =============================================================

void UavStatsCollector::receiveSignal(
    cComponent *source,
    simsignal_t signalID,
    cObject *object,
    cObject *details)
{
    Packet *packet =
        dynamic_cast<Packet *>(object);

    if (packet == nullptr)
        return;


    std::string name =
        packet->getName();


    // AODV packet names are based on their control-packet class.
    bool isRreq =
        name.find("Rreq") != std::string::npos;

    bool isRrepAck =
        name.find("RrepAck") != std::string::npos;

    bool isRrep =
        name.find("Rrep") != std::string::npos
        && !isRrepAck;

    bool isRerr =
        name.find("Rerr") != std::string::npos;


    // ---------------------------------------------------------
    // AODV packet SENT by this UAV
    // ---------------------------------------------------------

    if (signalID == packetSentToLowerSignal)
    {
        if (isRreq)
            emit(rreqSentSignal, 1L);

        else if (isRrep)
            emit(rrepSentSignal, 1L);

        else if (isRerr)
            emit(rerrSentSignal, 1L);
    }


    // ---------------------------------------------------------
    // AODV packet RECEIVED by this UAV
    // ---------------------------------------------------------

    else if (signalID == packetSentToUpperSignal)
    {
        if (isRreq)
            emit(rreqReceivedSignal, 1L);

        else if (isRrep)
            emit(rrepReceivedSignal, 1L);

        else if (isRerr)
            emit(rerrReceivedSignal, 1L);
    }
}


// =============================================================
// IPv4 FORWARDING HOOK
//
// Called only when this node is forwarding an IP packet
// that came from another node.
// =============================================================

INetfilter::IHook::Result
UavStatsCollector::datagramForwardHook(Packet *packet)
{
    // We only want our experiment's application data,
    // not unrelated IPv4 control traffic.

    std::string name =
        packet->getName();

    bool isExperimentData =
        name.find("UavTelemetry") != std::string::npos ||
        name.find("GcsCommand") != std::string::npos;


    if (isExperimentData)
    {
        emit(
            dataForwardedSignal,
            1L
        );
    }


    // This collector NEVER changes networking behavior.
    return INetfilter::IHook::ACCEPT;
}