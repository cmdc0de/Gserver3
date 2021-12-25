#pragma once

#include <list>
#include <inet/tcp.h>
#include <inet/channel.h>

class AcceptCallable {
  public:
    AcceptCallable(wss::ListenerSocket *ls, uint32_t maxAccepts, std::list<wss::TCPComChannel*> *cl) : Listener(ls)
        , MaxAccepts(maxAccepts), ConnectionList(cl) {}
    void operator()() {
      getLogger()->info("AcceptCallable");
      wss::TCPServerSocket *out = nullptr;
      for(uint32_t i=0;i<MaxAccepts;++i) {
        if(Listener->getNewConnection(out)) {
          if(out!=nullptr) {
            ConnectionList->push_back(new wss::TCPComChannel(out));
          } else {
            break;
          }
        } else {
          //?????? close the listening socket?
          Listener->closeSocket();
          break;
        }
      }
    }
  private:
    wss::ListenerSocket *Listener;
    uint32_t MaxAccepts;
    std::list<wss::TCPComChannel*> *ConnectionList;
};

/*
class ReadCallable {
  public:
    ReadCallable();
    void operator() const {
    }
  private:
};

class SendAllData : tf::task {
  public:
    SendAllData();
    void operator() const {
    }
};

class SocketHandler {
  public:
    SocketHandler();
    wss::ErrorType init(wss::InetAddressV4 &addr, wss::PortNum &port, int32_t backlog);
    wss::ErrorType setup(tf::Taskflow &f) {
      AcceptCallable ac;
      std::list<ws::TCPCommChannel*> &cl = SHandler->getConnectionList();
      tf::Task tAccept = t.for_each(cl.begin(),cl.end(),ac);
      ReadCallable rc;
      tf::Task tRead =   t.for_each(cl.begin(),cl.end(),rc);
    }
    wss::ErrorType sendAll();
    std::list<wss::TcpCommChannel*> &getConnectionList() {return ConnectionList;}
  private:
    std::list<wss::TcpCommChannel*> ConnectionList;
    ListenerSocket Listener;
    AcceptAndReadTask AR;

};
*/

