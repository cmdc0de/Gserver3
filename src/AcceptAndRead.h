#pragma once

class SocketHandler {
  public:
    SocketHandler();
    wss::ErrorType init(wss::InetAddressV4 &addr, wss::PortNum &port, int32_t backlog);
    wss::ErrorType AcceptAndRead();
    wss::ErrorType sendAll();
    std::list<wss::TcpCommChannel*> &getConnectionList() {return ConnectionList;}
  private:
    std::list<wss::TcpCommChannel*> ConnectionList;

};

class AcceptAndReadTask : tf::task {
  public:
    AcceptAndReadTask(std::share_ptr<SocketHandler> &sh);
    void operator() const {
      //for_each?
      std::list<ws::TCPCommChannel*> &cl = sh->getConnectionList();
      std::ignore = cl;
    }
};

class SendAllData : tf::task {
  public:
    SendAllData();
    void operator() const {
    }
};
