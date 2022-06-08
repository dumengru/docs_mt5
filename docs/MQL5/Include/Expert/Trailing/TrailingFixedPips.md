# TrailingFixedPips.mqh

source: `{{ page.path }}`

实现固定点差尾随停止算法

只实现检查并调整止盈止损点

```cpp
#include <Expert\ExpertTrailing.mqh>
// wizard description start
//+----------------------------------------------------------------------+
//| Description of the class                                             |
//| Title=Trailing Stop based on fixed Stop Level                        |
//| Type=Trailing                                                        |
//| Name=FixedPips                                                       |
//| Class=CTrailingFixedPips                                             |
//| Page=                                                                |
//| Parameter=StopLevel,int,30,Stop Loss trailing level (in points)      |
//| Parameter=ProfitLevel,int,50,Take Profit trailing level (in points)  |
//+----------------------------------------------------------------------+
// wizard description end
//+------------------------------------------------------------------+
//| Class CTrailingFixedPips.                                        |
//| Purpose: Class of trailing stops with fixed stop level in pips.  |
//|              Derives from class CExpertTrailing.                 |
//+------------------------------------------------------------------+
class CTrailingFixedPips : public CExpertTrailing
  {
protected:
   //--- 输入参数: 止盈/止损点数
   int               m_stop_level;
   int               m_profit_level;

public:
                     CTrailingFixedPips(void);
                    ~CTrailingFixedPips(void);
   //--- 参数设置
   void              StopLevel(int stop_level)     { m_stop_level=stop_level;     }
   void              ProfitLevel(int profit_level) { m_profit_level=profit_level; }
   virtual bool      ValidationSettings(void);
   //--- 检查尾随方法
   virtual bool      CheckTrailingStopLong(CPositionInfo *position,double &sl,double &tp);
   virtual bool      CheckTrailingStopShort(CPositionInfo *position,double &sl,double &tp);
  };
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
void CTrailingFixedPips::CTrailingFixedPips(void) : m_stop_level(30),
                                                    m_profit_level(50)
  {
  }
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
CTrailingFixedPips::~CTrailingFixedPips(void)
  {
  }
//+------------------------------------------------------------------+
//| Validation settings protected data.                              |
//+------------------------------------------------------------------+
bool CTrailingFixedPips::ValidationSettings(void)
  {
   if(!CExpertTrailing::ValidationSettings())
      return(false);
//--- initial data checks
   if(m_profit_level!=0 && m_profit_level*(m_adjusted_point/m_symbol.Point())<m_symbol.StopsLevel())
     {
      printf(__FUNCTION__+": trailing Profit Level must be 0 or greater than %d",m_symbol.StopsLevel());
      return(false);
     }
   if(m_stop_level!=0 && m_stop_level*(m_adjusted_point/m_symbol.Point())<m_symbol.StopsLevel())
     {
      printf(__FUNCTION__+": trailing Stop Level must be 0 or greater than %d",m_symbol.StopsLevel());
      return(false);
     }
//--- ok
   return(true);
  }
//+------------------------------------------------------------------+
//| Checking trailing stop and/or profit for long position.          |
//+------------------------------------------------------------------+
bool CTrailingFixedPips::CheckTrailingStopLong(CPositionInfo *position,double &sl,double &tp)
  {
//--- check
   if(position==NULL)
      return(false);
   if(m_stop_level==0)
      return(false);
/*
delta: 调整阈值=止损点数*调整比例
base: 止损价或开盘价
price: 最新卖价

逻辑:
当最新价超出波动阈值, 则更新止损止盈价格
*/
   double delta;
   double pos_sl=position.StopLoss();   // 获取持仓止损价
   double base  =(pos_sl==0.0) ? position.PriceOpen() : pos_sl;
   double price =m_symbol.Bid();
//---
   sl=EMPTY_VALUE;
   tp=EMPTY_VALUE;
   delta=m_stop_level*m_adjusted_point;
   if(price-base>delta)
     {
      sl=price-delta;
      if(m_profit_level!=0)
         tp=price+m_profit_level*m_adjusted_point;
     }
//---
   return(sl!=EMPTY_VALUE);
  }
//+------------------------------------------------------------------+
//| Checking trailing stop and/or profit for short position.         |
//+------------------------------------------------------------------+
bool CTrailingFixedPips::CheckTrailingStopShort(CPositionInfo *position,double &sl,double &tp)
  {
//--- check
   if(position==NULL)
      return(false);
   if(m_stop_level==0)
      return(false);
//---
   double delta;
   double pos_sl=position.StopLoss();
   double base  =(pos_sl==0.0) ? position.PriceOpen() : pos_sl;
   double price =m_symbol.Ask();
//---
   sl=EMPTY_VALUE;
   tp=EMPTY_VALUE;
   delta=m_stop_level*m_adjusted_point;
   if(base-price>delta)
     {
      sl=price+delta;
      if(m_profit_level!=0)
         tp=price-m_profit_level*m_adjusted_point;
     }
//---
   return(sl!=EMPTY_VALUE);
  }
```