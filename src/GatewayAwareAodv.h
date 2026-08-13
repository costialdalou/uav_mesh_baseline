#ifndef __GATEWAYAWAREAODV_H
#define __GATEWAYAWAREAODV_H

#include "inet/routing/aodv/Aodv.h"

class GatewayAwareAodv : public inet::aodv::Aodv
{
  protected:
    virtual inet::INetfilter::IHook::Result datagramForwardHook(inet::Packet *datagram) override;
};

#endif
