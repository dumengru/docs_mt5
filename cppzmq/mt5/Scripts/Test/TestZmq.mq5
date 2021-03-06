//+------------------------------------------------------------------+
//|                                                      TestZmq.mq4 |
//|                                          Copyright 2016, Li Ding |
//|                                            dingmaotu@hotmail.com |
//+------------------------------------------------------------------+
#property copyright "Copyright 2016, Li Ding"
#property link      "dingmaotu@hotmail.com"
#property version   "1.00"
#property strict

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
   subscriber.connect("tcp://localhost:5554");
   subscriber.subscribe("");

   int update_nbr;
   
   // 以二进制形式接收数据
   uchar bytes[];
   ZmqMsg msg_recv;
   string   data_msg;
   for(update_nbr=0; update_nbr<1; update_nbr++)
   {
      subscriber.recv(msg_recv);
      msg_recv.getData(bytes);
      
      data_msg=CharArrayToString(bytes, 0, -1, CP_ACP);
      Print(data_msg);
      
      msg_recv.rebuild();
   }

   context.destroy(0);
   subscriber.unbind("tcp://localhost:5556");
   subscriber.disconnect("tcp://localhost:5556");
   
  }
//+------------------------------------------------------------------+
