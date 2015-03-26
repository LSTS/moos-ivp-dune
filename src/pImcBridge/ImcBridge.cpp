#include "ImcBridge.h"

ImcBridge::ImcBridge() {
  //defaults
  m_DunePort = 6002;
  m_LocalPort = 6969;
  m_ImcId = 65001;
  m_DuneHost = "localhost";
  bfr = new uint8_t[65535];
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
  bind(m_LocalPort);
} 

bool ImcBridge::OnConnectToServer () {
  //return(Register("X", 0.0));
  MOOSTrace ("OnConnect\n");
  return true;
} 
  
bool ImcBridge::Iterate ( ) {
  //std :: vector<unsigned char> X(100) ;
  //
  Message * msg;

  int count = 0;
  while ((msg = imcPoll()) != NULL) {
      MOOSTrace ("Received message of type %s\n", msg->getName());
      free(msg);
      count ++;
  }
  Abort * ab = new Abort();
  MOOSTrace ("%lf\n", ab->getTimeStamp());
  sendToDune(ab);

  MOOSTrace ("%d messages received from DUNE\n", count);

  if (count == 0) {
      std :: vector<unsigned char> CONN(0);
      Notify("CONNECTED",CONN) ;
  }
  else {
      std :: vector<unsigned char> CONN(1);
      Notify("CONNECTED",CONN) ;
  }

  return true ;
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
    std::cerr << "ERROR: " << ": " << e.what() << "\n";
    return false;
  }
  return true;
}

