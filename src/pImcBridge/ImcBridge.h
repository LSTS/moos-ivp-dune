// MOOS-IMC bridge.
//
// March 26th, 2015

#include "MOOS/libMOOS/App/MOOSApp.h"
#include <DUNE/DUNE.hpp>
using DUNE_NAMESPACES;

class ImcBridge : public CMOOSApp {
public:
  bool OnNewMail (MOOSMSG_LIST & Mail); 
  bool OnStartUp ();
  bool OnConnectToServer ();
  bool Iterate ( );
  Message * imcPoll();
  bool imcSend(Message * msg, std::string addr, int port);
  bool sendToDune(Message * msg);
  bool bind(int port);
  bool unbind();
  void processMessage(Message * message);
  ~ImcBridge();
  ImcBridge();
private:

  // process configuration
  int cfg_dune_port, cfg_local_port, cfg_imc_id;
  std::string cfg_dune_host;

  // mission configuration
  double cfg_lat_origin, cfg_lon_origin;

  // IVP variables
  std::string ivp_helm_state;
  double ivp_desired_speed, ivp_desired_heading, ivp_desired_depth;

  // Variables coming from nav computer
  std::string nav_plan_id, nav_control_mode;
  double nav_lat, nav_lon;
  bool nav_aborted;

  // IMC-related
  UDPSocket sock_send, sock_receive;
  DUNE::IO::Poll m_poll;
  uint8_t* bfr;

};

