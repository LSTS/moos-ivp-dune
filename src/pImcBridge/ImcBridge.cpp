#include "ImcBridge.h"
#include "IMCUtilities.h"

ImcBridge::ImcBridge() {
  //defaults
  m_DunePort = 6002;
  m_LocalPort = 6969;
  m_ImcId = 65001;
  m_DuneHost = "localhost";
  m_ControlMode = "PLAN";
  m_DesiredAltitude = m_DesiredDepth = m_DesiredHeading = m_DesiredLat =
      m_DesiredLon = m_DesiredSpeed = 0;
  bfr = new uint8_t[65535];
}

ImcBridge::~ImcBridge() {
  unbind();
}


bool ImcBridge::OnNewMail (MOOSMSG_LIST & NewMail) {

  bool speed_changed = false, heading_changed = false, loc_changed = false;

  MOOSMSG_LIST::iterator p;
  for (p = NewMail.begin(); p != NewMail.end(); p++) {
      CMOOSMsg &rMsg = *p;
      if (strcmp(rMsg.GetKey().c_str(), "CONTROL_MODE") == 0) {
	  m_ControlMode = rMsg.GetAsString();
      }
      if (strcmp(rMsg.GetKey().c_str(), "DESIRED_HEADING") == 0) {
	  m_DesiredHeading = rMsg.m_dfVal;
      }
      else if (strcmp(rMsg.GetKey().c_str(), "DESIRED_SPEED") == 0) {
	  m_DesiredSpeed = rMsg.m_dfVal;
      }
      else if (strcmp(rMsg.GetKey().c_str(), "DESIRED_DEPTH") == 0) {
	  m_DesiredDepth = rMsg.m_dfVal;
      }
      else if (strcmp(rMsg.GetKey().c_str(), "DESIRED_LAT") == 0) {
	  m_DesiredLat = rMsg.m_dfVal;
      }
      else if (strcmp(rMsg.GetKey().c_str(), "DESIRED_LON") == 0) {
	  m_DesiredLon = rMsg.m_dfVal;
      }
      else if (strcmp(rMsg.GetKey().c_str(), "DESIRED_PLAN_ID") == 0) {
	  std::string plan_id = rMsg.GetAsString();
	  if (strcmp("ABORT", plan_id.c_str()) == 0)  {
	      MOOSTrace("SENDING ABORT!\n");
	      PlanControl * msg = createStopPlanMessage();
	      sendToDune(msg);
	      free(msg);
	  }
	  else if (m_ControlMode == "PLAN") {
	    PlanControl * msg = createStartPlanMessage(plan_id);
	    sendToDune(msg);
	    free(msg);
	  }
      }
  }
  return(true);
}
  
bool ImcBridge::OnStartUp () {
  if (!m_MissionReader.GetConfigurationParam("DunePort", m_DunePort))
    MOOSTrace ("Warning parameter \"DunePort\" not specified. Using \"%d\"\n" , m_DunePort);
  
  if (!m_MissionReader.GetConfigurationParam("DuneHost", m_DuneHost))
    MOOSTrace ("Warning parameter \"DuneHost\" not specified. Using \"%s\"\n" , m_DuneHost.c_str());

  if (!m_MissionReader.GetConfigurationParam("LocalPort", m_LocalPort))
    MOOSTrace ("Warning parameter \"LocalPort\" not specified. Using \"%d\"\n" , m_LocalPort);

  if (!m_MissionReader.GetConfigurationParam("ImcId", m_ImcId))
    MOOSTrace ("Warning parameter \"ImcId\" not specified. Using \"%d\"\n" , m_ImcId);

  MOOSTrace ("Sending IMC data to %s:%d\n", m_DuneHost.c_str(), m_DunePort);
  sock_send.connect(Address(m_DuneHost.c_str()), m_DunePort);

  MOOSTrace ("Binding to port %d\n", m_LocalPort);
  return bind(m_LocalPort);
} 

