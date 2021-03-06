//+------------------------------------------------------------------+
//|                                                       CtpZmq.mq5 |
//|                                  Copyright 2022, MetaQuotes Ltd. |
//|                                             https://www.mql5.com |
//+------------------------------------------------------------------+
#property copyright "Copyright 2022, MetaQuotes Ltd."
#property link      "https://www.mql5.com"
#property version   "1.00"

#include <CtpZmq.mqh>

//+------------------------------------------------------------------+
//| Script program start function                                    |
//+------------------------------------------------------------------+

CCtpMd CtpMd;
CCtpTd CtpTd;
CJAVal ReqJson(NULL, jtUNDEF);;


void FuncMd()
{
   CtpMd.MdConnect();
   CtpMd.SubscribeMarketData();
   CtpMd.OnRtnDepthMarketData();
}

void FuncTd()
{
   CtpTd.TdConnect();


/*
   ReqJson.Clear();
   ReqJson["func"]="ReqOrderInsert";
   ReqJson["params"]["InstrumentID"]="rb2210";
   ReqJson["params"]["ExchangeID"]="SHFE";
   ReqJson["params"]["Direction"]=1;
   ReqJson["params"]["CombOffsetFlag"]=1;
   ReqJson["params"]["VolumeTotalOriginal"]=1;
   ReqJson["params"]["LimitPrice"]=5555;
   CtpTd.TdRequest(ReqJson);

   ReqJson.Clear();
   ReqJson["func"]="ReqQryOrder";
   ReqJson["params"]["ExchangeID"]="SHFE";
   CtpTd.TdRequest(ReqJson);


   ReqJson.Clear();
   ReqJson["func"]="ReqOrderInsert";
   ReqJson["params"]["InstrumentID"]="rb2210";
   ReqJson["params"]["ExchangeID"]="SHFE";
   ReqJson["params"]["Direction"]=1;
   ReqJson["params"]["CombOffsetFlag"]=1;
   ReqJson["params"]["VolumeTotalOriginal"]=1;
   ReqJson["params"]["LimitPrice"]=5555;
   CtpTd.TdRequest(ReqJson);
   
   Sleep(3);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqOrderAction";
   ReqJson["params"]["InstrumentID"]="rb2210";
   ReqJson["params"]["ExchangeID"]="SHFE";
   ReqJson["params"]["OrderSysID"]="95";
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQryDepthMarketData";
   ReqJson["params"]["InstrumentID"]="rb2210";
   CtpTd.TdRequest(ReqJson);

   ReqJson.Clear();
   ReqJson["func"]="ReqQryExchange";
   ReqJson["params"]["ExchangeID"]="SHFE";
   CtpTd.TdRequest(ReqJson);

   ReqJson.Clear();
   ReqJson["func"]="ReqQryExchangeMarginRate";
   ReqJson["params"]["InstrumentID"]="rb2210";
   CtpTd.TdRequest(ReqJson);
  
   ReqJson.Clear();
   ReqJson["func"]="ReqQryInstrument";
   ReqJson["params"]["InstrumentID"]="rb2210";
   CtpTd.TdRequest(ReqJson);
      
   ReqJson.Clear();
   ReqJson["func"]="ReqQryInstrumentCommissionRate";
   ReqJson["params"]["InstrumentID"]="rb2210";
   CtpTd.TdRequest(ReqJson);
      
   ReqJson.Clear();
   ReqJson["func"]="ReqQryInstrumentMarginRate";
   ReqJson["params"]["InstrumentID"]="rb2210";
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQryInstrumentOrderCommRate";
   ReqJson["params"]["InstrumentID"]="rb2210";
   CtpTd.TdRequest(ReqJson);

   ReqJson.Clear();
   ReqJson["func"]="ReqQryInvestor";
   ReqJson["params"]["1"]=1;
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQryInvestorPosition";
   ReqJson["params"]["InstrumentID"]="rb2210";
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQryInvestorPositionCombineDetail";
   ReqJson["params"]["CombInstrumentID"]="1";
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQryInvestorPositionDetail";
   ReqJson["params"]["InstrumentID"]="rb2210";
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQryInvestUnit";
   ReqJson["params"]["1"]="1";
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQryOrder";
   ReqJson["params"]["ExchangeID"]="SHFE";
   CtpTd.TdRequest(ReqJson);

   ReqJson.Clear();
   ReqJson["func"]="ReqQryProduct";
   ReqJson["params"]["ProductID"]="IC";
   CtpTd.TdRequest(ReqJson);

   ReqJson.Clear();
   ReqJson["func"]="ReqQryTrade";
   ReqJson["params"]["InstrumentID"]="rb2210";
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQryTradingAccount";
   ReqJson["params"][""]="";
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQryTradingCode";
   ReqJson["params"]["ExchangeID"]="SHFE";
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQryTradingNotice";
   ReqJson["params"][""]="";
   CtpTd.TdRequest(ReqJson);
   
   ReqJson.Clear();
   ReqJson["func"]="ReqQueryMaxOrderVolume";
   ReqJson["params"]["InstrumentID"]="rb2210";
   ReqJson["params"]["Direction"]=1;
   ReqJson["params"]["OffsetFlag"]=1;
   ReqJson["params"]["HedgeFlag"]=1;
   CtpTd.TdRequest(ReqJson);

/*
     
   ReqJson.Clear();
   ReqJson["func"]="";
   ReqJson["params"][""]="";
   CtpTd.TdRequest(ReqJson);
   
*/
}


void OnStart()
  {
//---
   FuncTd();
   // FuncMd();
  }
//+------------------------------------------------------------------+
