# ReqRep

source: `{{ page.path }}`

MT5_Client

```cpp
#include <Zmq/Zmq.mqh>
//+------------------------------------------------------------------+
//| Hello World client in MQL                                        |
//| Connects REQ socket to tcp://localhost:5555                      |
//| Sends "Hello" to server, expects "World" back                    |
//+------------------------------------------------------------------+
void OnStart()
  {
// Prepare our context and socket
   Context context("helloworld");
   Socket socket(context,ZMQ_REQ);

   Print("Connecting to hello world server…");
   socket.connect("tcp://localhost:5555");

// Do 10 requests, waiting each time for a response
   for(int request_nbr=0; request_nbr!=10 && !IsStopped(); request_nbr++)
     {
      ZmqMsg request("Hello");
      PrintFormat("Sending Hello %d...",request_nbr);
      socket.send(request);

      // Get the reply.
      ZmqMsg reply;
      socket.recv(reply);
      PrintFormat("Received World %d",request_nbr);
     }
  }
```

CPP_Server

```cpp
#include <zmq.hpp>
#include <iostream>
#include <thread>


int main()
{
    // initialize the zmq context with a single IO thread
    zmq::context_t context{ 1 };

    // construct a REP (reply) socket and bind to interface
    zmq::socket_t socket{ context, zmq::socket_type::rep };
    socket.bind("tcp://*:5555");

    // prepare some static data for responses
    const std::string data{ "World" };

    for (;;)
    {
        zmq::message_t request;

        // receive a request from client
        socket.recv(request, zmq::recv_flags::none);
        std::cout << "Received " << request.to_string() << std::endl;

        // simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // send the reply to the client
        socket.send(zmq::buffer(data), zmq::send_flags::none);
    }

    return 0;
}
```

MT5_Server

```cpp
#include <Zmq/Zmq.mqh>
//+------------------------------------------------------------------+
//| Hello World server in MQL                                        |
//| Binds REP socket to tcp://*:5555                                 |
//| Expects "Hello" from client, replies with "World"                |
//+------------------------------------------------------------------+
void OnStart()
  {
   Context context("helloworld");
   Socket socket(context,ZMQ_REP);

   socket.bind("tcp://*:5555");

   while(!IsStopped())
     {
      ZmqMsg request;

      socket.recv(request);
      Print("Receive Hello");

      Sleep(1000);

      ZmqMsg reply("World");
      // Send reply back to client
      socket.send(reply);
     }
  }
```

CPP_Client

```cpp
#include <zmq.hpp>
#include <iostream>
#include <thread>


int main()
{
    // initialize the zmq context with a single IO thread
    zmq::context_t context{ 1 };

    // construct a REQ (request) socket and connect to interface
    zmq::socket_t socket{ context, zmq::socket_type::req };
    socket.connect("tcp://localhost:5555");

    // set up some static data to send
    const std::string data{ "Hello" };

    for (auto request_num = 0; request_num < 10; ++request_num)
    {
        // send the request message
        std::cout << "Sending Hello " << request_num << "..." << std::endl;
        socket.send(zmq::buffer(data), zmq::send_flags::none);

        // wait for reply from server
        zmq::message_t reply{};
        socket.recv(reply, zmq::recv_flags::none);

        std::cout << "Received " << reply.to_string();
        std::cout << " (" << request_num << ")";
        std::cout << std::endl;
    }

    return 0;
}
```