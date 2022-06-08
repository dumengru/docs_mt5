# TestAC注释

source: `{{ page.path }}`

## 概述

利用Wizard生成简单的策略

- 品种: XAUUSD
- 周期: PERIOD_M1
- Signal: AC
- Trailing: FixedPips
- Money: FixLot

## 初始化

### Expert初始化

ExtExpert.Init, 进入CExpert::Init
1. 创建`m_symbol`对象, `m_symbol=new CSymbolInfo`
2. 设置`m_symbol`品种, `m_symbol.Name(symbol)`
3. 设置`m_period`, 
4. 设置`m_every_tick`,
5. 设置`m_magic`, 
6. 设置`m_margin_mode`, `SetMarginMode`
7. 设置`m_period_flags`, `TimeframeAdd`
8. 设置`m_adjusted_point`,
9. 设置`m_trade`, `InitTrade`
10. 设置`m_signal`, `InitSignal`
11. 设置`m_trailing`, `InitTrailing`
12. 设置`m_money`, `InitMoney`
13. 设置`m_beg_date`起始时间, `PrepareHistoryDate`
14. 检查订单和持仓, `HistoryPoint`
    - m_hist_ord_tot
    - m_deal_tot
    - m_ord_tot
    - m_pos_tot
15. 修改初始化状态, `m_init_phase=INIT_PHASE_TUNING`

### ExpertSignal初始化

#### 初始化参数
1. 创建CExpertSignal, `signal=new CExpertSignal`
2. 初始化, `ExtExpert.InitSignal`
3. 因为之前初始化一次, 所以首先`delete m_signal`
4. 替换并设置`m_signal`, `m_signal=signal`
5. 设置`m_threshold_open`, `signal.ThresholdOpen`
6. 设置`m_threshold_close`, `signal.ThresholdClose`
7. 设置`m_price_level`, `signal.PriceLevel`
8. 设置`m_stop_level`, `signal.StopLevel`
9. 设置`m_take_level`, `signal.TakeLevel`
10. 设置`m_expiration`, `signal.Expiration`

#### 添加过滤器

1. 创建过滤器, `CSignalAC *filter0=new CSignalAC`
2. 将过滤器添加至信号中, `signal.AddFilter(filter0)`
   - 进入`CExpertSignal::AddFilter`
   - 初始化过滤器`filter.Init`, 进入`CExpertBase::Init`
   - 添加过滤器, `m_filters.Add(filter)`
3. 设置过滤器权重`m_weight`, `filter0.Weight`

### Trailing初始化

1. 创建Trailing, `*trailing=new CTrailingFixedPips`
2. 初始化, `ExtExpert.InitTrailing`
3. 因为之前初始化一次, 所以首先`delete m_trailing`
4. 替换并设置`m_trailing`, `m_trailing=trailing`
5. 设置`m_stop_level`, `trailing.StopLevel`
6. 设置`m_profit_level`, `trailing.ProfitLevel`

### Money初始化

1. 创建Money, `*money=new CMoneyFixedLot`
2. 初始化, `ExtExpert.InitMoney`
3. 因为之前初始化一次, 所以首先`delete m_money`
4. 替换并设置`m_money`, `m_money=money`
5. 设置`m_percent`, `money.Percent`
6. 设置`m_lots`, `money.Lots`

### 参数校验 

ExtExpert.ValidationSettings

### 指标和时间序列初始化

ExtExpert.InitIndicators
进入`CExpert::InitIndicators`
1. 获取`m_indicators`指针, `*indicators_ptr=GetPointer(m_indicators);`
2. 设置`m_used_series`,
3. 初始化指标
   - `m_open`
   - `m_high`
   - `m_low`
   - `m_close`
   - `m_spread`
   - `m_time`
   - `m_tickvolume`
   - `m_realvolume`
