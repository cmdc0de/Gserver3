#pragma once

#include <list>
#include <inet/tcp.h>
#include <inet/channel.h>
#include <queue>

class GameTCPChannel : public wss::TCPComChannel {
public:
  static const uint32_t MAX_TEXT_COMMAND = 127;
public:
  GameTCPChannel(wss::TCPServerSocket *ss) : wss::TCPComChannel(ss) {}
  bool parseTextCommand(std::string &ret) {
    bool delimHit = false;
    char buf[MAX_TEXT_COMMAND+1];
    size_t bytes = getIncomingBuffer().readUntilDelim(&buf[0],sizeof(buf),'\n', delimHit);
    if(delimHit) {
      buf[bytes] = '\0';
      ret.append(&buf[0]);
      getIncomingBuffer().erase(0,bytes);
    }
    return delimHit;
  }
  private:
};

class TextConnection {
  public:
    typedef std::shared_ptr<TextConnection> TextConPtr;
    typedef std::shared_ptr<GameTCPChannel> TextComChannel;
    typedef std::queue<std::string> CommandQueue;
  public:
    void processCommand() {
      std::string s = MyCommandQueue.front();
      if(s=="connect") {
        getLogger()->debug("connect");
      } else {
        getLogger()->debug("unknown");
      }
      MyCommandQueue.pop();
    }
  public:
    TextConnection(const TextComChannel &p) : mChannel(p), MyCommandQueue() {}
    wss::ErrorType processInput() {
      wss::ErrorType et;
      int32_t bytesRead = mChannel.get()->bufferIn();
      if(0==bytesRead) {
        mChannel->setDeath();
        getLogger()->debug("setting Socket: {} for death", static_cast<void*>(mChannel.get()));
        et =  mChannel->getLastSocketError();
      } else if (-1==bytesRead){
        //ewouldblock
      } else {
        getLogger()->debug("Socket: {} read {} bytes", static_cast<void*>(mChannel.get()), bytesRead);
        size_t numCommands = processInCommingBufferEnqueCommands();
        std::ignore = numCommands;
      }
      return et;
    }
    bool markedForDeath() {return mChannel->markedForDeath();}
  protected:
    size_t processInCommingBufferEnqueCommands() {
      std::string command;
      while(mChannel->parseTextCommand(command)) {
        MyCommandQueue.push(command);
        command.clear();
      }
      return MyCommandQueue.size();
    }
  private:
    TextComChannel mChannel;
    TextConnection::CommandQueue MyCommandQueue;
};

class AcceptTextCallable {
  public:
    AcceptTextCallable(wss::ListenerSocket *ls, uint32_t maxAccepts, std::list<TextConnection::TextConPtr> *cl) 
      : Listener(ls), MaxAccepts(maxAccepts), ConnectionList(cl) {}
    void operator()() {
      wss::TCPServerSocket *out = nullptr;
      for(uint32_t i=0;i<MaxAccepts;++i) {
        if(Listener->getNewConnection(out)) {
          if(out!=nullptr) {
            std::shared_ptr<GameTCPChannel> gtc(new GameTCPChannel(out));
            ConnectionList->push_back(TextConnection::TextConPtr(new TextConnection(gtc)));
            getLogger()->info("new Socket!");
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
    std::list<TextConnection::TextConPtr> *ConnectionList;
};

class SocketReadCallable {
  public:
    SocketReadCallable() {}
    void operator()(TextConnection::TextConPtr &p) const {
      p->processInput();
    }
  private:
};


/*
class DeadSocketCallable {
  public:
    DeadSocketCallable(): DeathList() {}
    void operator()(std::list<wss::TCPComChannel *>::const_iterator &p) const {
      if((*p)->markedForDeath()) {
        DeathList.push_back(p);
      }
    }
  private:
    std::list<std::list<wss::TCPComChannel*>::const_iterator> DeathList;
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

