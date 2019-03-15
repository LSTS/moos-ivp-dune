// MOOS-IMC bridge.
//
// March 26th, 2015

#include "MOOS/libMOOS/App/MOOSApp.h"
#include "Moos2Imc.hpp"

int main(int argc, char *argv[])
{
  // //here we do some command line parsing ...
  MOOS::CommandLineParser P(argc, argv);

  // //mission file could be first free parameter
  std::string mission_file = P.GetFreeParameter(0, "Moos2Imc.moos");

  // //app name can be the second free parameter
  std::string app_name = P.GetFreeParameter(1, "Moos2Imc");

  Moos2Imc App;
  App.Run(app_name, mission_file, argc, argv);

  return(0);
}