4. 更新状态, `m_init_phase=INIT_PHASE_COMPLETE`
5. 为`m_signal`添加时间序列并初始化
6. 为`m_trailing`添加时间序列并初始化
7. 为`m_money`添加时间序列并初始化

## OnTick

默认`m_on_tick_process`为`true`

### 更新数据

`CExpert::Refresh`

1. 更新tick数据, `m_symbol.RefreshRates`
2. 更新指标数据, `m_indicators.Refresh`
3. 更新`m_prev_time`, `m_prev_time=time`

### 处理逻辑

#### 1. 信号方向

执行`m_signal.SetDirection`, 进入`CExpertSignal::Direction`
1. 添加掩码`mask`
2. 添加指标方向数值`direction`
3. 添加指标结果数值`result`
4. 添加指标数量`number`
5. 遍历过滤器`m_filters`, 执行`filter.Direction`
6. 还是进入`CExpertSignal::Direction`
7. 计算`result`并返回

#### 2. 选择持仓

`SelectPosition()`

#### 3. 遍历订单

#### 4. 检查开仓

1. 执行`CheckOpen`, 进入`CExpert::CheckOpen`
2. 检查开多`CheckOpenLong`, 进入`CExpert::CheckOpenLong`
   - 执行`m_signal.CheckOpenLong`, 进入`CExpertSignal::CheckOpenLong`
   - 判断开多条件`m_direction>=m_threshold_open`
   - 设置`m_expiration`
   - 执行`OpenLong`, 买开

#### 5. 其他逻辑

如果有持仓
1. 选中持仓, `SelectPosition`
2. 检查反手, `CheckReverse`
3. 检查平仓, `CheckClose`
4. 尾随停止, `CheckTrailingStop`

如果有订单
1. 检查订单方向
2. 检查删除多头订单
3. 检查多头尾随订单
4. 检查删除空头订单
5. 检查空头尾随订单