bool ImcBridge::OnConnectToServer () {

  bool status = true;

  status = status && Register("IVPHELM_ENGAGED", 0);
  status = status && Register("VEHICLE_UNDERWAY", 0);

  // "IVP" control
  status = status && Register("DESIRED_SPEED", 0);
  status = status && Register("DESIRED_HEADING", 0);
  status = status && Register("DESIRED_DEPTH", 0);

  // "WPT" control
  status = status && Register("DESIRED_LAT", 0);
  status = status && Register("DESIRED_LON", 0);

  // "PLAN" control
  status = status && Register("DESIRED_PLAN_ID", 0);

  status = status && Register("CONTROL_MODE", 0);

  return status;
} 
  
bool ImcBridge::Iterate ( ) {
  //std :: vector<unsigned char> X(100) ;

  Message * msg;

  int count = 0;
  while ((msg = imcPoll()) != NULL) {
      processMessage(msg);
      free(msg);
      count ++;
  }

  Heartbeat * beat = createHeartBeatMessage();
  sendToDune(beat);
  free(beat);

  if (count == 0) {
      Notify("NAV_CONNECTED","false") ;
  }
  else {
      Notify("NAV_CONNECTED","true") ;
  }
  return true ;
}

void ImcBridge::processMessage(Message * message) {

  int type = message->getId();

  if (type == EstimatedState::getIdStatic()) {
    EstimatedState * msg = dynamic_cast<IMC::EstimatedState *>(message);

    double lat = msg->lat;
    double lon = msg->lon;
    WGS84::displace(msg->x, msg->y, &lat, &lon);

    Notify("NAV_LAT", Angles::degrees(lat));
    Notify("NAV_LON", Angles::degrees(lon));
    Notify("NAV_DEPTH", msg->depth);
    Notify("NAV_ALTITUDE", msg->alt);
  }
  else if (type == VehicleState::getIdStatic()) {
    VehicleState * msg = dynamic_cast<IMC::VehicleState *>(message);

    if (msg->op_mode == VehicleState::VS_SERVICE)
      Notify("NAV_CONTROL_MODE", "PLAN");
    else if (msg->op_mode == VehicleState::VS_ERROR || msg->op_mode == VehicleState::VS_BOOT)
      Notify("NAV_CONTROL_MODE", "ERROR");
  }
  else if (type == PlanControlState::getIdStatic()) {
      PlanControlState * msg = dynamic_cast<IMC::PlanControlState *>(message);
      if (msg->state == PlanControlState::PCS_EXECUTING) {
	Notify("NAV_PLAN_ID", msg->plan_id);
	if (msg->plan_id == IVP_PLAN_ID) {
	    Notify("NAV_CONTROL_MODE", "IVP");
	}
	else if (msg->plan_id == WPT_PLAN_ID) {
	    Notify("NAV_CONTROL_MODE", "WPT");
	}
	else {
	    Notify("NAV_CONTROL_MODE", "PLAN");
	}
      }
      else
	Notify("NAV_PLAN_ID", "");
  }
  else if (type == GpsFix::getIdStatic()) {
      GpsFix * msg = dynamic_cast<IMC::GpsFix *>(message);
      Notify("NAV_GPS_SATELLITES", msg->satellites);
  }
}

Message * ImcBridge::imcPoll() {

  if (m_poll.poll(0)) {
    Address addr;
    uint16_t rv = sock_receive.read(bfr, 65535, &addr);
    IMC::Message * msg = IMC::Packet::deserialize(bfr, rv);
    return msg;
  }
  return NULL;
}

bool ImcBridge::bind(int port) {
  sock_receive.bind(port, Address::Any, false);
  m_poll.add(sock_receive);
  return true;
}

bool ImcBridge::unbind() {
  m_poll.remove(sock_receive);
  return true;
}

bool ImcBridge::sendToDune(Message * msg) {
  return imcSend(msg, m_DuneHost, m_DunePort);
}

bool ImcBridge::imcSend(Message * msg, std::string addr, int port) {
  msg->setTimeStamp();
  msg->setSource(m_ImcId);

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

