//+------------------------------------------------------------------+
//| 1. 
//|                                  Copyright 2022, MetaQuotes Ltd. |
//|                                             https://www.mql5.com |
//+------------------------------------------------------------------+
#property copyright "Copyright 2022, MetaQuotes Ltd."
#property link      "https://www.mql5.com"
#include <Zmq/Zmq.mqh>
#include <JAson.mqh>


#define TDADDRSUB "tcp://localhost:5554"     // 交易订阅地址
#define TDADDRREQ "tcp://localhost:5555"     // 交易请求地址
#define MDADDRSUB "tcp://localhost:5556"     // 行情订阅地址
#define MDADDRREQ "tcp://localhost:5557"     // 行情请求地址


//--- 处理行情接口(被动订阅)
class CCtpMd
{
private:
   intptr_t    md_context;
   intptr_t    md_socket_sub;
   intptr_t    md_socket_req;
   CJAVal*     md_json_sub;
   CJAVal*     md_json_req;
   
   uchar       md_addr_sub[];
   uchar       md_addr_req[];

   uchar       md_recv_sub[2048];
   uchar       md_recv_req[128];
   uchar       md_send_req[2048];

public:
   CCtpMd()
   {
      ArrayInitialize(md_recv_sub,0);
      ArrayInitialize(md_recv_req,0);
      ArrayInitialize(md_send_req,0);
      
      md_context=zmq_ctx_new();
      md_socket_sub=zmq_socket(md_context,ZMQ_SUB);
      md_socket_req=zmq_socket(md_context,ZMQ_REQ);
      md_json_sub=new CJAVal(NULL, jtUNDEF);
      md_json_req=new CJAVal(NULL, jtUNDEF);
      
      StringToCharArray(MDADDRSUB,md_addr_sub);
      StringToCharArray(MDADDRREQ,md_addr_req);
      
   }
   ~CCtpMd()
   {
      MdZmqClose();
   }
   //--- 关闭zmq
   bool MdZmqClose()
   {
      zmq_unbind(md_socket_sub,md_addr_sub);
      zmq_disconnect(md_socket_sub,md_addr_sub);
      zmq_close(md_socket_sub);
      
      zmq_unbind(md_socket_req,md_addr_req);
      zmq_disconnect(md_socket_req,md_addr_req);
      zmq_close(md_socket_req);
      
      zmq_ctx_shutdown(md_context);
      zmq_ctx_term(md_context);

      Print("md_zmq Closed.");
      
      delete md_json_sub;
      md_json_sub=NULL;
      delete md_json_req;
      md_json_req=NULL;
      
      return(true);
   }
   
   //--- 连接zmq
   bool MdConnect()
   {
      Print("md_zmq Connected.");
      zmq_connect(md_socket_sub,md_addr_sub);
      zmq_connect(md_socket_req,md_addr_req);
      
      string tmp_str="";
      uchar tmp_char[];
      if(StringToCharArray(tmp_str,tmp_char)<=0) return(false);
      zmq_setsockopt(md_socket_sub,ZMQ_SUBSCRIBE,tmp_char,0);
      return(true);
   }
   
   bool SubscribeMarketData()
   {
      md_json_req.Clear();
      md_json_req["func"] = "SubscribeMarketData";
      md_json_req["params"] = "中文;rb2210";
      
      string md_data_req = "";
      md_json_req.Serialize(md_data_req);
      
      if(StringToCharArray(md_data_req,md_send_req)<0) return(false);
      zmq_send(md_socket_req,md_send_req,ArraySize(md_send_req),0);
      zmq_recv(md_socket_req,md_recv_req,ArraySize(md_recv_req),0);
      
      if(CharArrayToString(md_recv_req)!="yes") return(false);
      return(true);
   }
   
