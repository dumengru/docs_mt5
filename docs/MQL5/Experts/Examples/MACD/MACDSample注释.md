# MACD Sample注释

source: `{{ page.path }}`

## 面向对象EA流程

1.设置MAGIC

```cpp
#define MACD_MAGIC 1234502
```

2.包含头文件

```cpp
#include <Trade\Trade.mqh>
#include <Trade\SymbolInfo.mqh>
#include <Trade\PositionInfo.mqh>
#include <Trade\AccountInfo.mqh>
```

3.设置输入参数

```cpp
input double InpLots          =0.1; // Lots
input int    InpTakeProfit    =50;  // Take Profit (in pips)
input int    InpTrailingStop  =30;  // Trailing Stop Level (in pips)
input int    InpMACDOpenLevel =3;   // MACD open level (in pips)
input int    InpMACDCloseLevel=2;   // MACD close level (in pips)
input int    InpMATrendPeriod =26;  // MA trend period

//--- 每隔10秒执行一次
int ExtTimeOut=10;
```

4.创建并实例化交易类对象

```cpp
class CSampleExpert{};

CSampleExpert ExtExpert;
```

5.OnInit

```cpp
int OnInit(void)
  {
//--- 交易类对象初始化
   if(!ExtExpert.Init())
      return(INIT_FAILED);
//--- secceed
   return(INIT_SUCCEEDED);
  }
```

6.OnTick

```cpp
void OnTick(void)
  {
   //--- 创建静态时间, 设置交易时间间隔
   static datetime limit_time=0;
   if(TimeCurrent()>=limit_time)
     {
      //--- 检查数据
      if(Bars(Symbol(),Period())>2*InpMATrendPeriod)
        {
         //--- 执行主处理程序, 并设置下次交易时间
         if(ExtExpert.Processing())
            limit_time=TimeCurrent()+ExtTimeOut;
        }
     }
  }
```

## 交易类注释

