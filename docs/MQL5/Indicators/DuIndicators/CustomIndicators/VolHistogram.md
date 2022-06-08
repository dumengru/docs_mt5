# VolHistogram.mq5

source: `{{ page.path }}`

计算增仓/减仓, 上涨/下跌

```cpp
//+------------------------------------------------------------------+
//|                                            OHLC_Volume_Histo.mq5 |
//|                        Copyright 2018, MetaQuotes Software Corp. |
//|                                                 https://mql5.com |
//+------------------------------------------------------------------+
#property copyright "Copyright 2018, MetaQuotes Software Corp."
#property link      "https://mql5.com"
#property version   "1.00"
#property description "OHLC Volume Histogram indicator"
#property indicator_separate_window
#property indicator_buffers 2
#property indicator_plots   1
//--- plot VolHist
#property indicator_label1  "VolHis"
#property indicator_type1   DRAW_COLOR_HISTOGRAM
#property indicator_color1  clrLimeGreen,clrRed,clrDarkGray
#property indicator_style1  STYLE_SOLID
#property indicator_width1  2


/*
判断连续bar增仓/减仓, 上涨/下跌
1. 最新bar编号为0, 前一bar编号为1, 向前以此类推
2. 判断bar1成交量在bar1-barn中成交量最大/最小
3. 计算bar2-barn成交量均值
4. 增仓: bar1(成交量最大)-均值, 减仓: bar1(成交量最小) - 均值
5. 上涨: open<close, 下跌: open>close
*/

input int NBars=5;

//--- indicator buffers
double         BufferVolHist[];
double         BufferColors[];
//+------------------------------------------------------------------+
//| Custom indicator initialization function                         |
//+------------------------------------------------------------------+
int OnInit()
  {
//--- indicator buffers mapping
   SetIndexBuffer(0,BufferVolHist,INDICATOR_DATA);
   SetIndexBuffer(1,BufferColors,INDICATOR_COLOR_INDEX);
//--- setting indicator parameters
   IndicatorSetString(INDICATOR_SHORTNAME,"Add/Sub Volume");
   IndicatorSetInteger(INDICATOR_DIGITS,Digits());
//--- setting buffer arrays as timeseries
   ArraySetAsSeries(BufferVolHist,true);
   ArraySetAsSeries(BufferColors,true);
//---
   return(INIT_SUCCEEDED);
  }
//+------------------------------------------------------------------+
//| Custom indicator iteration function                              |
//+------------------------------------------------------------------+
int OnCalculate(const int rates_total,
                const int prev_calculated,
                const datetime &time[],
                const double &open[],
                const double &high[],
                const double &low[],
                const double &close[],
                const long &tick_volume[],
                const long &volume[],
                const int &spread[])
  {
//--- 转换时间序列索引
   ArraySetAsSeries(open,true);
   ArraySetAsSeries(high,true);
   ArraySetAsSeries(low,true);
   ArraySetAsSeries(close,true);
   ArraySetAsSeries(tick_volume,true);
//--- 判断bar数量
   if(rates_total<NBars) return 0;
//--- 判断未计算bar数量(首次计算)
   int limit=rates_total-prev_calculated;
   if(limit>1)
     {
      limit=rates_total-1;
      ArrayInitialize(BufferVolHist,EMPTY_VALUE);
      ArrayInitialize(BufferColors,2);
     }
     
     int total=limit>1 ? limit-NBars:limit;
//--- 填充数据(从前向后)
   for(int i=total; i>0 && !IsStopped(); i--)
     {
      long avg_vol=0;
      long diff_vol=0;
      //--- bari应该和bar(i+1),bar(i+2)... 比较
      if(ArrayMaximum(tick_volume,i,NBars)==i)
      {
         // 遍历成交量求和
         for(int j=i+1;j<i+NBars;j++)
            avg_vol+=tick_volume[j];
         // 求成交量均值
         avg_vol/=(NBars-1); 
         // 求增仓/减仓
         diff_vol=tick_volume[i]-avg_vol;
      }
      if(ArrayMinimum(tick_volume,i,NBars)==i)
      {
         for(int j=i+1;j<i+NBars;j++)
            avg_vol+=tick_volume[j];
         avg_vol/=(NBars-1); 
         diff_vol=tick_volume[i]-avg_vol;
      }
      
      //--- 填充数据和颜色
      BufferVolHist[i]=double(diff_vol);
      BufferColors[i]=(open[i]<close[i] ? 0 : open[i]>close[i] ? 1 : 2);
     }
   
//--- return value of prev_calculated for next call
   return(rates_total);
  }
//+------------------------------------------------------------------+
```