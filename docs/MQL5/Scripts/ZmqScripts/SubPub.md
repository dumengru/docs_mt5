# SubPub

source: `{{ page.path }}`

MT5_Client

```cpp
input string InpZipCode="10001"; // ZipCode to subscribe to, default is NYC, 10001

#include <Zmq/Zmq.mqh>
//+------------------------------------------------------------------+
//| Script program start function                                    |
//+------------------------------------------------------------------+
void OnStart()
  {
   Context context;

//  Socket to talk to server
   Print("Collecting updates from weather server…");
   Socket subscriber(context,ZMQ_SUB);
   subscriber.connect("tcp://localhost:5556");

   subscriber.subscribe(InpZipCode);

//  Process 100 updates
   int update_nbr;
   long total_temp=0;
   for(update_nbr=0; update_nbr<100; update_nbr++)
     {
      ZmqMsg update;
      long zipcode,temperature,relhumidity;

      subscriber.recv(update);

      string msg=update.getData();
      string msg_array[];
      StringSplit(msg,' ',msg_array);
      zipcode=StringToInteger(msg_array[0]);
      temperature = StringToInteger(msg_array[1]);
      relhumidity = StringToInteger(msg_array[2]);
      total_temp+=temperature;
     }
   PrintFormat("Average temperature for zipcode '%s' was %dF",
               InpZipCode,(int)(total_temp/update_nbr));
  }
//+------------------------------------------------------------------+
```

CPP_Server

```cpp
#include <zmq.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#if (defined (WIN32))
#include <zhelpers.hpp>
#endif

#define within(num) (int) ((float) num * rand() / (RAND_MAX + 1.0))

int main() {

    //  Prepare our context and publisher
    zmq::context_t context(1);
    zmq::socket_t publisher(context, zmq::socket_type::pub);
    publisher.bind("tcp://*:5556");
    publisher.bind("ipc://weather.ipc");				// Not usable on Windows.

    //  Initialize random number generator
    srand((unsigned)time(NULL));
    while (1) {

        int zipcode, temperature, relhumidity;

        //  Get values that will fool the boss
        zipcode = within(11000);
        temperature = within(215) - 80;
        relhumidity = within(50) + 10;

        if (zipcode == 10001)
        {
            std::cout << zipcode << std::endl;
        }
        
        //  Send message to all subscribers
        zmq::message_t message(20);
        snprintf((char*)message.data(), 20,
            "%05d %d %d", zipcode, temperature, relhumidity);
        publisher.send(message, zmq::send_flags::none);

    }
    return 0;
}
```

MT5_Server

```cpp
#include <Zmq/Zmq.mqh>

#define within(num) (int) ((float) num * MathRand() / (32767 + 1.0))
//+------------------------------------------------------------------+
//| Weather update server in MQL                                     |
//| Binds PUB socket to tcp://*:5556                                 |
//| Publishes random weather updates                                 |
//+------------------------------------------------------------------+
void OnStart()
  {
//--- Prepare our context and publisher
   Context context;
   Socket publisher(context,ZMQ_PUB);
   publisher.bind("tcp://*:5556");

   long messages_sent=0;
//--- Initialize random number generator
   MathSrand(GetTickCount());
   while(!IsStopped())
     {
      int zipcode,temperature,relhumidity;

      zipcode=within(11000);
      temperature=within(215) - 80;
      relhumidity=within(50) + 10;

      // Send message to all subscribers
      ZmqMsg message(StringFormat("%05d %d %d",zipcode,temperature,relhumidity));
      publisher.send(message);
      messages_sent++;

      if(zipcode==10001)
      {
         Print(zipcode);
      }
     }
  }
```

CPP_Client

```cpp
#include <zmq.hpp>
#include <iostream>
#include <sstream>


int main (int argc, char *argv[])
{
    zmq::context_t context (1);

    //  Socket to talk to server
    std::cout << "Collecting updates from weather server...\n" << std::endl;
    zmq::socket_t subscriber (context, zmq::socket_type::sub);
    subscriber.connect("tcp://localhost:5556");

    //  Subscribe to zipcode, default is NYC, 10001
    const char *filter = (argc > 1)? argv [1]: "10001";
    int rc = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, filter, strlen (filter));
    assert(rc == 0);

    //  Process 100 updates
    int update_nbr;
    long total_temp = 0;
    for (update_nbr = 0; update_nbr < 100; update_nbr++) {

        zmq::message_t update;
        int zipcode, temperature, relhumidity;

        subscriber.recv(update, zmq::recv_flags::none);

        std::istringstream iss(static_cast<char*>(update.data()));
		iss >> zipcode >> temperature >> relhumidity ;

        std::cout << zipcode << std::endl;

		total_temp += temperature;
    }
    std::cout 	<< "Average temperature for zipcode '"<< filter
    			<<"' was "<<(int) (total_temp / update_nbr) <<"F"
    			<< std::endl;
    return 0;
}
```