   bool OnRtnDepthMarketData()
   {
          // 以二进制形式接收数据
      int count=0;
      while(!IsStopped())
      {
         count +=1;
         if(count > 20)
            break;

         zmq_recv(md_socket_sub,md_recv_sub,ArraySize(md_recv_sub),0);
         md_json_sub.Deserialize(md_recv_sub);
         Print("数据量：",StringLen(CharArrayToString(md_recv_sub)));
         Print(md_json_sub["合约代码"].ToStr(),md_json_sub["最新价"].ToDbl()); 
      }
      return(true);
   }
};


//--- 处理交易接口(主动订阅)
class CCtpTd
{
private:
   intptr_t    td_context;
   intptr_t    td_socket_sub;
   intptr_t    td_socket_req;
   CJAVal*     td_json_req;
   CJAVal*     td_json_sub;
   
   uchar       td_addr_sub[];
   uchar       td_addr_req[];
   
   uchar       td_recv_sub[2048];
   uchar       td_recv_req[128];   // 一般只收到yes
   uchar       td_send_req[1024];
   
public:
   CCtpTd()
   {
      ArrayInitialize(td_recv_sub,0);
      ArrayInitialize(td_recv_req,0);
      ArrayInitialize(td_send_req,0);
      
      td_context=zmq_ctx_new();
      td_socket_sub=zmq_socket(td_context,ZMQ_SUB);
      td_socket_req=zmq_socket(td_context,ZMQ_REQ);
      
      td_json_sub=new CJAVal(NULL, jtUNDEF);
      td_json_req=new CJAVal(NULL, jtUNDEF);
      StringToCharArray(TDADDRSUB,td_addr_sub);
      StringToCharArray(TDADDRREQ,td_addr_req);
   }
   ~CCtpTd()
   {
      TdZmqClose();
   }
   bool TdZmqClose()
   {
      zmq_unbind(td_socket_sub,td_addr_sub);
      zmq_unbind(td_socket_req,td_addr_req);
      zmq_disconnect(td_socket_sub,td_addr_sub);
      zmq_disconnect(td_socket_req,td_addr_req);
      zmq_close(td_socket_sub);
      zmq_close(td_socket_req);
      zmq_ctx_shutdown(td_context);
      zmq_ctx_term(td_context);
      Print("td_zmq Closed.");
      delete td_json_sub;
      delete td_json_req;
      td_json_sub=NULL;
      td_json_req=NULL;
      return(true);
   }
   
   //--- 连接zmq
   bool TdConnect()
   {
      Print("td_zmq Connected.");
      zmq_connect(td_socket_sub,td_addr_sub);
      zmq_connect(td_socket_req,td_addr_req);
      
      string tmp_str="";
      uchar tmp_char[];
      if(StringToCharArray(tmp_str,tmp_char)<=0) return(false);
      zmq_setsockopt(td_socket_sub,ZMQ_SUBSCRIBE,tmp_char,0);
      
      return(true);
   }
   
   bool TdRequest(const CJAVal &req_json)
   {
      td_json_req.Copy(req_json);
      string td_data_req = "";
      td_json_req.Serialize(td_data_req);
      Print("发送数据:",td_data_req);
      if(StringToCharArray(td_data_req,td_send_req)<0) return(false);
      zmq_send(td_socket_req,td_send_req,ArraySize(td_send_req),0);
      zmq_recv(td_socket_req,td_recv_req,ArraySize(td_recv_req),0);
      if(CharArrayToString(td_recv_req)=="yes")
      {
         while(true)
         {
            zmq_recv(td_socket_sub,td_recv_sub,ArraySize(td_recv_sub),0);
            Print("长度:",StringLen(CharArrayToString(td_recv_sub)),"收到数据:",CharArrayToString(td_recv_sub));
            
            td_json_sub.Deserialize(CharArrayToString(td_recv_sub));
            Print("本地报单编号:",td_json_sub["本地报单编号"].ToInt());
            if(td_json_sub["bIsLast"].ToBool()==true) break;
         }

      };
      
      return(true);
   };
};