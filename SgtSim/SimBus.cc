#include "SimBus.h"

namespace Sgt
{
    void SimBusAbc::linkToSimNetwork(SimNetwork& simNetwork)
    {
        // Safety check that my bus has already been added to simNetwork's network.
        auto networkBus = simNetwork.network()->bus(bus()->id());
        sgtAssert(networkBus != nullptr, "My Bus must be added to the SimNetwork's Network before calling "
                << __PRETTY_FUNCTION__);
        
        simNetwork.dependsOn(shared<SimComponent>());
        bus()->setpointChanged().addAction([&simNetwork]() {simNetwork.needsUpdate().trigger();},
                std::string("Trigger ") + sComponentType() + " " + id() + " needs update");
        bus()->isInServiceChanged().addAction([&simNetwork]() {simNetwork.needsUpdate().trigger();},
                std::string("Trigger ") + sComponentType() + " " + id() + " needs update");
    }
            
    void SimBusAbc::initializeState()
    {
        bool sv = bus()->voltageUpdated().isEnabled();
        bus()->voltageUpdated().setIsEnabled(false);
        bus()->setV(bus()->VNom());
        bus()->voltageUpdated().setIsEnabled(sv);
    }
}
