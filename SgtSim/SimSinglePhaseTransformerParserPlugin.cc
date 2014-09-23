#include "SimSinglePhaseTransformerParserPlugin.h"

#include "SimNetwork.h"
#include "Simulation.h"

#include <SgtCore/SinglePhaseTransformer.h>
#include <SgtCore/SinglePhaseTransformerParserPlugin.h>

#include <memory>

namespace SmartGridToolbox
{
   void SimSinglePhaseTransformerParserPlugin::parse(const YAML::Node& nd, Simulation& sim,
         const ParserState& state) const
   {
      SinglePhaseTransformerParserPlugin transParser;
      auto trans = sim.newSimComponent<SimSinglePhaseTransformer>(*transParser.parseSinglePhaseTransformer(nd, state));
      
      assertFieldPresent(nd, "network_id");
      assertFieldPresent(nd, "bus_0_id");
      assertFieldPresent(nd, "bus_1_id");

      string netwId = state.expandName(nd["network_id"].as<std::string>());
      std::string bus0Id = state.expandName(nd["bus_0_id"].as<std::string>());
      std::string bus1Id = state.expandName(nd["bus_1_id"].as<std::string>());

      auto netw = sim.simComponent<SimNetwork>(netwId);

      netw->addArc(trans, bus0Id, bus1Id);
   }
}
