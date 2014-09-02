#include "GenericZipParser.h"

#include "Bus.h"
#include "Network.h"
#include "Zip.h"

namespace SmartGridToolbox
{
   void GenericZipParser::parse(const YAML::Node& nd, Network& into) const
   {
      SGT_DEBUG(debug() << "GenericZip : parse." << std::endl);

      auto zip = parseGenericZip(nd);

      assertFieldPresent(nd, "bus_id");

      std::string busId = nd["bus_id"].as<std::string>();
      into.addZip(std::move(zip), busId);
   }

   std::unique_ptr<GenericZip> GenericZipParser::parseGenericZip(const YAML::Node& nd) const
   {
      assertFieldPresent(nd, "id");
      assertFieldPresent(nd, "phases");
      assertFieldPresent(nd, "Y_const");
      assertFieldPresent(nd, "I_const");
      assertFieldPresent(nd, "S_const");

      std::string id = nd["id"].as<std::string>();
      Phases phases = nd["phases"].as<Phases>();
      ublas::vector<Complex> Y = nd["Y_const"].as<ublas::vector<Complex>>();
      ublas::vector<Complex> I = nd["I_const"].as<ublas::vector<Complex>>();
      ublas::vector<Complex> S = nd["S_const"].as<ublas::vector<Complex>>();
      
      std::unique_ptr<GenericZip> zip(new GenericZip(id, phases));
      zip->setYConst(Y);
      zip->setIConst(I);
      zip->setSConst(S);

      return zip;
   }

}