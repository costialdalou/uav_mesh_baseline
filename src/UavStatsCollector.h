#ifndef __UAVSTATSCOLLECTOR_H
#define __UAVSTATSCOLLECTOR_H

#include <omnetpp.h>

#include "inet/common/InitStages.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/contract/INetfilter.h"


class UavStatsCollector :
    public omnetpp::cSimpleModule,
    public omnetpp::cListener,
    public inet::NetfilterBase::HookBase
{
  protected:

    // UDP signals that we observe
    omnetpp::simsignal_t packetSentToLowerSignal;
    omnetpp::simsignal_t packetSentToUpperSignal;

    // Statistics produced by this collector
    omnetpp::simsignal_t rreqSentSignal;
    omnetpp::simsignal_t rreqReceivedSignal;

    omnetpp::simsignal_t rrepSentSignal;
    omnetpp::simsignal_t rrepReceivedSignal;

    omnetpp::simsignal_t rerrSentSignal;
    omnetpp::simsignal_t rerrReceivedSignal;

    omnetpp::simsignal_t dataForwardedSignal;

    // The IPv4 Netfilter interface of the UAV being observed
    inet::INetfilter *networkProtocol = nullptr;

  protected:

    virtual int numInitStages() const override
    {
        return inet::NUM_INIT_STAGES;
    }

    virtual void initialize(int stage) override;

    // Listener for UDP packet signals
    virtual void receiveSignal(
        omnetpp::cComponent *source,
        omnetpp::simsignal_t signalID,
        omnetpp::cObject *object,
        omnetpp::cObject *details
    ) override;


    // ---------------------------------------------------------
    // IPv4 Netfilter hooks
    // ---------------------------------------------------------

    virtual inet::INetfilter::IHook::Result
    datagramPreRoutingHook(inet::Packet *packet) override
    {
        return inet::INetfilter::IHook::ACCEPT;
    }

    virtual inet::INetfilter::IHook::Result
    datagramForwardHook(inet::Packet *packet) override;

    virtual inet::INetfilter::IHook::Result
    datagramPostRoutingHook(inet::Packet *packet) override
    {
        return inet::INetfilter::IHook::ACCEPT;
    }

    virtual inet::INetfilter::IHook::Result
    datagramLocalInHook(inet::Packet *packet) override
    {
        return inet::INetfilter::IHook::ACCEPT;
    }

    virtual inet::INetfilter::IHook::Result
    datagramLocalOutHook(inet::Packet *packet) override
    {
        return inet::INetfilter::IHook::ACCEPT;
    }
};

#endif