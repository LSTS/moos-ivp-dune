#include "MOOS/libMOOS/App/MOOSApp.h"
#include "MantaComms.h"

int main(int argc, char *argv[]) 
{
  //here we do some command line parsing ...
  MOOS::CommandLineParser P(argc, argv);
 
  //mission file could be first free parameter
  std::string mission_file = P.GetFreeParameter(0, "Mission.moos");
  
  //app name can be the second free parameter
  std::string app_name = P.GetFreeParameter(1, "MantaComms"); 
  
  MantaComms App;
  App.Run(app_name, mission_file, argc, argv);
  
  return(0);
}

