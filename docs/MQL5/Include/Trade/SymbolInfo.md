# SymbolInfo.mqh

source: `{{ page.path }}`

定义品种基本信息

```cpp
#include <Object.mqh>
//+------------------------------------------------------------------+
//| Class CSymbolInfo.                                               |
//| Appointment: Class for access to symbol info.                    |
//|              Derives from class CObject.                         |
//+------------------------------------------------------------------+
class CSymbolInfo : public CObject
  {
protected:
   string            m_name;                       // 名称
   MqlTick           m_tick;                       // tick结构体
   double            m_point;                      // 点值
   double            m_tick_value;                 // symbol tick value
   double            m_tick_value_profit;          // symbol tick value profit
   double            m_tick_value_loss;            // symbol tick value loss
   double            m_tick_size;                  // 最小价格改变
   double            m_contract_size;              // 合约乘数
   double            m_lots_min;                   // 最小下单量
   double            m_lots_max;                   // 最大下单量
   double            m_lots_step;                  // 下单量步长值
   double            m_lots_limit;                 // 下单量限制
   double            m_swap_long;                  // 买入库存费
   double            m_swap_short;                 // 卖出库存费
   int               m_digits;                     // 精度
   int               m_order_mode;                 // 订单模式
   ENUM_SYMBOL_TRADE_EXECUTION m_trade_execution;  // 订单执行方式(立即/市价执行)
   ENUM_SYMBOL_CALC_MODE m_trade_calcmode;         // 保证金计算模式(外汇/期货模式)
   ENUM_SYMBOL_TRADE_MODE m_trade_mode;            // 交易模式(仅买入/卖出等)
   ENUM_SYMBOL_SWAP_MODE m_swap_mode;              // 利息模式(禁用利息/以点数收取利息)
   ENUM_DAY_OF_WEEK  m_swap3;                      // 周度标识符
   double            m_margin_initial;             // 初始保证金
   double            m_margin_maintenance;         // 维持保证金
   bool              m_margin_hedged_use_leg;      // 单边保证金
   double            m_margin_hedged;              // 锁仓预付款
   int               m_trade_time_flags;           // 订单到期模式
   int               m_trade_fill_flags;           // 填充模式(立即执行/取消)

public:
                     CSymbolInfo(void);
                    ~CSymbolInfo(void);
   //--- 获取数据: 设置/获取名称, 更新基本信息, 更新价格数据
   string            Name(void) const { return(m_name); }
   bool              Name(const string name);
   bool              Refresh(void);
   bool              RefreshRates(void);
   //--- 选择品种: 添加, 添加或移除, 数据是否同步
   bool              Select(void) const;
   bool              Select(const bool select);
   bool              IsSynchronized(void) const;
   //--- 成交量: 最后一笔, 日内最大/最小
   ulong             Volume(void)     const { return(m_tick.volume); }
   ulong             VolumeHigh(void) const;
   ulong             VolumeLow(void)  const;
   //--- 综合: 最后一笔时间, 点差, 是否浮动点差, 市场深度
   datetime          Time(void)           const { return(m_tick.time); }
   int               Spread(void)         const;
   bool              SpreadFloat(void)    const;
   int               TicksBookDepth(void) const;
   //--- 级别: 订单距现价点数
   int               StopsLevel(void)  const;
   int               FreezeLevel(void) const;
   //--- 卖价: 最新卖价, 日内最高/最低卖价
   double            Bid(void)      const { return(m_tick.bid); }
   double            BidHigh(void)  const;
   double            BidLow(void)   const;
   //--- 买价: 最新买价, 日内最高/最低买价
   double            Ask(void)      const { return(m_tick.ask); }
   double            AskHigh(void)  const;
   double            AskLow(void)   const;
   //--- 成交价: 最新价, 日内最高/最低成交价 
   double            Last(void)     const { return(m_tick.last); }
   double            LastHigh(void) const;
   double            LastLow(void)  const;
   //--- 订单模式
   int               OrderMode(void) const { return(m_order_mode); }
   //--- 交易模式
   ENUM_SYMBOL_CALC_MODE TradeCalcMode(void)        const { return(m_trade_calcmode); }
   string            TradeCalcModeDescription(void) const;
   ENUM_SYMBOL_TRADE_MODE TradeMode(void)           const { return(m_trade_mode);     }
   string            TradeModeDescription(void)     const;
   //--- execution terms of trade
   ENUM_SYMBOL_TRADE_EXECUTION TradeExecution(void)  const { return(m_trade_execution); }
   string            TradeExecutionDescription(void) const;
   //--- 掉期利率
   ENUM_SYMBOL_SWAP_MODE SwapMode(void)                 const { return(m_swap_mode); }
   string            SwapModeDescription(void)          const;
   ENUM_DAY_OF_WEEK  SwapRollover3days(void)            const { return(m_swap3);     }
   string            SwapRollover3daysDescription(void) const;
   //--- dates for futures
   datetime          StartTime(void)      const;
   datetime          ExpirationTime(void) const;
   //--- 保证金和标志
   double            MarginInitial(void)      const { return(m_margin_initial);        }
   double            MarginMaintenance(void)  const { return(m_margin_maintenance);    }
   bool              MarginHedgedUseLeg(void) const { return(m_margin_hedged_use_leg); }
   double            MarginHedged(void)       const { return(m_margin_hedged);         }
   //--- left for backward compatibility
   double            MarginLong(void)      const { return(0.0); }
   double            MarginShort(void)     const { return(0.0); }
   double            MarginLimit(void)     const { return(0.0); }
   double            MarginStop(void)      const { return(0.0); }
   double            MarginStopLimit(void) const { return(0.0); }
   //--- trade flags parameters
   int               TradeTimeFlags(void)  const { return(m_trade_time_flags);  }
   int               TradeFillFlags(void)  const { return(m_trade_fill_flags);  }
   //--- tick参数
   int               Digits(void)          const { return(m_digits);            }
   double            Point(void)           const { return(m_point);             }
   double            TickValue(void)       const { return(m_tick_value);        }
   double            TickValueProfit(void) const { return(m_tick_value_profit); }
   double            TickValueLoss(void)   const { return(m_tick_value_loss);   }
   double            TickSize(void)        const { return(m_tick_size);         }
   //--- lots 参数
   double            ContractSize(void) const { return(m_contract_size); }
   double            LotsMin(void)      const { return(m_lots_min);      }
   double            LotsMax(void)      const { return(m_lots_max);      }
   double            LotsStep(void)     const { return(m_lots_step);     }
   double            LotsLimit(void)    const { return(m_lots_limit);    }
   //--- 掉期利率大小
   double            SwapLong(void)  const { return(m_swap_long);  }
   double            SwapShort(void) const { return(m_swap_short); }
   //--- 文本属性
   string            CurrencyBase(void)   const;
   string            CurrencyProfit(void) const;
   string            CurrencyMargin(void) const;
   string            Bank(void)           const;
   string            Description(void)    const;
   string            Path(void)           const;
   //--- 品种属性
   long              SessionDeals(void)            const;
   long              SessionBuyOrders(void)        const;
   long              SessionSellOrders(void)       const;
   double            SessionTurnover(void)         const;
   double            SessionInterest(void)         const;
   double            SessionBuyOrdersVolume(void)  const;
   double            SessionSellOrdersVolume(void) const;
   double            SessionOpen(void)             const;
   double            SessionClose(void)            const;
   double            SessionAW(void)               const;
   double            SessionPriceSettlement(void)  const;
   double            SessionPriceLimitMin(void)    const;
   double            SessionPriceLimitMax(void)    const;
   //--- MQL5API
   bool              InfoInteger(const ENUM_SYMBOL_INFO_INTEGER prop_id,long& var) const;
   bool              InfoDouble(const ENUM_SYMBOL_INFO_DOUBLE prop_id,double& var) const;
   bool              InfoString(const ENUM_SYMBOL_INFO_STRING prop_id,string& var) const;
   bool              InfoMarginRate(const ENUM_ORDER_TYPE order_type,double& initial_margin_rate,double& maintenance_margin_rate) const;
   //--- 服务函数: 规范后价格, 添加品种到观察窗
   double            NormalizePrice(const double price) const;
   bool              CheckMarketWatch(void);
  };
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
CSymbolInfo::CSymbolInfo(void) : m_name(NULL),
                                 m_point(0.0),
                                 m_tick_value(0.0),
                                 m_tick_value_profit(0.0),
                                 m_tick_value_loss(0.0),
                                 m_tick_size(0.0),
                                 m_contract_size(0.0),
                                 m_lots_min(0.0),
                                 m_lots_max(0.0),
                                 m_lots_step(0.0),
                                 m_swap_long(0.0),
                                 m_swap_short(0.0),
                                 m_digits(0),
                                 m_order_mode(0),
                                 m_trade_execution(0),
                                 m_trade_calcmode(0),
                                 m_trade_mode(0),
                                 m_swap_mode(0),
                                 m_swap3(0),
                                 m_margin_initial(0.0),
                                 m_margin_maintenance(0.0),
                                 m_margin_hedged_use_leg(false),
                                 m_margin_hedged(0.0),
                                 m_trade_time_flags(0),
                                 m_trade_fill_flags(0)
  {
  }
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
CSymbolInfo::~CSymbolInfo(void)
  {
  }
//+------------------------------------------------------------------+
//| Set name                                                         |
//+------------------------------------------------------------------+
bool CSymbolInfo::Name(const string name)
  {
   string symbol_name=StringLen(name)>0 ? name : _Symbol;
//--- check previous set name
   if(m_name!=symbol_name)
     {
      m_name=symbol_name;
      //---
      if(!CheckMarketWatch())
         return(false);
      //---
      if(!Refresh())
        {
         m_name="";
         Print(__FUNCTION__+": invalid data of symbol '"+symbol_name+"'");
         return(false);
        }
     }
//--- succeed
   return(true);
  }
//+------------------------------------------------------------------+
//| Refresh cached data                                              |
//+------------------------------------------------------------------+
bool CSymbolInfo::Refresh(void)
  {
   long tmp_long=0;
//---
   if(!SymbolInfoDouble(m_name,SYMBOL_POINT,m_point))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_TRADE_TICK_VALUE,m_tick_value))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_TRADE_TICK_VALUE_PROFIT,m_tick_value_profit))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_TRADE_TICK_VALUE_LOSS,m_tick_value_loss))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_TRADE_TICK_SIZE,m_tick_size))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_TRADE_CONTRACT_SIZE,m_contract_size))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_VOLUME_MIN,m_lots_min))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_VOLUME_MAX,m_lots_max))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_VOLUME_STEP,m_lots_step))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_VOLUME_LIMIT,m_lots_limit))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_SWAP_LONG,m_swap_long))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_SWAP_SHORT,m_swap_short))
      return(false);
   if(!SymbolInfoInteger(m_name,SYMBOL_DIGITS,tmp_long))
      return(false);
   m_digits=(int)tmp_long;
   if(!SymbolInfoInteger(m_name,SYMBOL_ORDER_MODE,tmp_long))
      return(false);
   m_order_mode=(int)tmp_long;
   if(!SymbolInfoInteger(m_name,SYMBOL_TRADE_EXEMODE,tmp_long))
      return(false);
   m_trade_execution=(ENUM_SYMBOL_TRADE_EXECUTION)tmp_long;
   if(!SymbolInfoInteger(m_name,SYMBOL_TRADE_CALC_MODE,tmp_long))
      return(false);
   m_trade_calcmode=(ENUM_SYMBOL_CALC_MODE)tmp_long;
   if(!SymbolInfoInteger(m_name,SYMBOL_TRADE_MODE,tmp_long))
      return(false);
   m_trade_mode=(ENUM_SYMBOL_TRADE_MODE)tmp_long;
   if(!SymbolInfoInteger(m_name,SYMBOL_SWAP_MODE,tmp_long))
      return(false);
   m_swap_mode=(ENUM_SYMBOL_SWAP_MODE)tmp_long;
   if(!SymbolInfoInteger(m_name,SYMBOL_SWAP_ROLLOVER3DAYS,tmp_long))
      return(false);
   m_swap3=(ENUM_DAY_OF_WEEK)tmp_long;
   if(!SymbolInfoDouble(m_name,SYMBOL_MARGIN_INITIAL,m_margin_initial))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_MARGIN_MAINTENANCE,m_margin_maintenance))
      return(false);
   if(!SymbolInfoDouble(m_name,SYMBOL_MARGIN_HEDGED,m_margin_hedged))
      return(false);
   if(!SymbolInfoInteger(m_name,SYMBOL_MARGIN_HEDGED_USE_LEG,tmp_long))
      return(false);
   m_margin_hedged_use_leg=(bool)tmp_long;
   if(!SymbolInfoInteger(m_name,SYMBOL_EXPIRATION_MODE,tmp_long))
      return(false);
   m_trade_time_flags=(int)tmp_long;
   if(!SymbolInfoInteger(m_name,SYMBOL_FILLING_MODE,tmp_long))
      return(false);
   m_trade_fill_flags=(int)tmp_long;
//--- succeed
   return(true);
  }
//+------------------------------------------------------------------+
//| Refresh cached data                                              |
//+------------------------------------------------------------------+
bool CSymbolInfo::RefreshRates(void)
  {
   return(SymbolInfoTick(m_name,m_tick));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SELECT"                           |
//+------------------------------------------------------------------+
bool CSymbolInfo::Select(void) const
  {
   return((bool)SymbolInfoInteger(m_name,SYMBOL_SELECT));
  }
//+------------------------------------------------------------------+
//| Set the property value "SYMBOL_SELECT"                           |
//+------------------------------------------------------------------+
bool CSymbolInfo::Select(const bool select)
  {
   return(SymbolSelect(m_name,select));
  }
//+------------------------------------------------------------------+
//| Check synchronize symbol                                         |
//+------------------------------------------------------------------+
bool CSymbolInfo::IsSynchronized(void) const
  {
   return(SymbolIsSynchronized(m_name));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_VOLUMEHIGH"                       |
//+------------------------------------------------------------------+
ulong CSymbolInfo::VolumeHigh(void) const
  {
   return(SymbolInfoInteger(m_name,SYMBOL_VOLUMEHIGH));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_VOLUMELOW"                        |
//+------------------------------------------------------------------+
ulong CSymbolInfo::VolumeLow(void) const
  {
   return(SymbolInfoInteger(m_name,SYMBOL_VOLUMELOW));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SPREAD"                           |
//+------------------------------------------------------------------+
int CSymbolInfo::Spread(void) const
  {
   return((int)SymbolInfoInteger(m_name,SYMBOL_SPREAD));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SPREAD_FLOAT"                     |
//+------------------------------------------------------------------+
bool CSymbolInfo::SpreadFloat(void) const
  {
   return((bool)SymbolInfoInteger(m_name,SYMBOL_SPREAD_FLOAT));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_TICKS_BOOKDEPTH"                  |
//+------------------------------------------------------------------+
int CSymbolInfo::TicksBookDepth(void) const
  {
   return((int)SymbolInfoInteger(m_name,SYMBOL_TICKS_BOOKDEPTH));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_TRADE_STOPS_LEVEL"                |
//+------------------------------------------------------------------+
int CSymbolInfo::StopsLevel(void) const
  {
   return((int)SymbolInfoInteger(m_name,SYMBOL_TRADE_STOPS_LEVEL));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_TRADE_FREEZE_LEVEL"               |
//+------------------------------------------------------------------+
int CSymbolInfo::FreezeLevel(void) const
  {
   return((int)SymbolInfoInteger(m_name,SYMBOL_TRADE_FREEZE_LEVEL));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_BIDHIGH"                          |
//+------------------------------------------------------------------+
double CSymbolInfo::BidHigh(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_BIDHIGH));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_BIDLOW"                           |
//+------------------------------------------------------------------+
double CSymbolInfo::BidLow(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_BIDLOW));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_ASKHIGH"                          |
//+------------------------------------------------------------------+
double CSymbolInfo::AskHigh(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_ASKHIGH));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_ASKLOW"                           |
//+------------------------------------------------------------------+
double CSymbolInfo::AskLow(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_ASKLOW));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_LASTHIGH"                         |
//+------------------------------------------------------------------+
double CSymbolInfo::LastHigh(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_LASTHIGH));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_LASTLOW"                          |
//+------------------------------------------------------------------+
double CSymbolInfo::LastLow(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_LASTLOW));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_TRADE_CALC_MODE" as string        |
//+------------------------------------------------------------------+
string CSymbolInfo::TradeCalcModeDescription(void) const
  {
   string str;
//---
   switch(m_trade_calcmode)
     {
      case SYMBOL_CALC_MODE_FOREX:
         str="Calculation of profit and margin for Forex";
         break;
      case SYMBOL_CALC_MODE_CFD:
         str="Calculation of collateral and earnings for CFD";
         break;
      case SYMBOL_CALC_MODE_FUTURES:
         str="Calculation of collateral and profits for futures";
         break;
      case SYMBOL_CALC_MODE_CFDINDEX:
         str="Calculation of collateral and earnings for CFD on indices";
         break;
      case SYMBOL_CALC_MODE_CFDLEVERAGE:
         str="Calculation of collateral and earnings for the CFD when trading with leverage";
         break;
      case SYMBOL_CALC_MODE_EXCH_STOCKS:
         str="Calculation for exchange stocks";
         break;
      case SYMBOL_CALC_MODE_EXCH_FUTURES:
         str="Calculation for exchange futures";
         break;
      case SYMBOL_CALC_MODE_EXCH_FUTURES_FORTS:
         str="Calculation for FORTS futures";
         break;
      default:
         str="Unknown calculation mode";
     }
//--- result
   return(str);
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_TRADE_MODE" as string             |
//+------------------------------------------------------------------+
string CSymbolInfo::TradeModeDescription(void) const
  {
   string str;
//---
   switch(m_trade_mode)
     {
      case SYMBOL_TRADE_MODE_DISABLED:
         str="Disabled";
         break;
      case SYMBOL_TRADE_MODE_LONGONLY:
         str="Long only";
         break;
      case SYMBOL_TRADE_MODE_SHORTONLY:
         str="Short only";
         break;
      case SYMBOL_TRADE_MODE_CLOSEONLY:
         str="Close only";
         break;
      case SYMBOL_TRADE_MODE_FULL:
         str="Full access";
         break;
      default:
         str="Unknown trade mode";
     }
//--- result
   return(str);
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_TRADE_EXEMODE" as string          |
//+------------------------------------------------------------------+
string CSymbolInfo::TradeExecutionDescription(void) const
  {
   string str;
//---
   switch(m_trade_execution)
     {
      case SYMBOL_TRADE_EXECUTION_REQUEST:
         str="Trading on request";
         break;
      case SYMBOL_TRADE_EXECUTION_INSTANT:
         str="Trading on live streaming prices";
         break;
      case SYMBOL_TRADE_EXECUTION_MARKET:
         str="Execution of orders on the market";
         break;
      case SYMBOL_TRADE_EXECUTION_EXCHANGE:
         str="Exchange execution";
         break;
      default:
         str="Unknown trade execution";
     }
//--- result
   return(str);
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SWAP_MODE" as string              |
//+------------------------------------------------------------------+
string CSymbolInfo::SwapModeDescription(void) const
  {
   string str;
//---
   switch(m_swap_mode)
     {
      case SYMBOL_SWAP_MODE_DISABLED:
         str="No swaps";
         break;
      case SYMBOL_SWAP_MODE_POINTS:
         str="Swaps are calculated in points";
         break;
      case SYMBOL_SWAP_MODE_CURRENCY_SYMBOL:
         str="Swaps are calculated in base currency";
         break;
      case SYMBOL_SWAP_MODE_CURRENCY_MARGIN:
         str="Swaps are calculated in margin currency";
         break;
      case SYMBOL_SWAP_MODE_CURRENCY_DEPOSIT:
         str="Swaps are calculated in deposit currency";
         break;
      case SYMBOL_SWAP_MODE_INTEREST_CURRENT:
         str="Swaps are calculated as annual interest using the current price";
         break;
      case SYMBOL_SWAP_MODE_INTEREST_OPEN:
         str="Swaps are calculated as annual interest using the open price";
         break;
      case SYMBOL_SWAP_MODE_REOPEN_CURRENT:
         str="Swaps are charged by reopening positions at the close price";
         break;
      case SYMBOL_SWAP_MODE_REOPEN_BID:
         str="Swaps are charged by reopening positions at the Bid price";
         break;
      default:
         str="Unknown swap mode";
     }
//--- result
   return(str);
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SWAP_ROLLOVER3DAYS" as string     |
//+------------------------------------------------------------------+
string CSymbolInfo::SwapRollover3daysDescription(void) const
  {
   string str;
//---
   switch(m_swap3)
     {
      case SUNDAY:
         str="Sunday";
         break;
      case MONDAY:
         str="Monday";
         break;
      case TUESDAY:
         str="Tuesday";
         break;
      case WEDNESDAY:
         str="Wednesday";
         break;
      case THURSDAY:
         str="Thursday";
         break;
      case FRIDAY:
         str="Friday";
         break;
      case SATURDAY:
         str="Saturday";
         break;
      default:
         str="Unknown";
     }
//--- result
   return(str);
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_START_TIME"                       |
//+------------------------------------------------------------------+
datetime CSymbolInfo::StartTime(void) const
  {
   return((datetime)SymbolInfoInteger(m_name,SYMBOL_START_TIME));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_EXPIRATION_TIME"                  |
//+------------------------------------------------------------------+
datetime CSymbolInfo::ExpirationTime(void) const
  {
   return((datetime)SymbolInfoInteger(m_name,SYMBOL_EXPIRATION_TIME));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_CURRENCY_BASE"                    |
//+------------------------------------------------------------------+
string CSymbolInfo::CurrencyBase(void) const
  {
   return(SymbolInfoString(m_name,SYMBOL_CURRENCY_BASE));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_CURRENCY_PROFIT"                  |
//+------------------------------------------------------------------+
string CSymbolInfo::CurrencyProfit(void) const
  {
   return(SymbolInfoString(m_name,SYMBOL_CURRENCY_PROFIT));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_CURRENCY_MARGIN"                  |
//+------------------------------------------------------------------+
string CSymbolInfo::CurrencyMargin(void) const
  {
   return(SymbolInfoString(m_name,SYMBOL_CURRENCY_MARGIN));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_BANK"                             |
//+------------------------------------------------------------------+
string CSymbolInfo::Bank(void) const
  {
   return(SymbolInfoString(m_name,SYMBOL_BANK));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_DESCRIPTION"                      |
//+------------------------------------------------------------------+
string CSymbolInfo::Description(void) const
  {
   return(SymbolInfoString(m_name,SYMBOL_DESCRIPTION));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_PATH"                             |
//+------------------------------------------------------------------+
string CSymbolInfo::Path(void) const
  {
   return(SymbolInfoString(m_name,SYMBOL_PATH));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_DEALS"                    |
//+------------------------------------------------------------------+
long CSymbolInfo::SessionDeals(void) const
  {
   return(SymbolInfoInteger(m_name,SYMBOL_SESSION_DEALS));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_BUY_ORDERS"               |
//+------------------------------------------------------------------+
long CSymbolInfo::SessionBuyOrders(void) const
  {
   return(SymbolInfoInteger(m_name,SYMBOL_SESSION_BUY_ORDERS));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_SELL_ORDERS"              |
//+------------------------------------------------------------------+
long CSymbolInfo::SessionSellOrders(void) const
  {
   return(SymbolInfoInteger(m_name,SYMBOL_SESSION_SELL_ORDERS));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_TURNOVER"                 |
//+------------------------------------------------------------------+
double CSymbolInfo::SessionTurnover(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_SESSION_TURNOVER));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_INTEREST"                 |
//+------------------------------------------------------------------+
double CSymbolInfo::SessionInterest(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_SESSION_INTEREST));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_BUY_ORDERS_VOLUME"        |
//+------------------------------------------------------------------+
double CSymbolInfo::SessionBuyOrdersVolume(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_SESSION_BUY_ORDERS_VOLUME));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_SELL_ORDERS_VOLUME"       |
//+------------------------------------------------------------------+
double CSymbolInfo::SessionSellOrdersVolume(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_SESSION_SELL_ORDERS_VOLUME));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_OPEN"                     |
//+------------------------------------------------------------------+
double CSymbolInfo::SessionOpen(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_SESSION_OPEN));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_CLOSE"                    |
//+------------------------------------------------------------------+
double CSymbolInfo::SessionClose(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_SESSION_CLOSE));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_AW"                       |
//+------------------------------------------------------------------+
double CSymbolInfo::SessionAW(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_SESSION_AW));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_PRICE_SETTLEMENT"         |
//+------------------------------------------------------------------+
double CSymbolInfo::SessionPriceSettlement(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_SESSION_PRICE_SETTLEMENT));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_PRICE_LIMIT_MIN"          |
//+------------------------------------------------------------------+
double CSymbolInfo::SessionPriceLimitMin(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_SESSION_PRICE_LIMIT_MIN));
  }
//+------------------------------------------------------------------+
//| Get the property value "SYMBOL_SESSION_PRICE_LIMIT_MAX"          |
//+------------------------------------------------------------------+
double CSymbolInfo::SessionPriceLimitMax(void) const
  {
   return(SymbolInfoDouble(m_name,SYMBOL_SESSION_PRICE_LIMIT_MAX));
  }
//+------------------------------------------------------------------+
//| Access functions SymbolInfoInteger(...)                          |
//+------------------------------------------------------------------+
bool CSymbolInfo::InfoInteger(const ENUM_SYMBOL_INFO_INTEGER prop_id,long &var) const
  {
   return(SymbolInfoInteger(m_name,prop_id,var));
  }
//+------------------------------------------------------------------+
//| Access functions SymbolInfoDouble(...)                           |
//+------------------------------------------------------------------+
bool CSymbolInfo::InfoDouble(const ENUM_SYMBOL_INFO_DOUBLE prop_id,double &var) const
  {
   return(SymbolInfoDouble(m_name,prop_id,var));
  }
//+------------------------------------------------------------------+
//| Access functions SymbolInfoString(...)                           |
//+------------------------------------------------------------------+
bool CSymbolInfo::InfoString(const ENUM_SYMBOL_INFO_STRING prop_id,string &var) const
  {
   return(SymbolInfoString(m_name,prop_id,var));
  }
//+------------------------------------------------------------------+
//| Access functions SymbolInfoMarginRate(...)                           |
//+------------------------------------------------------------------+
bool CSymbolInfo::InfoMarginRate(const ENUM_ORDER_TYPE order_type,double& initial_margin_rate,double& maintenance_margin_rate) const
  {
   return(SymbolInfoMarginRate(m_name,order_type,initial_margin_rate,maintenance_margin_rate));
  }
//+------------------------------------------------------------------+
//| Normalize price                                                  |
//+------------------------------------------------------------------+
double CSymbolInfo::NormalizePrice(const double price) const
  {
   if(m_tick_size!=0)
      return(NormalizeDouble(MathRound(price/m_tick_size)*m_tick_size,m_digits));
//---
   return(NormalizeDouble(price,m_digits));
  }
//+------------------------------------------------------------------+
//| Checks if symbol is selected in the MarketWatch                  |
//| and adds symbol to the MarketWatch, if necessary                 |
//+------------------------------------------------------------------+
bool CSymbolInfo::CheckMarketWatch(void)
  {
//--- check if symbol is selected in the MarketWatch
   if(!Select())
     {
      if(GetLastError()==ERR_MARKET_UNKNOWN_SYMBOL)
        {
         printf(__FUNCTION__+": Unknown symbol '%s'",m_name);
         return(false);
        }
      if(!Select(true))
        {
         printf(__FUNCTION__+": Error adding symbol %d",GetLastError());
         return(false);
        }
     }
//--- succeed
   return(true);
  }
```