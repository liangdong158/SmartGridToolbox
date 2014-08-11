#include "Zip.h"

#include <ostream>

namespace SmartGridToolbox
{
   std::ostream& Zip::print(std::ostream& os) const
   {
      IndentingOStreamBuf ind(os,"");
      os << "zip:" << std::endl;
      ind.setInd("    ");
      os << "id: " << id_ << std::endl;
      os << "phases: " << phases_ << std::endl;
      os << "YConst: " << YConst_ << std::endl;
      os << "IConst: " << IConst_ << std::endl;
      os << "SConst: " << SConst_ << std::endl;
      return os;
   }
}
