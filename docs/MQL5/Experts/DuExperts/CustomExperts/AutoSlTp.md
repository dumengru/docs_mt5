# AutoSlTp.mq5

source: `{{ page.path }}`

自动监控持仓的EA
1. 只交易指定品种
2. 自动添加止损止盈
3. 强制平仓其他品种和修改过大止损

```cpp
//+------------------------------------------------------------------+
//|                                                     AutoSlTp.mq5 |
//|                                  Copyright 2022, MetaQuotes Ltd. |
//|                                             https://www.mql5.com |
//+------------------------------------------------------------------+
#property copyright "Copyright 2022, MetaQuotes Ltd."
#property link      "https://www.mql5.com"
#property version   "1.00"
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
#include <Trade\Trade.mqh>

input string TSymbol="XAUUSD";
input double TStopTrailing=300;
input double TTakeProfit=500;

CPositionInfo ExtPos;
CTrade ExtTrade;

/*
1. 检查是否有持仓, 如果持仓大于1, 平掉新开仓只留1手
2. 检查是否有止损, 如果无止损, 添加止损300(止盈500), 如果止损大于300, 改为300
*/

void CheckPosition()
{
   uint total=PositionsTotal();
   datetime old_time=UINT_MAX;
   ulong old_ticket=UINT_MAX;
   
   for(uint i=0; i<total; i++)
     {
      string position_symbol=PositionGetSymbol(i);
      if(position_symbol!=TSymbol)
      {
         //--- 如果不是交易品种, 直接平仓
         ExtTrade.PositionClose(position_symbol);
      }
      else
      {
         ExtPos.SelectByIndex(i);
         if(old_time==UINT_MAX || old_ticket==UINT_MAX)
         {
            //--- 第一笔订单
            old_time=ExtPos.Time();
            old_ticket=ExtPos.Ticket();
         }
         else
         {
            //--- 还有其他订单
            if(old_time<ExtPos.Time())
            {
               // 如果旧订单时间更早, 平掉新订单
               ExtTrade.PositionClose(ExtPos.Ticket());
            }
            else
            {
               // 如果旧订单时间晚, 平掉旧订单
               ExtTrade.PositionClose(old_ticket);
               old_ticket=ExtPos.Ticket();
               old_time=ExtPos.Time();
            }
         }
      }
   }
}

void CheckSl()
{
   
   uint total=PositionsTotal();
   for(uint i=0; i<total; i++)
   {
      ExtPos.SelectByIndex(i);
      double sl=0.0;
      double tp=0.0;
      if(ExtPos.PositionType()==POSITION_TYPE_BUY)
      {
         sl=NormalizeDouble(ExtPos.PriceOpen()-TStopTrailing*Point(),2);
         tp=NormalizeDouble(ExtPos.PriceOpen()+TTakeProfit*Point(),2);
         // 无止损或止损过大
         if(ExtPos.StopLoss()==0.0 || ExtPos.StopLoss()<sl)
            if(ExtTrade.PositionModify(ExtPos.Ticket(),sl,tp))
               PrintFormat("修改 sl: %d, tp: %d", sl, tp);
      }
      else
      {
         sl=NormalizeDouble(ExtPos.PriceOpen()+TStopTrailing*Point(),2);
         tp=NormalizeDouble(ExtPos.PriceOpen()-TTakeProfit*Point(),2);
         // 无止损或止损过大
         if(ExtPos.StopLoss()==0.0 || ExtPos.StopLoss()>sl)
            if(ExtTrade.PositionModify(ExtPos.Ticket(),sl,tp))
               PrintFormat("修改 sl: %d, tp: %d", sl, tp);
      }   
   }
}


int OnInit(void)
  {
   ExtTrade.SetMarginMode();
   ExtTrade.SetTypeFillingBySymbol(Symbol());
//--- ok
   return(INIT_SUCCEEDED);
  }
//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick(void)
  {
//---
   CheckPosition();
   CheckSl();
//---
  }
//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
  {
  }
//+------------------------------------------------------------------+
```