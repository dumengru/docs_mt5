# MovingAverage注释

source: `{{ page.path }}`

可以根据连续亏损次数调整下单量

## EA流程

1.导入头文件

```cpp
#include <Trade\Trade.mqh>
```

2.输入参数

```cpp
input double MaximumRisk        = 0.02;    // 最大风险比例
input double DecreaseFactor     = 3;       // 衰减因子
input int    MovingPeriod       = 12;      // MA周期参数
input int    MovingShift        = 6;       // MA平移参数
```

3.全局变量

```cpp
int    ExtHandle=0;                       // 指标句柄
bool   ExtHedging=false;                  // 账户模式
CTrade ExtTrade;                          // 交易对象
```

4.创建MAGIC

```cpp
#define MA_MAGIC 1234501
```

5.OnInit

```cpp
int OnInit(void)
  {
//--- prepare trade class to control positions if hedging mode is active
   ExtHedging=((ENUM_ACCOUNT_MARGIN_MODE)AccountInfoInteger(ACCOUNT_MARGIN_MODE)==ACCOUNT_MARGIN_MODE_RETAIL_HEDGING);
   ExtTrade.SetExpertMagicNumber(MA_MAGIC);     // 设置MAGIC
   ExtTrade.SetMarginMode();                    // 根据账户设置保证金计算模式
   ExtTrade.SetTypeFillingBySymbol(Symbol());   // 根据品种设置下单模式
//--- 创建MA指标
   ExtHandle=iMA(_Symbol,_Period,MovingPeriod,MovingShift,MODE_SMA,PRICE_CLOSE);
   if(ExtHandle==INVALID_HANDLE)
     {
      printf("Error creating MA indicator");
      return(INIT_FAILED);
     }
//--- ok
   return(INIT_SUCCEEDED);
  }
```

6.OnTick

```cpp
void OnTick(void)
  {
//--- 1. 选择持仓, 2. 检查是否平仓, 3. 检查是否开仓
   if(SelectPosition())
      CheckForClose();
   else
      CheckForOpen();
  }
```

## 主函数

1.TradeSizeOptimized