```cpp
class CSampleExpert
  {
protected:
   double            m_adjusted_point;             // 调整点值
   CTrade            m_trade;                      // 交易对象
   CSymbolInfo       m_symbol;                     // 品种对象
   CPositionInfo     m_position;                   // 持仓对象
   CAccountInfo      m_account;                    // 账户对象
   //--- 指标
   int               m_handle_macd;                // MACD 指标句柄
   int               m_handle_ema;                 // EMA 指标句柄
   //--- 指标数据缓冲区
   double            m_buff_MACD_main[];           // MACD main buffer
   double            m_buff_MACD_signal[];         // MACD signal buffer
   double            m_buff_EMA[];                 // EMA buffer
   //--- 处理指标数据
   double            m_macd_current;               // 最新macd main
   double            m_macd_previous;              // 前一个macd main
   double            m_signal_current;             // 最新macd signal
   double            m_signal_previous;            // 前一个macd signal
   double            m_ema_current;                // 最新ema
   double            m_ema_previous;               // 前一个ema
   //--- 阈值
   double            m_macd_open_level;            // macd开仓阈值
   double            m_macd_close_level;           // macd平仓阈值
   double            m_traling_stop;               // 止损
   double            m_take_profit;                // 止盈

public:
                     CSampleExpert(void);
                    ~CSampleExpert(void);
   bool              Init(void);
   void              Deinit(void);
   bool              Processing(void);

protected:
   bool              InitCheckParameters(const int digits_adjust);
   bool              InitIndicators(void);
   bool              LongClosed(void);
   bool              ShortClosed(void);
   bool              LongModified(void);
   bool              ShortModified(void);
   bool              LongOpened(void);
   bool              ShortOpened(void);
  };
//--- 创建交易类对象
CSampleExpert ExtExpert;
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
CSampleExpert::CSampleExpert(void) : m_adjusted_point(0),
                                     m_handle_macd(INVALID_HANDLE),
                                     m_handle_ema(INVALID_HANDLE),
                                     m_macd_current(0),
                                     m_macd_previous(0),
                                     m_signal_current(0),
                                     m_signal_previous(0),
                                     m_ema_current(0),
                                     m_ema_previous(0),
                                     m_macd_open_level(0),
                                     m_macd_close_level(0),
                                     m_traling_stop(0),
                                     m_take_profit(0)
  {
   //--- 设置数据缓冲数组方向
   ArraySetAsSeries(m_buff_MACD_main,true);
   ArraySetAsSeries(m_buff_MACD_signal,true);
   ArraySetAsSeries(m_buff_EMA,true);
  }
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
CSampleExpert::~CSampleExpert(void)
  {
  }
//+------------------------------------------------------------------+
//| Initialization and checking for input parameters                 |
//+------------------------------------------------------------------+
bool CSampleExpert::Init(void)
  {
//--- initialize common information
   m_symbol.Name(Symbol());                  // 设置品种
   m_trade.SetExpertMagicNumber(MACD_MAGIC); // 设置交易magic
   m_trade.SetMarginMode();                  // 根据账户设定预付款计算模式
   m_trade.SetTypeFillingBySymbol(Symbol()); // 根据品种设定订单成交指令

//--- 如果品种价格精度是3/5, 则调整点值*10
   int digits_adjust=1; //--- 可看做调整单位
   if(m_symbol.Digits()==3 || m_symbol.Digits()==5)
      digits_adjust=10;
   m_adjusted_point=m_symbol.Point()*digits_adjust;
//--- 根据调整点值调整开平仓阈值和止盈/止损
   m_macd_open_level =InpMACDOpenLevel*m_adjusted_point;
   m_macd_close_level=InpMACDCloseLevel*m_adjusted_point;
   m_traling_stop    =InpTrailingStop*m_adjusted_point;
   m_take_profit     =InpTakeProfit*m_adjusted_point;
//--- 设置默认滑点
   m_trade.SetDeviationInPoints(3*digits_adjust);
//--- 检查参数
   if(!InitCheckParameters(digits_adjust))
      return(false);
//--- 初始化指标
   if(!InitIndicators())
      return(false);
//--- succeed
   return(true);
  }
//+------------------------------------------------------------------+
//| Checking for input parameters                                    |
//+------------------------------------------------------------------+
bool CSampleExpert::InitCheckParameters(const int digits_adjust)
  {
//--- 止损和止盈点数必须大于0(初始时 m_symbol.StopsLevel()==0)
   if(InpTakeProfit*digits_adjust<m_symbol.StopsLevel())
     {
      printf("Take Profit must be greater than %d",m_symbol.StopsLevel());
      return(false);
     }
   if(InpTrailingStop*digits_adjust<m_symbol.StopsLevel())
     {
      printf("Trailing Stop must be greater than %d",m_symbol.StopsLevel());
      return(false);
     }
//--- 输入的下单量必须在合理范围内(包括大小和步长值)
   if(InpLots<m_symbol.LotsMin() || InpLots>m_symbol.LotsMax())
     {
      printf("Lots amount must be in the range from %f to %f",m_symbol.LotsMin(),m_symbol.LotsMax());
      return(false);
     }
   if(MathAbs(InpLots/m_symbol.LotsStep()-MathRound(InpLots/m_symbol.LotsStep()))>1.0E-10)
     {
      printf("Lots amount is not corresponding with lot step %f",m_symbol.LotsStep());
      return(false);
     }
//--- 如果止盈小于止损则警告
   if(InpTakeProfit<=InpTrailingStop)
      printf("Warning: Trailing Stop must be less than Take Profit");
//--- succeed
   return(true);
  }
//+------------------------------------------------------------------+
//| Initialization of the indicators                                 |
//+------------------------------------------------------------------+
bool CSampleExpert::InitIndicators(void)
  {
//--- 创建MACD指标句柄
   if(m_handle_macd==INVALID_HANDLE)
      if((m_handle_macd=iMACD(NULL,0,12,26,9,PRICE_CLOSE))==INVALID_HANDLE)
        {
         printf("Error creating MACD indicator");
         return(false);
        }
//--- 创建EMA指标句柄
   if(m_handle_ema==INVALID_HANDLE)
      if((m_handle_ema=iMA(NULL,0,InpMATrendPeriod,0,MODE_EMA,PRICE_CLOSE))==INVALID_HANDLE)
        {
         printf("Error creating EMA indicator");
         return(false);
        }
//--- succeed
   return(true);
  }
//+------------------------------------------------------------------+
//| Check for long position closing                                  |
//+------------------------------------------------------------------+
bool CSampleExpert::LongClosed(void)
  {
   bool res=false;
//--- 多头平仓条件
   if(m_macd_current>0)
      if(m_macd_current<m_signal_current && m_macd_previous>m_signal_previous)
         if(m_macd_current>m_macd_close_level)
           {
            //--- 平仓
            if(m_trade.PositionClose(Symbol()))
               printf("Long position by %s to be closed",Symbol());
            else
               printf("Error closing position by %s : '%s'",Symbol(),m_trade.ResultComment());
            //--- processed and cannot be modified
            res=true;
           }
//--- result
   return(res);
  }
//+------------------------------------------------------------------+
//| Check for short position closing                                 |
//+------------------------------------------------------------------+
bool CSampleExpert::ShortClosed(void)
  {
   bool res=false;
//--- 空头平仓条件
   if(m_macd_current<0)
      if(m_macd_current>m_signal_current && m_macd_previous<m_signal_previous)
         if(MathAbs(m_macd_current)>m_macd_close_level)
           {
            //--- 平仓
            if(m_trade.PositionClose(Symbol()))
               printf("Short position by %s to be closed",Symbol());
            else
               printf("Error closing position by %s : '%s'",Symbol(),m_trade.ResultComment());
            //--- processed and cannot be modified
            res=true;
           }
//--- result
   return(res);
  }
//+------------------------------------------------------------------+
//| Check for long position modifying                                |
//+------------------------------------------------------------------+
bool CSampleExpert::LongModified(void)
  {
   bool res=false;
//--- 多头跟踪止损
   if(InpTrailingStop>0)
     {
      //--- 1. 判断当前价格大于跟踪止点差
      if(m_symbol.Bid()-m_position.PriceOpen()>m_adjusted_point*InpTrailingStop)
        {
         //--- 2. 计算止损/止盈
         double sl=NormalizeDouble(m_symbol.Bid()-m_traling_stop,m_symbol.Digits());
         double tp=m_position.TakeProfit();
         if(m_position.StopLoss()<sl || m_position.StopLoss()==0.0)
           {
            //--- 3. 修改止损/止盈
            if(m_trade.PositionModify(Symbol(),sl,tp))
               printf("Long position by %s to be modified",Symbol());
            else
              {
               printf("Error modifying position by %s : '%s'",Symbol(),m_trade.ResultComment());
               printf("Modify parameters : SL=%f,TP=%f",sl,tp);
              }
            //--- modified and must exit from expert
            res=true;
           }
        }
     }
//--- result
   return(res);
  }
//+------------------------------------------------------------------+
//| Check for short position modifying                               |
//+------------------------------------------------------------------+
bool CSampleExpert::ShortModified(void)
  {
   bool   res=false;
//--- 空头跟踪止损
   if(InpTrailingStop>0)
     {
      //--- 1. 判断当前价格大于跟踪止点差
      if((m_position.PriceOpen()-m_symbol.Ask())>(m_adjusted_point*InpTrailingStop))
        {
         //--- 2. 计算止损/止盈
         double sl=NormalizeDouble(m_symbol.Ask()+m_traling_stop,m_symbol.Digits());
         double tp=m_position.TakeProfit();
         if(m_position.StopLoss()>sl || m_position.StopLoss()==0.0)
           {
            //--- 3. 修改止损/止盈
            if(m_trade.PositionModify(Symbol(),sl,tp))
               printf("Short position by %s to be modified",Symbol());
            else
              {
               printf("Error modifying position by %s : '%s'",Symbol(),m_trade.ResultComment());
               printf("Modify parameters : SL=%f,TP=%f",sl,tp);
              }
            //--- modified and must exit from expert
            res=true;
           }
        }
     }
//--- result
   return(res);
  }
//+------------------------------------------------------------------+
//| Check for long position opening                                  |
//+------------------------------------------------------------------+
bool CSampleExpert::LongOpened(void)
  {
   bool res=false;
//--- 检查多头开仓条件
   if(m_macd_current<0)
      if(m_macd_current>m_signal_current && m_macd_previous<m_signal_previous)
         if(MathAbs(m_macd_current)>(m_macd_open_level) && m_ema_current>m_ema_previous)
           {
            //--- 1. 计算开仓价格和止盈价格
            double price=m_symbol.Ask();
            double tp   =m_symbol.Bid()+m_take_profit;
            //--- 2. 检查账户余额
            if(m_account.FreeMarginCheck(Symbol(),ORDER_TYPE_BUY,InpLots,price)<0.0)
               printf("We have no money. Free Margin = %f",m_account.FreeMargin());
            else
              {
               //--- 3. 开仓
               if(m_trade.PositionOpen(Symbol(),ORDER_TYPE_BUY,InpLots,price,0.0,tp))
                  printf("Position by %s to be opened",Symbol());
               else
                 {
                  printf("Error opening BUY position by %s : '%s'",Symbol(),m_trade.ResultComment());
                  printf("Open parameters : price=%f,TP=%f",price,tp);
                 }
              }
            //--- in any case we must exit from expert
            res=true;
           }
//--- result
   return(res);
  }
//+------------------------------------------------------------------+
//| Check for short position opening                                 |
//+------------------------------------------------------------------+
bool CSampleExpert::ShortOpened(void)
  {
   bool res=false;
//--- 检查空头开仓条件
   if(m_macd_current>0)
      if(m_macd_current<m_signal_current && m_macd_previous>m_signal_previous)
         if(m_macd_current>(m_macd_open_level) && m_ema_current<m_ema_previous)
           {
            //--- 1. 计算开仓价和止盈价
            double price=m_symbol.Bid();
            double tp   =m_symbol.Ask()-m_take_profit;
            //--- 2. 检查账户余额
            if(m_account.FreeMarginCheck(Symbol(),ORDER_TYPE_SELL,InpLots,price)<0.0)
               printf("We have no money. Free Margin = %f",m_account.FreeMargin());
            else
              {
               //--- 3. 开仓
               if(m_trade.PositionOpen(Symbol(),ORDER_TYPE_SELL,InpLots,price,0.0,tp))
                  printf("Position by %s to be opened",Symbol());
               else
                 {
                  printf("Error opening SELL position by %s : '%s'",Symbol(),m_trade.ResultComment());
                  printf("Open parameters : price=%f,TP=%f",price,tp);
                 }
              }
            //--- in any case we must exit from expert
            res=true;
           }
//--- result
   return(res);
  }
//+------------------------------------------------------------------+
//| main function returns true if any position processed             |
//+------------------------------------------------------------------+
bool CSampleExpert::Processing(void)
  {
//--- 1. 刷新品种报价数据
   if(!m_symbol.RefreshRates())
      return(false);
//--- 2. 更新指标数据
   if(BarsCalculated(m_handle_macd)<2 || BarsCalculated(m_handle_ema)<2)
      return(false);
   if(CopyBuffer(m_handle_macd,0,0,2,m_buff_MACD_main)  !=2 ||
      CopyBuffer(m_handle_macd,1,0,2,m_buff_MACD_signal)!=2 ||
      CopyBuffer(m_handle_ema,0,0,2,m_buff_EMA)         !=2)
      return(false);

   m_macd_current   =m_buff_MACD_main[0];
   m_macd_previous  =m_buff_MACD_main[1];
   m_signal_current =m_buff_MACD_signal[0];
   m_signal_previous=m_buff_MACD_signal[1];
   m_ema_current    =m_buff_EMA[0];
   m_ema_previous   =m_buff_EMA[1];
//--- 3. 先判断平仓/修改持仓条件
   if(m_position.Select(Symbol()))
     {
      if(m_position.PositionType()==POSITION_TYPE_BUY)
        {
         //--- 3.1 先判断多单是否平仓, 再判断是否修改持仓
         if(LongClosed())
            return(true);
         if(LongModified())
            return(true);
        }
      else
        {
         //--- 3.2 先判断多单是否平仓, 再判断是否修改持仓
         if(ShortClosed())
            return(true);
         if(ShortModified())
            return(true);
        }
     }
//--- 4. 再判断开仓条件
   else
     {
      //--- check for long position (BUY) possibility
      if(LongOpened())
         return(true);
      //--- check for short position (SELL) possibility
      if(ShortOpened())
         return(true);
     }
//--- exit without position processing
   return(false);
  }
```