```cpp
#include <Expert\Expert.mqh>
//--- available signals
#include <Expert\Signal\SignalAC.mqh>
//--- available trailing
#include <Expert\Trailing\TrailingFixedPips.mqh>
//--- available money management
#include <Expert\Money\MoneyFixedLot.mqh>
//+------------------------------------------------------------------+
//| Inputs                                                           |
//+------------------------------------------------------------------+
//--- expert参数
input string Expert_Title                  ="TestAC"; // 策略名称
ulong        Expert_MagicNumber            =14430;    // MagicID
bool         Expert_EveryTick              =false;    // 是否使用Tick
//--- signal参数
input int    Signal_ThresholdOpen          =10;       // 开仓阈值 [0...100]
input int    Signal_ThresholdClose         =10;       // 平仓阈值 [0...100]
input double Signal_PriceLevel             =0.0;      // Price level to execute a deal
input double Signal_StopLevel              =50.0;     // 止损点数 (in points)
input double Signal_TakeLevel              =50.0;     // 止盈点数 (in points)
input int    Signal_Expiration             =4;        // 订单到期时间 (in bars)
input double Signal_AC_Weight              =1.0;      // AC信号权重 [0...1.0]
//--- inputs for trailing
input int    Trailing_FixedPips_StopLevel  =30;       // 尾随止损点数 (in points)
input int    Trailing_FixedPips_ProfitLevel=50;       // 尾随止盈点数 (in points)
//--- inputs for money
input double Money_FixLot_Percent          =10.0;     // 资金风险百分比
input double Money_FixLot_Lots             =0.1;      // 固定手数
//+------------------------------------------------------------------+
//| 全局expert对象                                                    |
//+------------------------------------------------------------------+
CExpert ExtExpert;
//+------------------------------------------------------------------+
//| Initialization function of the expert                            |
//+------------------------------------------------------------------+
int OnInit()
  {
//--- 1. 初始化Expert
   if(!ExtExpert.Init("XAUUSD",PERIOD_M1,Expert_EveryTick,Expert_MagicNumber))
     {
      //--- failed
      printf(__FUNCTION__+": error initializing expert");
      ExtExpert.Deinit();
      return(INIT_FAILED);
     }
//--- 2. 创建Signal
   CExpertSignal *signal=new CExpertSignal;
   if(signal==NULL)
     {
      //--- failed
      printf(__FUNCTION__+": error creating signal");
      ExtExpert.Deinit();
      return(INIT_FAILED);
     }
//--- 2.1 初始化Signal并设置参数
   ExtExpert.InitSignal(signal);
   signal.ThresholdOpen(Signal_ThresholdOpen);
   signal.ThresholdClose(Signal_ThresholdClose);
   signal.PriceLevel(Signal_PriceLevel);
   signal.StopLevel(Signal_StopLevel);
   signal.TakeLevel(Signal_TakeLevel);
   signal.Expiration(Signal_Expiration);
//--- 3. 创建CSignalAC
   CSignalAC *filter0=new CSignalAC;
   if(filter0==NULL)
     {
      //--- failed
      printf(__FUNCTION__+": error creating filter0");
      ExtExpert.Deinit();
      return(INIT_FAILED);
     }
//--- 3.1 将CSignalAC添加到Signal中
   signal.AddFilter(filter0);
//--- 3.2 设置CSignalAC权重
   filter0.Weight(Signal_AC_Weight);
//--- 4. 创建Trailing
   CTrailingFixedPips *trailing=new CTrailingFixedPips;
   if(trailing==NULL)
     {
      //--- failed
      printf(__FUNCTION__+": error creating trailing");
      ExtExpert.Deinit();
      return(INIT_FAILED);
     }
//--- 4.1 将Trailing添加到Expert
   if(!ExtExpert.InitTrailing(trailing))
     {
      //--- failed
      printf(__FUNCTION__+": error initializing trailing");
      ExtExpert.Deinit();
      return(INIT_FAILED);
     }
//--- 4.2 设置Trailing参数
   trailing.StopLevel(Trailing_FixedPips_StopLevel);
   trailing.ProfitLevel(Trailing_FixedPips_ProfitLevel);
//--- 5. 创建Money
   CMoneyFixedLot *money=new CMoneyFixedLot;
   if(money==NULL)
     {
      //--- failed
      printf(__FUNCTION__+": error creating money");
      ExtExpert.Deinit();
      return(INIT_FAILED);
     }
//--- 5.1 将Money添加到Expert
   if(!ExtExpert.InitMoney(money))
     {
      //--- failed
      printf(__FUNCTION__+": error initializing money");
      ExtExpert.Deinit();
      return(INIT_FAILED);
     }
//--- 5.2 设置Money参数
   money.Percent(Money_FixLot_Percent);
   money.Lots(Money_FixLot_Lots);
//--- 6. 参数校验
   if(!ExtExpert.ValidationSettings())
     {
      //--- failed
      ExtExpert.Deinit();
      return(INIT_FAILED);
     }
//--- 7. 初始化指标和时间序列
   if(!ExtExpert.InitIndicators())
     {
      //--- failed
      printf(__FUNCTION__+": error initializing indicators");
      ExtExpert.Deinit();
      return(INIT_FAILED);
     }
//--- ok
   return(INIT_SUCCEEDED);
  }
//+------------------------------------------------------------------+
//| Deinitialization function of the expert                          |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
  {
   ExtExpert.Deinit();
  }
//+------------------------------------------------------------------+
//| "Tick" event handler function                                    |
//+------------------------------------------------------------------+
void OnTick()
  {
   ExtExpert.OnTick();
  }
//+------------------------------------------------------------------+
//| "Trade" event handler function                                   |
//+------------------------------------------------------------------+
void OnTrade()
  {
   ExtExpert.OnTrade();
  }
//+------------------------------------------------------------------+
//| "Timer" event handler function                                   |
//+------------------------------------------------------------------+
void OnTimer()
  {
   ExtExpert.OnTimer();
  }
```