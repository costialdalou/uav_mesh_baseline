#ifndef __GATEWAYAWAREAODV_H
#define __GATEWAYAWAREAODV_H

#include "inet/routing/aodv/Aodv.h"

// Project-local AODV adaptation for the mesh gateway. It suppresses the
// spurious RERR that the base AODV forwarding hook would otherwise generate
// when the gateway uses a valid, non-AODV route to the external GCS network.
class GatewayAwareAodv : public inet::aodv::Aodv
{
  protected:
    virtual inet::INetfilter::IHook::Result datagramForwardHook(inet::Packet *datagram) override;
};

#endif
