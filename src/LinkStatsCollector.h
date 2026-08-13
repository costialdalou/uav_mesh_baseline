#ifndef __LINKSTATSCOLLECTOR_H
#define __LINKSTATSCOLLECTOR_H

#include <omnetpp.h>

#include <fstream>
#include <map>
#include <string>

#include "inet/common/InitStages.h"
#include "inet/common/packet/Packet.h"

class LinkStatsCollector :
    public omnetpp::cSimpleModule,
    public omnetpp::cListener
{
  protected:

    // Signal emitted when the Wi-Fi MAC successfully passes
    // a received packet upward.
    omnetpp::simsignal_t packetSentToUpperSignal;

    // Maps Wi-Fi MAC address -> node name.
    //
    // Example:
    // 0A-AA-00-00-00-01 -> gateway
    // 0A-AA-00-00-00-02 -> uav1
    std::map<std::string, std::string> macToNode;

    // Output CSV file.
    std::ofstream output;

  protected:

    virtual int numInitStages() const override
    {
        return inet::NUM_INIT_STAGES;
    }

    virtual void initialize(int stage) override;

    virtual void finish() override;

    virtual void receiveSignal(
        omnetpp::cComponent *source,
        omnetpp::simsignal_t signalID,
        omnetpp::cObject *object,
        omnetpp::cObject *details
    ) override;

    // Subscribe to one node's Wi-Fi MAC and remember
    // which MAC address belongs to that node.
    void registerWirelessNode(
        const char *nodeName
    );

    // Determines which top-level network node emitted a signal.
    std::string getNodeName(
        omnetpp::cComponent *source
    ) const;
};

#endif