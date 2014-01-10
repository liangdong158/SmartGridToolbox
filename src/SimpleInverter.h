#ifndef SIMPLE_INVERTER_DOT_H
#define SIMPLE_INVERTER_DOT_H

#include <SmartGridToolbox/InverterBase.h>

namespace SmartGridToolbox
{
   /// SimpleInverter: DC power to n-phase AC converter.
   class SimpleInverter : public InverterBase
   {
      /// @name Overridden from InverterBase
      /// @{
      public:
         virtual ublas::vector<Complex> S() const override;
         virtual double PPerPhase() const override;
      /// @}
      
      /// @name My public member functions.
      /// @{
      public:
         SimpleInverter(const std::string & name, const Phases & phases);

         virtual double efficiency(double powerDc) const override
         {
            return efficiency_;
         }
         void setEfficiency(double efficiency)
         {
            efficiency_ = efficiency;
         }

         double maxSMagPerPhase() const
         {
            return maxSMagPerPhase_;
         }
         void setMaxSMagPerPhase(double maxSMagPerPhase)
         {
            maxSMagPerPhase_ = maxSMagPerPhase;
         }

         double minPowerFactor() const
         {
            return minPowerFactor_;
         }
         void setMinPowerFactor(double minPowerFactor)
         {
            minPowerFactor_ = minPowerFactor;
         }

         double requestedQPerPhase() const
         {
            return requestedQPerPhase_;
         }
         void setRequestedQPerPhase(double requestedQPerPhase)
         {
            requestedQPerPhase_ = requestedQPerPhase;
         }

         bool inService() const
         {
            return inService_;
         }
         void setInService(bool inService)
         {
            inService_ = inService;
         }
      /// @}

      /// @name My private member functions.
      /// @{
      public:
      /// @}

      /// @name Private member variables.
      /// @{
      public:
         // Operating parameters:
         double efficiency_;
         double maxSMagPerPhase_;
         double minPowerFactor_;

         // Settings:
         double requestedQPerPhase_;
         bool inService_;

         // State:
         ublas::vector<Complex> S_;
      /// @}
   };
}

#endif // SIMPLE_INVERTER_DOT_H
