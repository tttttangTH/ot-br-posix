/*
 *    Copyright (c) 2020, The OpenThread Authors.
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *    POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   The file implements the Thread Backbone agent.
 */

#include "backbone_router/backbone_agent.hpp"

#include <assert.h>
#include <openthread/backbone_router_ftd.h>

#include "common/code_utils.hpp"

namespace otbr {
namespace BackboneRouter {

BackboneAgent::BackboneAgent(otbr::Ncp::ControllerOpenThread &aNcp)
    : mNcp(aNcp)
    , mBackboneRouterState(OT_BACKBONE_ROUTER_STATE_DISABLED)
{
}

void BackboneAgent::Init(void)
{
    mNcp.On(Ncp::kEventBackboneRouterState, HandleBackboneRouterState, this);
    mNcp.On(Ncp::kEventBackboneRouterMulticastListenerEvent, HandleBackboneRouterMulticastListenerEvent, this);

    mSMCRouteManager.Init();

    HandleBackboneRouterState();
}

void BackboneAgent::HandleBackboneRouterState(void *aContext, int aEvent, va_list aArguments)
{
    OT_UNUSED_VARIABLE(aEvent);
    OT_UNUSED_VARIABLE(aArguments);

    assert(aEvent == Ncp::kEventBackboneRouterState);

    static_cast<BackboneAgent *>(aContext)->HandleBackboneRouterState();
}

void BackboneAgent::HandleBackboneRouterState(void)
{
    otBackboneRouterState state      = otBackboneRouterGetState(mNcp.GetInstance());
    bool                  wasPrimary = (mBackboneRouterState == OT_BACKBONE_ROUTER_STATE_PRIMARY);

    otbrLog(OTBR_LOG_DEBUG, "BackboneAgent: HandleBackboneRouterState: state=%d, mBackboneRouterState=%d", state,
            mBackboneRouterState);
    VerifyOrExit(mBackboneRouterState != state);

    mBackboneRouterState = state;

    if (IsPrimary())
    {
        OnBecomePrimary();
    }
    else if (wasPrimary)
    {
        OnResignPrimary();
    }

exit:
    return;
}

void BackboneAgent::HandleBackboneRouterMulticastListenerEvent(void *aContext, int aEvent, va_list aArguments)
{
    OT_UNUSED_VARIABLE(aEvent);

    otBackboneRouterMulticastListenerEvent event;
    const otIp6Address *                   address;

    assert(aEvent == Ncp::kEventBackboneRouterMulticastListenerEvent);

    event   = static_cast<otBackboneRouterMulticastListenerEvent>(va_arg(aArguments, int));
    address = va_arg(aArguments, const otIp6Address *);
    static_cast<BackboneAgent *>(aContext)->HandleBackboneRouterMulticastListenerEvent(event, *address);
}

void BackboneAgent::HandleBackboneRouterMulticastListenerEvent(otBackboneRouterMulticastListenerEvent aEvent,
                                                               const otIp6Address &                   aAddress)
{
    otbrLog(OTBR_LOG_INFO, "BackboneAgent: Multicast Listener event: %d, address: %s, state: %s", aEvent,
            Ip6Address(aAddress).ToString().c_str(), StateToString(mBackboneRouterState));

    switch (aEvent)
    {
    case OT_BACKBONE_ROUTER_MULTICAST_LISTENER_ADDED:
        mSMCRouteManager.Add(Ip6Address(aAddress));
        break;
    case OT_BACKBONE_ROUTER_MULTICAST_LISTENER_REMOVED:
        mSMCRouteManager.Remove(Ip6Address(aAddress));
        break;
    }
}

void BackboneAgent::OnBecomePrimary(void)
{
    otbrLog(OTBR_LOG_NOTICE, "BackboneAgent: Backbone Router becomes Primary!");

    mSMCRouteManager.Enable();
}

void BackboneAgent::OnResignPrimary(void)
{
    otbrLog(OTBR_LOG_NOTICE, "BackboneAgent: Backbone Router resigns Primary to %s!",
            StateToString(mBackboneRouterState));

    mSMCRouteManager.Disable();
}

const char *BackboneAgent::StateToString(otBackboneRouterState aState)
{
    const char *ret = "Unknown";

    switch (aState)
    {
    case OT_BACKBONE_ROUTER_STATE_DISABLED:
        ret = "Disabled";
        break;
    case OT_BACKBONE_ROUTER_STATE_SECONDARY:
        ret = "Secondary";
        break;
    case OT_BACKBONE_ROUTER_STATE_PRIMARY:
        ret = "Primary";
        break;
    }

    return ret;
}

} // namespace BackboneRouter
} // namespace otbr
