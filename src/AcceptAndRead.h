#pragma once

class AcceptCallable {
  public:
    AcceptCallable();
    void operator() const {
    }
  private:
};

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

