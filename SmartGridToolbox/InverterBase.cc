#include <SmartGridToolbox/InverterBase.h>

#include <SmartGridToolbox/DcPowerSourceBase.h>

namespace SmartGridToolbox
{
   void InverterBase::addDcPowerSource(DcPowerSourceBase & source)
   {
      dependsOn(source);
      sources_.push_back(&source);
      source.didUpdate().addAction([this](){needsUpdate().trigger();}, 
            "Trigger InverterBase " + name() + " needs update.");
      // TODO: this will recalculate all zips. Efficiency?
   }

   void InverterBase::updateState(Time t0, Time t1)
   {
      PDc_ = 0.0;
      for (const DcPowerSourceBase* source : sources_)
      {
         PDc_ += source->PDc();
      }
   }

   double InverterBase::PPerPhase() const
   {
      return PDc_*efficiency(PDc_)/phases().size();
   }
}