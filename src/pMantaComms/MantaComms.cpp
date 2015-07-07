#include "MantaComms.h"
#include "IMCUtilities.h"

MantaComms::MantaComms() {
  //defaults
  cfg_dune_port = 6002;
  cfg_local_port = 6969;
  cfg_imc_id = 65001;
  cfg_lat_origin = cfg_lon_origin = 0;
  cfg_dune_host = "localhost";
  nav_control_mode = "PLAN";
  nav_aborted = false;
  ivp_helm_state = "PARK";
  ivp_desired_depth = ivp_desired_heading = ivp_desired_speed = nav_lat = nav_lon = 0;
  bfr = new uint8_t[65535];
}

MantaComms::~MantaComms() {
  unbind();
}

bool MantaComms::OnNewMail (MOOSMSG_LIST & NewMail) {

  bool speed_changed = false, heading_changed = false, loc_changed = false;

  MOOSMSG_LIST::iterator p;
  for (p = NewMail.begin(); p != NewMail.end(); p++) {
      if (nav_aborted)
          continue;

      CMOOSMsg &rMsg = *p;
      if (strcmp(rMsg.GetKey().c_str(), "NAV_LAT") == 0)
	       m_state.lat = Angles::radians(rMsg.m_dfVal);
      else if (strcmp(rMsg.GetKey().c_str(), "NAV_LONG") == 0)
   	       m_state.lon = Angles::radians(rMsg.m_dfVal);
      else if (strcmp(rMsg.GetKey().c_str(), "NAV_ALTITUDE") == 0)
           m_state.alt = rMsg.m_dfVal;
      else if (strcmp(rMsg.GetKey().c_str(), "NAV_DEPTH") == 0)
           m_state.depth = rMsg.m_dfVal;
      else if (strcmp(rMsg.GetKey().c_str(), "NAV_HEADING") == 0)
           m_state.phi = rMsg.m_dfVal;
      else if (strcmp(rMsg.GetKey().c_str(), "BATTERY_PERCENT") == 0)
           m_fuel.value = (int) rMsg.m_dfVal;
  }
  return(true);
}

bool MantaComms::OnStartUp () {
  if (!m_MissionReader.GetConfigurationParam("DunePort", cfg_dune_port))
    MOOSTrace ("Warning parameter \"DunePort\" not specified. Using \"%d\"\n" , cfg_dune_port);

  if (!m_MissionReader.GetConfigurationParam("DuneHost", cfg_dune_host))
    MOOSTrace ("Warning parameter \"DuneHost\" not specified. Using \"%s\"\n" , cfg_dune_host.c_str());

  if (!m_MissionReader.GetConfigurationParam("LocalPort", cfg_local_port))
    MOOSTrace ("Warning parameter \"LocalPort\" not specified. Using \"%d\"\n" , cfg_local_port);

  if (!m_MissionReader.GetConfigurationParam("ImcId", cfg_imc_id))
    MOOSTrace ("Warning parameter \"ImcId\" not specified. Using \"%d\"\n" , cfg_imc_id);

  MOOSTrace ("Sending IMC data to %s:%d\n", cfg_dune_host.c_str(), cfg_dune_port);
  sock_send.connect(Address(cfg_dune_host.c_str()), cfg_dune_port);

  MOOSTrace ("Binding to port %d\n", cfg_local_port);
  return bind(cfg_local_port);
}

bool MantaComms::OnConnectToServer () {

  bool status = true;

  // Vehicle state
  status = status && Register("NAV_LAT", 1.0);
  status = status && Register("NAV_LONG", 1.0);
  status = status && Register("NAV_DEPTH", 1.0);
  status = status && Register("NAV_ALTITUDE", 1.0);
  status = status && Register("NAV_HEADING", 1.0);
  status = status && Register("BATTERY_PERCENT", 1.0);
  status = status && Register("VEHICLE_UNDERWAY", 1.0);

  return status;
}

bool MantaComms::Iterate ( ) {
  //std :: vector<unsigned char> X(100) ;

  Message * msg;

  int count = 0;
  while ((msg = imcPoll()) != NULL) {
      processMessage(msg);
      free(msg);
      sendToDune(&m_state);
      sendToDune(&m_pstate);
      sendToDune(&m_fuel);
      count ++;
  }

  Heartbeat * beat = createHeartBeatMessage();
  sendToDune(beat);
  free(beat);
  return true ;
}

void MantaComms::processMessage(Message * message) {

  int type = message->getId();

  if (type == Abort::getIdStatic()) {
      // user has aborted operation. Stop MOOS.
      nav_aborted = true;
      MOOSTrace("Execution aborted by the user.\n");
      Notify("VEHICLE_UNDERWAY", "false");
  }
}

Message * MantaComms::imcPoll() {

  if (m_poll.poll(0)) {
      Address addr;
      uint16_t rv = sock_receive.read(bfr, 65535, &addr);
      IMC::Message * msg = IMC::Packet::deserialize(bfr, rv);
      return msg;
  }
  return NULL;
}

bool MantaComms::bind(int port) {
  sock_receive.bind(port, Address::Any, false);
  m_poll.add(sock_receive);
  return true;
}

bool MantaComms::unbind() {
  m_poll.remove(sock_receive);
  return true;
}

bool MantaComms::sendToDune(Message * msg) {
  return imcSend(msg, cfg_dune_host, cfg_dune_port);
}

bool MantaComms::imcSend(Message * msg, std::string addr, int port) {
  msg->setTimeStamp();
  msg->setSource(cfg_imc_id);

  DUNE::Utils::ByteBuffer bb;
  try {
      IMC::Packet::serialize(msg, bb);
      return sock_send.write(bb.getBuffer(), msg->getSerializationSize(),
			     Address(addr.c_str()), port);
  }
  catch (std::runtime_error& e)
  {
      MOOSTrace ("ERROR sending %s to %s:%d: %s\n", msg->getName(), addr.c_str(), port, e.what());
      return false;
  }

  return true;
}
