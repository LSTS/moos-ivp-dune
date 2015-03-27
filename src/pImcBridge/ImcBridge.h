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
  UDPSocket sock_send, sock_receive;
  DUNE::IO::Poll m_poll;
  uint8_t* bfr;
  int m_DunePort, m_LocalPort, m_ImcId;
  std::string m_DuneHost;

  std::string m_NavPlanId;

  double m_DesiredLat, m_DesiredLon, m_DesiredSpeed, m_DesiredHeading, m_DesiredDepth, m_DesiredAltitude, m_NavLat, m_NavLon;
  std::string m_ControlMode;
};

