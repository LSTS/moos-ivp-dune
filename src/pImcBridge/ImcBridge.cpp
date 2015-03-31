#include "ImcBridge.h"
#include "IMCUtilities.h"

ImcBridge::ImcBridge() {
  //defaults
  cfg_dune_port = 6002;
  cfg_local_port = 6969;
  cfg_imc_id = 65001;
  cfg_lat_origin = cfg_lon_origin = 0;
  cfg_dune_host = "localhost";
  nav_control_mode = "PLAN";
  ivp_helm_state = "PARK";
  ivp_desired_depth = ivp_desired_heading = ivp_desired_speed = nav_lat = nav_lon = 0;
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
      if (strcmp(rMsg.GetKey().c_str(), "IVPHELM_STATE") == 0) {

	  ivp_helm_state = rMsg.GetAsString();

	  if (strcmp(ivp_helm_state.c_str(), "DRIVE") == 0 && strcmp(nav_control_mode.c_str(), "IVP") != 0) {
	      PlanControl *pc = createIVPPlan(cfg_imc_id);
	      sendToDune(pc);
	      free(pc);
	  }
	  else if (strcmp(ivp_helm_state.c_str(), "PARK") == 0 && strcmp(nav_control_mode.c_str(), "IVP") == 0) {
	      PlanControl *pc = createStopPlanMessage();
	      sendToDune(pc);
	      free(pc);
	  }
      }
      if (strcmp(rMsg.GetKey().c_str(), "DESIRED_HEADING") == 0) {
	  ivp_desired_heading = rMsg.m_dfVal;
      }
      else if (strcmp(rMsg.GetKey().c_str(), "DESIRED_SPEED") == 0) {
	  ivp_desired_speed = rMsg.m_dfVal;
      }
      else if (strcmp(rMsg.GetKey().c_str(), "DESIRED_DEPTH") == 0) {
	  ivp_desired_depth = rMsg.m_dfVal;
      }
      else if (strcmp(rMsg.GetKey().c_str(), "DESIRED_PLAN_ID") == 0) {
	  std::string plan_id = rMsg.GetAsString();
	  if (strcmp("ABORT", plan_id.c_str()) == 0)  {
	      MOOSTrace("SENDING ABORT!\n");
	      PlanControl * msg = createStopPlanMessage();
	      sendToDune(msg);
	      free(msg);
	  }
	  else {
	      PlanControl * msg = createStartPlanMessage(plan_id);
	      sendToDune(msg);
	      free(msg);
	  }
      }
  }
  return(true);
}

bool ImcBridge::OnStartUp () {
  if (!m_MissionReader.GetConfigurationParam("DunePort", cfg_dune_port))
    MOOSTrace ("Warning parameter \"DunePort\" not specified. Using \"%d\"\n" , cfg_dune_port);

  if (!m_MissionReader.GetConfigurationParam("DuneHost", cfg_dune_host))
    MOOSTrace ("Warning parameter \"DuneHost\" not specified. Using \"%s\"\n" , cfg_dune_host.c_str());

  if (!m_MissionReader.GetConfigurationParam("LocalPort", cfg_local_port))
    MOOSTrace ("Warning parameter \"LocalPort\" not specified. Using \"%d\"\n" , cfg_local_port);

  if (!m_MissionReader.GetConfigurationParam("ImcId", cfg_imc_id))
    MOOSTrace ("Warning parameter \"ImcId\" not specified. Using \"%d\"\n" , cfg_imc_id);

  if (!m_MissionReader.GetValue("LatOrigin", cfg_lat_origin))
    MOOSTrace ("Warning parameter \"LatOrigin\" not specified. Using \"%d\"\n" , cfg_lat_origin);

  if (!m_MissionReader.GetValue("LongOrigin", cfg_lon_origin))
    MOOSTrace ("Warning parameter \"LongOrigin\" not specified. Using \"%d\"\n" , cfg_lon_origin);

  MOOSTrace ("Sending IMC data to %s:%d\n", cfg_dune_host.c_str(), cfg_dune_port);
  sock_send.connect(Address(cfg_dune_host.c_str()), cfg_dune_port);

  MOOSTrace ("Binding to port %d\n", cfg_local_port);
  return bind(cfg_local_port);
} 

bool ImcBridge::OnConnectToServer () {

  bool status = true;

  // "IVP" control
  status = status && Register("IVPHELM_STATE", 1.0);
  status = status && Register("DESIRED_SPEED", 1.0);
  status = status && Register("DESIRED_HEADING", 1.0);
  status = status && Register("DESIRED_DEPTH", 1.0);

  // "PLAN" control
  status = status && Register("DESIRED_PLAN_ID", 0);

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

  if (count > 0) {
      if (strcmp(ivp_helm_state.c_str(), "DRIVE") == 0 && strcmp(nav_control_mode.c_str(), "IVP") == 0) {
	  MOOSTrace("Send IVP reference!\n");
	  Reference * ref = createIVPReference(nav_lat, nav_lon, ivp_desired_heading, ivp_desired_speed, ivp_desired_depth);
	  sendToDune(ref);
	  free(ref);
      }
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

      nav_lat = Angles::degrees(lat);
      nav_lon = Angles::degrees(lon);

      double x, y;

      WGS84::displacement(Angles::radians(cfg_lat_origin), Angles::radians(cfg_lon_origin), 0, lat, lon, 0, &x, &y);

      Notify("NAV_X", y);
      Notify("NAV_Y", x);
      Notify("NAV_LAT", lat);
      Notify("NAV_LON", lon);
      Notify("NAV_SPEED", msg->u);
      Notify("NAV_DEPTH", msg->depth);
      Notify("NAV_HEADING", Angles::degrees(msg->psi));
  }
  else if (type == PlanControlState::getIdStatic()) {
      PlanControlState * msg = dynamic_cast<IMC::PlanControlState *>(message);
      nav_plan_id = "";
      if (msg->state == PlanControlState::PCS_EXECUTING) {
	  nav_plan_id = msg->plan_id;
	  if (msg->plan_id == IVP_PLAN_ID) {
	      nav_control_mode = "IVP";
	  }
	  else {
	      nav_control_mode = "PLAN";
	  }
      }
      else {
	  nav_control_mode = "PLAN";
      }
      Notify("NAV_CONTROL_MODE", nav_control_mode);
      Notify("NAV_PLAN_ID", nav_plan_id);
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
  return imcSend(msg, cfg_dune_host, cfg_dune_port);
}

bool ImcBridge::imcSend(Message * msg, std::string addr, int port) {
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

