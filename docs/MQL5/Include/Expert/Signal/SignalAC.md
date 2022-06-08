# SignalAC.mqh

source: `{{ page.path }}`

## SignalAC

1. 实现`InitIndicators`
    1. 初始化指标和数据
    2. 初始化AC指标
2. 实现`LongCondition`
3. 实现`ShortCondition`

```cpp
#include <Expert\ExpertSignal.mqh>
// wizard description start
//+------------------------------------------------------------------+
//| Description of the class                                         |
//| Title=Signals of indicator 'Accelerator Oscillator'              |
//| Type=SignalAdvanced                                              |
//| Name=Accelerator Oscillator                                      |
//| ShortName=AC                                                     |
//| Class=CSignalAC                                                  |
//| Page=signal_ac                                                   |
//+------------------------------------------------------------------+
// wizard description end
//+------------------------------------------------------------------+
//| Class CSignalAC.                                                 |
//| Purpose: Class of generator of trade signals based on            |
//|          the 'Accelerator Oscillator' indicator.                 |
//| Is derived from the CExpertSignal class.                         |
//+------------------------------------------------------------------+
class CSignalAC : public CExpertSignal
  {
protected:
   CiAC              m_ac;             // object-indicator
   //--- "weights" of market models (0-100)
   int               m_pattern_0;      // model 0 "first analyzed bar has required color"
   int               m_pattern_1;      // model 1 "there is a condition for entering the market"
   int               m_pattern_2;      // model 2 "condition for entering the market has just appeared"

public:
                     CSignalAC(void);
                    ~CSignalAC(void);
   //--- methods of adjusting "weights" of market models
   void              Pattern_0(int value)        { m_pattern_0=value;         }
   void              Pattern_1(int value)        { m_pattern_1=value;         }
   void              Pattern_2(int value)        { m_pattern_2=value;         }
   //--- method of creating the indicator and timeseries
   virtual bool      InitIndicators(CIndicators *indicators);
   //--- methods of checking if the market models are formed
   virtual int       LongCondition(void);
   virtual int       ShortCondition(void);

protected:
   //--- method of initialization of the indicator
   bool              InitAC(CIndicators *indicators);
   //--- methods of getting data
   double            AC(int ind)                 { return(m_ac.Main(ind));    }
   double            DiffAC(int ind)             { return(AC(ind)-AC(ind+1)); }
  };
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
CSignalAC::CSignalAC(void) : m_pattern_0(90),
                             m_pattern_1(50),
                             m_pattern_2(30)
  {
  }
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
CSignalAC::~CSignalAC(void)
  {
  }
//+------------------------------------------------------------------+
//| Create indicators.                                               |
//+------------------------------------------------------------------+
bool CSignalAC::InitIndicators(CIndicators *indicators)
  {
//--- check pointer
   if(indicators==NULL)
      return(false);
//--- initialization of indicators and timeseries of additional filters
   if(!CExpertSignal::InitIndicators(indicators))
      return(false);
//--- create and initialize AC indicator
   if(!InitAC(indicators))
      return(false);
//--- ok
   return(true);
  }
//+------------------------------------------------------------------+
//| Initialize AC indicators.                                        |
//+------------------------------------------------------------------+
bool CSignalAC::InitAC(CIndicators *indicators)
  {
//--- check pointer
   if(indicators==NULL)
      return(false);
//--- add object to collection
   if(!indicators.Add(GetPointer(m_ac)))
     {
      printf(__FUNCTION__+": error adding object");
      return(false);
     }
//--- initialize object
   if(!m_ac.Create(m_symbol.Name(),m_period))
     {
      printf(__FUNCTION__+": error initializing object");
      return(false);
     }
//--- ok
   return(true);
  }
//+------------------------------------------------------------------+
//| "Voting" that price will grow.                                   |
//+------------------------------------------------------------------+
int CSignalAC::LongCondition(void)
  {
/*
1. idx: tick策略索引为0, 否则为1
2. DiffAC: 指定索引AC - 前一个AC
3. AC: 指定索引AC
4. 若是Tick策略, 编号0为最新bar0, 1为前一个bar1 ...
*/
   int result=0;
   int idx   =StartIndex();
//--- bar0<bar1, 不符合做多条件, idx=1
   if(DiffAC(idx++)<0.0)
      return(result);
//--- bar0>bar1, pattern_0满足
   if(IS_PATTERN_USAGE(0))
      result=m_pattern_0;
//--- bar1<bar 2, 后续条件不满足
   if(DiffAC(idx)<0.0)
      return(result);
//--- 1. bar1>=0直接跳
   if(AC(idx++)<0.0)
     {
      //--- bar1<0, idx=2
      //--- 如果bar2<bar3, 后续条件不满足, idx=3
      if(DiffAC(idx++)<0.0)
         return(result);
     }
//--- pattern_1条件: 
//--- 1. bar1>=0, bar0>bar1>bar2
//--- 2. bar1<0, bar0>bar1>bar2>bar3
   if(IS_PATTERN_USAGE(1))
      result=m_pattern_1;
//--- pattern_2条件: 
//--- 1. bar1>=0, bar0>bar1>bar2>bar3
//--- 2. bar1<0, bar0>bar1>bar2>bar3>bar4
   if(IS_PATTERN_USAGE(2) && DiffAC(idx)<0.0)
      result=m_pattern_2;
//--- return the result
   return(result);
  }
//+------------------------------------------------------------------+
//| "Voting" that price will fall.                                   |
//+------------------------------------------------------------------+
int CSignalAC::ShortCondition(void)
  {
   int result=0;
   int idx   =StartIndex();
//--- if the first analyzed bar is "green", don't "vote" for selling
   if(DiffAC(idx++)>0.0)
      return(result);
//--- first analyzed bar is "red" (the indicator has no objections to selling)
   if(IS_PATTERN_USAGE(0))
      result=m_pattern_0;
//--- if the second analyzed bar is "green", there is no condition for selling
   if(DiffAC(idx)>0.0)
      return(result);
//--- second analyzed bar is "red" (the condition for selling may be fulfilled)
//--- if the second analyzed bar is greater than zero, we need to analyze the third bar
   if(AC(idx++)>0.0)
     {
      //--- if the third analyzed bar is "green", there is no condition for selling
      if(DiffAC(idx++)>0.0)
         return(result);
     }
//--- there us a condition for selling
   if(IS_PATTERN_USAGE(1))
      result=m_pattern_1;
//--- if the previously analyzed bar is "green", the condition for selling has just been fulfilled
   if(IS_PATTERN_USAGE(2) && DiffAC(idx)>0.0)
      result=m_pattern_2;
//--- return the result
   return(result);
  }
```