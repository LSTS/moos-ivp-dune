#include "Moos2Imc.hpp"
#include "IMCUtilities.h"

//! Latitude message id
static const char* c_msg_nav_lat        = "NAV_LAT";
//! Longitude message id
static const char* c_msg_nav_lon        = "NAV_LON";
//! X offset message id
static const char* c_msg_nav_x          = "NAV_X";
//! Y offset message id
static const char* c_msg_nav_y          = "NAV_Y";
//! Z offset char* message id
static const char* c_msg_nav_z          = "NAV_Z";
//! Roll message id
static const char* c_msg_nav_roll       = "NAV_ROLL";
//! Pitch message id
static const char* c_msg_nav_pitch      = "NAV_PITCH";
//! Yaw message id
static const char* c_msg_nav_hdg        = "NAV_HEADING";
//! Roll rate message id
static const char* c_msg_nav_roll_rate  = "NAV_ROLL_RATE";
//! Pitch rate message id
static const char* c_msg_nav_pitch_rate = "NAV_PITCH_RATE";
//! YAW rate message id
static const char* c_msg_nav_yaw_rate   = "NAV_YAW_RATE";
//! Speed, x-axis, message id
static const char* c_msg_nav_speed_x    = "NAV_SPEED";
//! Speed, y-axis, message id
static const char* c_msg_nav_speed_y    = "NAV_SPEED_Y";
//! Speed, z-axis, message id
static const char* c_msg_nav_speed_z    = "NAV_SPEED_Z";
//! Depth message id
static const char* c_msg_nav_depth      = "NAV_DEPTH";

Moos2Imc::Moos2Imc() {
  //defaults
  cfg_dune_port = 6002;
  cfg_imc_id = 65001;
  cfg_dune_host = "localhost";
  m_estate = new EstimatedState();

  new_nav_data = false;
}

Moos2Imc::~Moos2Imc()
{ }


bool Moos2Imc::OnNewMail (MOOSMSG_LIST & NewMail)
{
  MOOSMSG_LIST::iterator it;
  for (it = NewMail.begin(); it != NewMail.end(); ++it)
  {
    new_nav_data = true;
    CMOOSMsg &rMsg = *it;
    MOOSTrace("%s: %lf\n", rMsg.GetKey().c_str(), rMsg.m_dfVal);

    // Latitude
    if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_lat) == 0)
      m_estate->lat = rMsg.m_dfVal;

    // Longitude
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_lon) == 0)
      m_estate->lon = rMsg.m_dfVal;

    // X offset
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_x) == 0)
      m_estate->x = rMsg.m_dfVal;

    // Y offset
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_y) == 0)
      m_estate->y = rMsg.m_dfVal;

    // Z offset
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_z) == 0)
      m_estate->z = rMsg.m_dfVal;

    // Roll angle (rads)
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_roll) == 0)
      m_estate->phi = rMsg.m_dfVal;

    // Angular velocity in xx-axis
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_roll_rate) == 0)
      m_estate->p = rMsg.m_dfVal;

    // Pitch angle (rads)
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_pitch) == 0)
      m_estate->r = rMsg.m_dfVal;

    // Angular velocity in yy-axis
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_pitch_rate) == 0)
      m_estate->q = rMsg.m_dfVal;

    // Yaw angle (rads)
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_hdg) == 0)
      m_estate->psi = Angles::radians(rMsg.m_dfVal);

    // Angular velocity in zz-axis
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_yaw_rate) == 0)
      m_estate->r = rMsg.m_dfVal;

    // Body-Fixed xx Velocity
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_speed_x) == 0)
      m_estate->u = rMsg.m_dfVal;

    // Body-Fixed yy Velocity
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_speed_y) == 0)
      m_estate->v = rMsg.m_dfVal;

    // Body-Fixed zz Velocity
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_speed_z) == 0)
      m_estate->w = rMsg.m_dfVal;

    // Depth in meters
    else if (std::strcmp(rMsg.GetKey().c_str(), c_msg_nav_depth) == 0)
      m_estate->depth = rMsg.m_dfVal;
  }

  MOOSTrace("\n");

  return(true);
}

bool Moos2Imc::OnStartUp () {
  if (!m_MissionReader.GetConfigurationParam("DunePort", cfg_dune_port))
    MOOSTrace ("Warning parameter \"DunePort\" not specified. Using \"%d\"\n" , cfg_dune_port);

  if (!m_MissionReader.GetConfigurationParam("DuneHost", cfg_dune_host))
    MOOSTrace ("Warning parameter \"DuneHost\" not specified. Using \"%s\"\n" , cfg_dune_host.c_str());

  if (!m_MissionReader.GetConfigurationParam("ImcId", cfg_imc_id))
    MOOSTrace ("Warning parameter \"ImcId\" not specified. Using \"%d\"\n" , cfg_imc_id);

  MOOSTrace ("Sending IMC data to %s:%d\n", cfg_dune_host.c_str(), cfg_dune_port);
  sock_send.connect(Address(cfg_dune_host.c_str()), cfg_dune_port);

  return true;
}

bool Moos2Imc::OnConnectToServer () {

  bool status = true;

  // Position
  status = status && Register(c_msg_nav_lat, 1.0);
  status = status && Register(c_msg_nav_lon, 1.0);
  status = status && Register(c_msg_nav_x, 1.0);
  status = status && Register(c_msg_nav_y, 1.0);
  status = status && Register(c_msg_nav_z, 1.0);

  // Attitude
  status = status && Register(c_msg_nav_roll, 1.0);
  status = status && Register(c_msg_nav_roll_rate, 1.0);
  status = status && Register(c_msg_nav_pitch, 1.0);
  status = status && Register(c_msg_nav_pitch_rate, 1.0);
  status = status && Register(c_msg_nav_hdg, 1.0);
  status = status && Register(c_msg_nav_yaw_rate, 1.0);

  // Speed and depth
  status = status && Register(c_msg_nav_speed_x, 1.0);
  status = status && Register(c_msg_nav_speed_y, 1.0);
  status = status && Register(c_msg_nav_speed_z, 1.0);
  status = status && Register(c_msg_nav_depth, 1.0);

  return status;
}

bool Moos2Imc::Iterate ( )
{
  if (!new_nav_data)
    return true;

  m_estate->setSource(cfg_imc_id);
  if (!sendToDune(m_estate))
    MOOSTrace("ERR: Failed to send navigation data\n");

  new_nav_data = false;
  return true;
}

bool Moos2Imc::sendToDune(Message * msg) {
  return imcSend(msg, cfg_dune_host, cfg_dune_port);
}

bool Moos2Imc::imcSend(Message * msg, std::string addr, int port) {
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

