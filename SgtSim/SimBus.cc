#include "SimBus.h"

#include "SimNetwork.h"

namespace SmartGridToolbox
{
   SimBus::SimBus(const std::string& id, Phases phases, const ublas::vector<Complex> & VNom, double VBase) :
      Bus(id, phases, VNom, VBase),
      changed_("SimBus " + id + " setpoint changed")
   {
      // Empty.
   }

   void SimBus::addZip(const std::shared_ptr<Zip>& zip)
   {
      dependsOn(zip);
      zips_.push_back(&zip);
      zip.didUpdate().addAction([this](){needsUpdate().trigger();},
            "Trigger SimBus " + name() + " needsUpdate.");
      zip.didUpdate().addAction([this](){changed().trigger();},
            "Trigger SimBus " + name() + " changed.");
      // TODO: this will recalculate all zips. Efficiency?
      changed().trigger();
   }
}