```cpp
double TradeSizeOptimized(void)
  {
   double price=0.0;
   double margin=0.0;
//--- 1. 获取最新价, 2. 计算1手的保证金
   if(!SymbolInfoDouble(_Symbol,SYMBOL_ASK,price))
      return(0.0);
   if(!OrderCalcMargin(ORDER_TYPE_BUY,_Symbol,1.0,price,margin))
      return(0.0);
   if(margin<=0.0)
      return(0.0);

//--- 根据输入风险参数计算下单量
   double lot=NormalizeDouble(AccountInfoDouble(ACCOUNT_MARGIN_FREE)*MaximumRisk/margin,2);
//--- calculate number of losses orders without a break
   if(DecreaseFactor>0)
     {
      //--- 1. 选择所有历史订单
      HistorySelect(0,TimeCurrent());
      //---
      int    orders=HistoryDealsTotal();  // 2. 获取全部历史成交单数量
      int    losses=0;                    // 记录亏损成交单数量
      //--- 3. 遍历全部成交单, 统计亏损成交单数量
      for(int i=orders-1;i>=0;i--)
        {
         //--- 3.1 选择一笔历史成交单
         ulong ticket=HistoryDealGetTicket(i);
         if(ticket==0)
           {
            Print("HistoryDealGetTicket failed, no trade history");
            break;
           }
         //--- 3.2 品种过滤
         if(HistoryDealGetString(ticket,DEAL_SYMBOL)!=_Symbol)
            continue;
         //--- 3.3 MAGIC过滤
         if(HistoryDealGetInteger(ticket,DEAL_MAGIC)!=MA_MAGIC)
            continue;
         //--- 3.4 计算成交单利润
         double profit=HistoryDealGetDouble(ticket,DEAL_PROFIT);
         //--- 3.5 利润大于0退出, 否则loss++
         if(profit>0.0)
            break;
         if(profit<0.0)
            losses++;
        }
      //--- 4. 如果连续亏损超过2笔, lot根据衰减因子减仓(可能减至<=0)
      if(losses>1)
         lot=NormalizeDouble(lot-lot*losses/DecreaseFactor,1);
     }
//--- 5. 调整下单量: 步长值, 最小值, 最大值
   double stepvol=SymbolInfoDouble(_Symbol,SYMBOL_VOLUME_STEP);
   lot=stepvol*NormalizeDouble(lot/stepvol,0);

   double minvol=SymbolInfoDouble(_Symbol,SYMBOL_VOLUME_MIN);
   if(lot<minvol)
      lot=minvol;

   double maxvol=SymbolInfoDouble(_Symbol,SYMBOL_VOLUME_MAX);
   if(lot>maxvol)
      lot=maxvol;
//--- return trading volume
   return(lot);
  }
//+------------------------------------------------------------------+
//| Check for open position conditions                               |
//+------------------------------------------------------------------+
void CheckForOpen(void)
  {
   MqlRates rt[2];
//--- 1. 只获取新Bar的第一个Tick
   if(CopyRates(_Symbol,_Period,0,2,rt)!=2)
     {
      Print("CopyRates of ",_Symbol," failed, no history");
      return;
     }
//--- 1.1 交易量>1表示不是新Bar(新Bar的第一个tick_volume应该=0)
   if(rt[1].tick_volume>1)
      return;
//--- 2. 获取MA指标值
   double   ma[1];
   if(CopyBuffer(ExtHandle,0,0,1,ma)!=1)
     {
      Print("CopyBuffer from iMA failed, no data");
      return;
     }
//--- 3. 检查信号
   ENUM_ORDER_TYPE signal=WRONG_VALUE;
//--- 3.1 卖出/买入条件
   if(rt[0].open>ma[0] && rt[0].close<ma[0])
      signal=ORDER_TYPE_SELL;    // sell conditions
   else
     {
      if(rt[0].open<ma[0] && rt[0].close>ma[0])
         signal=ORDER_TYPE_BUY;  // buy conditions
     }
//--- 4. 开仓
   if(signal!=WRONG_VALUE)
     {
      if(TerminalInfoInteger(TERMINAL_TRADE_ALLOWED) && Bars(_Symbol,_Period)>100)
         ExtTrade.PositionOpen(_Symbol,signal,TradeSizeOptimized(),
                               SymbolInfoDouble(_Symbol,signal==ORDER_TYPE_SELL ? SYMBOL_BID:SYMBOL_ASK),
                               0,0);
     }
  }
//+------------------------------------------------------------------+
//| Check for close position conditions                              |
//+------------------------------------------------------------------+
void CheckForClose(void)
  {
   MqlRates rt[2];
//--- go trading only for first ticks of new bar
   if(CopyRates(_Symbol,_Period,0,2,rt)!=2)
     {
      Print("CopyRates of ",_Symbol," failed, no history");
      return;
     }
   if(rt[1].tick_volume>1)
      return;
//--- get current Moving Average 
   double   ma[1];
   if(CopyBuffer(ExtHandle,0,0,1,ma)!=1)
     {
      Print("CopyBuffer from iMA failed, no data");
      return;
     }
//--- positions already selected before
   bool signal=false;
   long type=PositionGetInteger(POSITION_TYPE);

   if(type==(long)POSITION_TYPE_BUY && rt[0].open>ma[0] && rt[0].close<ma[0])
      signal=true;
   if(type==(long)POSITION_TYPE_SELL && rt[0].open<ma[0] && rt[0].close>ma[0])
      signal=true;
//--- additional checking
   if(signal)
     {
      if(TerminalInfoInteger(TERMINAL_TRADE_ALLOWED) && Bars(_Symbol,_Period)>100)
         ExtTrade.PositionClose(_Symbol,3);
     }
//---
  }
//+------------------------------------------------------------------+
//| Position select depending on netting or hedging                  |
//+------------------------------------------------------------------+
bool SelectPosition()
  {
   bool res=false;
//--- 1. 判断账户类型
//--- 1.1 对冲账户
   if(ExtHedging)
     {
      //--- 2. 遍历全部持仓
      uint total=PositionsTotal();
      for(uint i=0; i<total; i++)
        {
         //--- 3. 对冲账户需要名称和MAGIC双重确认
         string position_symbol=PositionGetSymbol(i);
         if(_Symbol==position_symbol && MA_MAGIC==PositionGetInteger(POSITION_MAGIC))
           {
            res=true;
            break;
           }
        }
     }
//--- 1.2 净持账户
   else
     {
      //--- 2. 净持账户要么根据名称选择, 要么根据MAGIC选择
      if(!PositionSelect(_Symbol))
         return(false);
      else
         return(PositionGetInteger(POSITION_MAGIC)==MA_MAGIC); //---check Magic number
     }
//--- result for Hedging mode
   return(res);
  }
```