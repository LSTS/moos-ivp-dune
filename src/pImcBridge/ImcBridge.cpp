#include "ImcBridge.h"

ImcBridge::ImcBridge() {
  //defaults
  m_DunePort = 6002;
  m_DuneHost = "localhost";
}

ImcBridge::~ImcBridge() {
  // empty
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

} 

bool ImcBridge::OnConnectToServer () {
  //return(Register("X", 0.0));
	return true;
} 
  
bool ImcBridge::Iterate ( ) {
  //std :: vector<unsigned char> X(100) ;
  //Notify("X" ,X) ;

  Message * msg;

  while ((msg = imcPoll()) != NULL) {
      std::cout << "Received message of type " << msg->getName() << std::endl;
      free(msg);
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

bool ImcBridge::sendToDune(Message * msg) {
  return imcSend(msg, m_DuneHost, m_DunePort);
}

bool ImcBridge::imcSend(Message * msg, std::string addr, int port) {
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

