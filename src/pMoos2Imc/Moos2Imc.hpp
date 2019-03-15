// MOOS to IMC bridge.
//
// March 26th, 2015

#include "MOOS/libMOOS/App/MOOSApp.h"
#include <DUNE/DUNE.hpp>
using DUNE_NAMESPACES;

class Moos2Imc : public CMOOSApp
{
public:
  bool OnNewMail (MOOSMSG_LIST & Mail);
  bool OnStartUp ();
  bool OnConnectToServer ();
  bool Iterate ( );
  bool imcSend(Message * msg, std::string addr, int port);
  bool sendToDune(Message * msg);
  ~Moos2Imc();
  Moos2Imc();
private:

  // process configuration
  int cfg_dune_port, cfg_imc_id;
  std::string cfg_dune_host;
  //! Navigation data to dispatch
  EstimatedState* m_estate;
  //! If we have new nav data to dispatch
  bool new_nav_data;

  // IMC-related
  UDPSocket sock_send;
};

