#include "ImcBridge.h"

ImcBridge::ImcBridge() {
  //defaults
  m_DunePort = 6002;
  m_LocalPort = 6969;
  m_ImcId = 65001;
  m_DuneHost = "localhost";
  bfr = new uint8_t[65535];
  iteration = 0;
}

ImcBridge::~ImcBridge() {
  unbind();
}


bool ImcBridge::OnNewMail (MOOSMSG_LIST & Mail) {
  // process it
  MOOSMSG_LIST::iterator q;
  for(q=Mail.begin(); q!=Mail.end(); q++) {
    q->Trace();
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

  MOOSTrace ("Binding to port %d...\n", m_LocalPort);

  return bind(m_LocalPort);
} 

bool ImcBridge::OnConnectToServer () {
  //return(Register("X", 0.0));
  return true;
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

  if (iteration % 10 == 0) {
    Announce * ann = new Announce();
    ann->sys_name = "MOOS";
    ann->sys_type = SYSTEMTYPE_STATICSENSOR;
    char services[128];
    sprintf(services, "imc+udp://192.168.0.101:%d", m_LocalPort);
    ann->services = services;
    sendToDune(ann);
    free(ann);
  }

  Heartbeat * beat = new Heartbeat();
  sendToDune(beat);
  free(beat);

  MOOSTrace ("%d messages received from DUNE\n", count);

  if (count == 0) {
      std :: vector<unsigned char> CONN(0);
      Notify("CONNECTED",CONN) ;
  }
  else {
      std :: vector<unsigned char> CONN(1);
      Notify("CONNECTED",CONN) ;
  }
  iteration++;
  return true ;
}

void ImcBridge::processMessage(Message * message) {
  //nothing for now
  MOOSTrace("Processing %s message\n", message->getName());
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
  sock_receive.bind(port, Address::Any, true);
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

