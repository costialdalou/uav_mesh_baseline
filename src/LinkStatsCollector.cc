#include "LinkStatsCollector.h"

#include <iomanip>

#include "inet/linklayer/common/MacAddress.h"
#include "inet/linklayer/common/MacAddressTag_m.h"

using namespace omnetpp;

Define_Module(LinkStatsCollector);

void LinkStatsCollector::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {

        // Get the signal ID used by INET when a MAC successfully
        // passes a received packet upward.
        packetSentToUpperSignal =
            cComponent::registerSignal("packetSentToUpper");

        // Open our CSV output file.
        const char *fileName = par("outputFile").stringValue();

        output.open(fileName);

        if (!output.is_open()) {
            throw cRuntimeError(
                "LinkStatsCollector could not open output file: %s",
                fileName
            );
        }

        // CSV header
        output
            << "time_s,"
            << "transmitter,"
            << "receiver,"
            << "source_mac,"
            << "dest_mac,"
            << "packet_name,"
            << "packet_bytes"
            << "\n";
    }

    //
    // Wait until the networking modules have completed initialization.
    // At this point, the automatically generated Wi-Fi MAC addresses
    // already exist.
    //
    if (stage == inet::INITSTAGE_APPLICATION_LAYER) {

        registerWirelessNode("gateway");

        registerWirelessNode("uav1");
        registerWirelessNode("uav2");
        registerWirelessNode("uav3");
        registerWirelessNode("uav4");
        registerWirelessNode("uav5");
        registerWirelessNode("uav6");
    }
}


void LinkStatsCollector::registerWirelessNode(
    const char *nodeName
)
{
    //
    // The LinkStatsCollector will be placed directly inside
    // UavMeshBaseline, so its parent is the network itself.
    //
    cModule *network = getParentModule();

    cModule *node = network->getSubmodule(nodeName);

    if (node == nullptr) {
        throw cRuntimeError(
            "LinkStatsCollector: node '%s' was not found",
            nodeName
        );
    }

    //
    // Find wlan[0].
    //
    cModule *wlan = node->getSubmodule("wlan", 0);

    if (wlan == nullptr) {
        throw cRuntimeError(
            "LinkStatsCollector: wlan[0] not found in node '%s'",
            nodeName
        );
    }

    //
    // Find the IEEE 802.11 MAC module inside wlan[0].
    //
    cModule *mac = wlan->getSubmodule("mac");

    if (mac == nullptr) {
        throw cRuntimeError(
            "LinkStatsCollector: MAC module not found in node '%s'",
            nodeName
        );
    }

    //
    // Listen only to successful packets passed upward by this
    // wireless MAC.
    //
    mac->subscribe(
        packetSentToUpperSignal,
        this
    );

    //
    // Read this interface's actual MAC address.
    //
    // INET replaces "auto" with a generated MAC address during
    // initialization.
    //
    const char *addressText =
        wlan->par("address").stringValue();

    if (strcmp(addressText, "auto") == 0) {
        throw cRuntimeError(
            "LinkStatsCollector: MAC address for '%s' is still 'auto'",
            nodeName
        );
    }

    //
    // Normalize the MAC address using INET's MacAddress class.
    //
    inet::MacAddress macAddress;
    macAddress.setAddress(addressText);

    macToNode[macAddress.str()] = nodeName;

    EV_INFO
        << "LinkStatsCollector registered "
        << nodeName
        << " with MAC "
        << macAddress
        << endl;
}


std::string LinkStatsCollector::getNodeName(
    cComponent *source
) const
{
    cModule *module =
        dynamic_cast<cModule *>(source);

    if (module == nullptr)
        return "";

    //
    // Example hierarchy:
    //
    // UavMeshBaseline
    //   └── uav3
    //        └── wlan[0]
    //             └── mac
    //
    // Walk upward until we reach the module immediately below
    // UavMeshBaseline.
    //
    while (
        module != nullptr &&
        module->getParentModule() != getParentModule()
    ) {
        module = module->getParentModule();
    }

    if (module == nullptr)
        return "";

    return module->getName();
}


void LinkStatsCollector::receiveSignal(
    cComponent *source,
    simsignal_t signalID,
    cObject *object,
    cObject *details
)
{
    if (signalID != packetSentToUpperSignal)
        return;

    //
    // The emitted object should be an INET Packet.
    //
    inet::Packet *packet =
        dynamic_cast<inet::Packet *>(object);

    if (packet == nullptr)
        return;

    //
    // Ieee80211Mac attaches this tag after successfully
    // decapsulating the received Wi-Fi frame.
    //
    auto macInd = packet->findTag<inet::MacAddressInd>();

    if (macInd == nullptr)
        return;

    const std::string sourceMac =
        macInd->getSrcAddress().str();

    const std::string destMac =
        macInd->getDestAddress().str();

    //
    // Convert transmitting MAC address into our node name.
    //
    auto it = macToNode.find(sourceMac);

    if (it == macToNode.end()) {
        EV_WARN
            << "LinkStatsCollector: unknown transmitter MAC "
            << sourceMac
            << endl;

        return;
    }

    const std::string transmitter =
        it->second;

    //
    // The module emitting packetSentToUpper is the receiver's MAC,
    // so obtain the receiving network node from the module hierarchy.
    //
    const std::string receiver =
        getNodeName(source);

    if (receiver.empty())
        return;

    //
    // Do not record a nonsensical self-link.
    //
    if (transmitter == receiver)
        return;

    //
    // Packet::getBitLength() returns the current packet data
    // length in bits. Our network packets are byte aligned.
    //
    const long packetBytes =
        packet->getBitLength() / 8;

    //
    // One CSV row = one successful one-hop wireless reception.
    //
    output
        << std::fixed
        << std::setprecision(9)
        << simTime().dbl()
        << ","
        << transmitter
        << ","
        << receiver
        << ","
        << sourceMac
        << ","
        << destMac
        << ",\""
        << packet->getName()
        << "\","
        << packetBytes
        << "\n";
}


void LinkStatsCollector::finish()
{
    if (output.is_open()) {
        output.flush();
        output.close();
    }
}
