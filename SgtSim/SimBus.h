// Copyright 2015 National ICT Australia Limited (NICTA)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SIM_BUS_DOT_H
#define SIM_BUS_DOT_H

#include <SgtSim/SimComponent.h>
#include <SgtSim/SimNetwork.h>

#include <SgtCore/Bus.h>

namespace Sgt
{
    /// @brief Abstract base class for SimBus.
    /// 
    /// Depending on how the derived class works, bus() could either be provided by containment or inheritance.
    class SimBusAbc : virtual public SimComponent
    {
        public:

            /// @brief Return the bus that I wrap (const). 
            virtual const Bus& bus() const = 0;
            /// @brief Return the bus that I wrap (non-const). 
            virtual Bus& bus() = 0;
            
            /// @brief Do anything I need to do to add myself to the simNetwork.
            ///
            /// Important: my bus must separately be added to SimNetwork's Network. This is to prevent any possible
            /// confusion about whether it is already added on not.
            virtual void linkToSimNetwork(SimNetwork& simNetwork);

        protected:

            virtual void initializeState() override;
    };

    /// @brief Simulation bus, corresponding to a Bus in a SimNetwork's network(). 
    /// 
    /// bus() is provided by containment which is enough for a normal network bus. 
    class SimBus : public SimBusAbc
    {
        public:

            SimBus(const std::string& id, Bus& bus) :
                Component(id),
                bus_(&bus)
            {
                // Empty.
            }

            virtual const Bus& bus() const override
            {
                return *bus_;
            }
            
            virtual Bus& bus() override
            {
                return *bus_;
            }


        private:

            Bus* bus_;
    };
}

#endif // SIM_BUS_DOT_H
