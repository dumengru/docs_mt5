# ZigZag注释.mq5

source: `{{ page.path }}`

## 参数设置

```cpp
#property indicator_chart_window    // 主窗口
#property indicator_buffers 3       // 缓冲区数量
#property indicator_plots   1       // 图形数量
//--- plot ZigZag
#property indicator_label1  "ZigZag"        // 指标名称
#property indicator_type1   DRAW_SECTION    // 指标类型
#property indicator_color1  clrRed          // 指标颜色
#property indicator_style1  STYLE_SOLID     // 线类型
#property indicator_width1  1               // 线宽度
//--- 指标参数
input int InpDepth    =12;  // Depth
input int InpDeviation=5;   // Deviation
input int InpBackstep =3;   // Back Step
//--- 指标缓冲区
double    ZigZagBuffer[];      // 主缓冲区
double    HighMapBuffer[];     // 波峰缓冲区
double    LowMapBuffer[];      // 波谷缓冲区

int       ExtRecalc=3;         // number of last extremes for recalculation

// 搜寻模式
enum EnSearchMode
  {
   Extremum=0, // 搜索第一个极值
   Peak=1,     // 搜索下一个波峰
   Bottom=-1   // 搜索下一个波谷
  };
```

## OnInit

```cpp
void OnInit()
  {
//--- 绑定缓冲区
   SetIndexBuffer(0,ZigZagBuffer,INDICATOR_DATA);
   SetIndexBuffer(1,HighMapBuffer,INDICATOR_CALCULATIONS);
   SetIndexBuffer(2,LowMapBuffer,INDICATOR_CALCULATIONS);
//--- 设置缩写和精度
   string short_name=StringFormat("ZigZag(%d,%d,%d)",InpDepth,InpDeviation,InpBackstep);
   IndicatorSetString(INDICATOR_SHORTNAME,short_name);
   PlotIndexSetString(0,PLOT_LABEL,short_name);
   IndicatorSetInteger(INDICATOR_DIGITS,_Digits);
//--- 设置空值为0.0
   PlotIndexSetDouble(0,PLOT_EMPTY_VALUE,0.0);
  }
```

## OnCalculate

```cpp
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
//--- 判断历史数据>100
   if(rates_total<100)
      return(0);
//--- 索引计数器
   int    i=0;
//--- 起始点
   int    start=0,extreme_counter=0,extreme_search=Extremum;
//--- 起始点偏移后计数, 回退计数, 前一最高价位置, 前一最低价位置
   int    shift=0,back=0,last_high_pos=0,last_low_pos=0;
//--- 临时极值价
   double val=0,res=0;
//--- 当前最低价, 当前最高价, 前一最高价, 前一最低价
   double curlow=0,curhigh=0,last_high=0,last_low=0;
//--- 初始化数据
   if(prev_calculated==0)
     {
      ArrayInitialize(ZigZagBuffer,0.0);
      ArrayInitialize(HighMapBuffer,0.0);
      ArrayInitialize(LowMapBuffer,0.0);
      start=InpDepth;
     }

//--- ZigZag was already calculated before
   if(prev_calculated>0)
     {
      i=rates_total-1;
      //--- searching for the third extremum from the last uncompleted bar
      while(extreme_counter<ExtRecalc && i>rates_total-100)
        {
         res=ZigZagBuffer[i];
         if(res!=0.0)
            extreme_counter++;
         i--;
        }
      i++;
      start=i;

      //--- what type of exremum we search for
      if(LowMapBuffer[i]!=0.0)
        {
         curlow=LowMapBuffer[i];
         extreme_search=Peak;
        }
      else
        {
         curhigh=HighMapBuffer[i];
         extreme_search=Bottom;
        }
      //--- clear indicator values
      for(i=start+1; i<rates_total && !IsStopped(); i++)
        {
         ZigZagBuffer[i] =0.0;
         LowMapBuffer[i] =0.0;
         HighMapBuffer[i]=0.0;
        }
     }

//--- 填充波谷波峰缓冲区
   for(shift=start; shift<rates_total && !IsStopped(); shift++)
     {
      /*
         1. 只要 InpBackstep 之内有最低价出现, LowMapBuffer 会将旧的最低价设为0, 只保存新的最低价
         2. 若是在 InpBackstep 之外, 且波动超过阈值, 则 LowMapBuffer 会保存旧的和新的两个最低价(因为回退周期之外超过阈值会产生一个波峰)
      */
      //--- 1. 计算波谷
      //--- 1.1 获取从shift开始最近InpDepth范围最低价
      val=low[Lowest(low,InpDepth,shift)];
      //--- 1.2 InpDepth周期内未出现新的最低价
      if(val==last_low)
         val=0.0;
      else
        {
         //--- 1.3 InpDepth周期内出现并保存新的最低价
         last_low=val;
         //--- 1.4 当前价-最低价>阈值
         if((low[shift]-val)>InpDeviation*_Point)
            val=0.0;
         else
           {
            //--- 1.5 未超出阈值, 回退计算 InpBackstep 步
            //--- 如果相邻两个最低价周期在 InpBackstep 步内且前一个最低价比当前最低价高, 则将前一个最低价置为0
            for(back=1; back<=InpBackstep; back++)
              {
               res=LowMapBuffer[shift-back];
               if((res!=0) && (res>val))
                  LowMapBuffer[shift-back]=0.0;
              }
           }
        }
      //--- 1.6 
      if(low[shift]==val)
         LowMapBuffer[shift]=val;
      else
         LowMapBuffer[shift]=0.0;
      //--- 2. 计算波峰
      //--- 2.1 从shift开始最近InpDepth周期内最高价
      val=high[Highest(high,InpDepth,shift)];
      //--- 2.2 未出现新最高价
      if(val==last_high)
         val=0.0;
      else
        {
         //---- 2.3 出现并保存新最高价
         last_high=val;
         //--- 2.4 新的最高价和当前最高价差大于阈值
         if((val-high[shift])>InpDeviation*_Point)
            val=0.0;
         else
           {
            //--- 2.4 在阈值之内, 允许回退3步查看是否
            for(back=1; back<=InpBackstep; back++)
              {
               res=HighMapBuffer[shift-back];
               if((res!=0) && (res<val))
                  HighMapBuffer[shift-back]=0.0;
              }
           }
        }
      if(high[shift]==val)
         HighMapBuffer[shift]=val;
      else
         HighMapBuffer[shift]=0.0;
     }

//--- set last values
   if(extreme_search==0)   // 首次计算
     {
      last_low=0.0;
      last_high=0.0;
     }
   else                    // 非首次计算
     {
      last_low=curlow;
      last_high=curhigh;
     }

//--- 填充指标缓冲区
   for(shift=start; shift<rates_total && !IsStopped(); shift++)
     {
      res=0.0;
      switch(extreme_search)
        {
         case Extremum:    // 首次计算
            if(last_low==0.0 && last_high==0.0)
              {
               if(HighMapBuffer[shift]!=0)
                 {
                  //--- 已经有波峰价格和位置, 填充波峰数据, 开始寻找波谷
                  last_high=high[shift];
                  last_high_pos=shift;
                  extreme_search=Bottom;
                  ZigZagBuffer[shift]=last_high;
                  res=1;
                 }
               if(LowMapBuffer[shift]!=0.0)
                 {
                  //--- 已经有波谷价格和位置, 填充波谷数据, 开始寻找波峰
                  last_low=low[shift];
                  last_low_pos=shift;
                  extreme_search=Peak;
                  ZigZagBuffer[shift]=last_low;
                  res=1;
                 }
              }
            break;
         case Peak:
            if(LowMapBuffer[shift]!=0.0 && LowMapBuffer[shift]<last_low && HighMapBuffer[shift]==0.0)
              {
               ZigZagBuffer[last_low_pos]=0.0;
               last_low_pos=shift;
               last_low=LowMapBuffer[shift];
               ZigZagBuffer[shift]=last_low;
               res=1;
              }
            if(HighMapBuffer[shift]!=0.0 && LowMapBuffer[shift]==0.0)
              {
               last_high=HighMapBuffer[shift];
               last_high_pos=shift;
               ZigZagBuffer[shift]=last_high;
               extreme_search=Bottom;
               res=1;
              }
            break;
         case Bottom:   
            // 如果寻找波谷时出现连续两个最高价, 保留最大的
            if(HighMapBuffer[shift]!=0.0 && HighMapBuffer[shift]>last_high && LowMapBuffer[shift]==0.0)
              {
               ZigZagBuffer[last_high_pos]=0.0;
               last_high_pos=shift;
               last_high=HighMapBuffer[shift];
               ZigZagBuffer[shift]=last_high;
              }
            // 填充波谷, 并准备寻找波峰
            if(LowMapBuffer[shift]!=0.0 && HighMapBuffer[shift]==0.0)
              {
               last_low=LowMapBuffer[shift];
               last_low_pos=shift;
               ZigZagBuffer[shift]=last_low;
               extreme_search=Peak;
              }
            break;
         default:
            return(rates_total);
        }
     }

//--- return value of prev_calculated for next call
   return(rates_total);
  }
```

## 辅助函数

```cpp
//--- 寻找时间序列 high 从 start 开始 depth 范围内的最低价索引
int Highest(const double &array[],const int depth,const int start)
  {
   if(start<0)
      return(0);
//--- 1. 初始最大值和索引
   double max=array[start];
   int    index=start;
//--- 2. 逐个比较最近 depth 条数据最大值
   for(int i=start-1; i>start-depth && i>=0; i--)
     {
      if(array[i]>max)
        {
         index=i;
         max=array[i];
        }
     }
//--- 3. 返回最大值索引
   return(index);
  }

//--- 寻找时间序列low从start开始depth范围内的最低价索引
int Lowest(const double &array[],const int depth,const int start)
  {
   if(start<0)
      return(0);
//--- 1. 初始最小值和索引
   double min=array[start];
   int    index=start;
//--- 2. 逐个比较最近 depth 条数据最小值
   for(int i=start-1; i>start-depth && i>=0; i--)
     {
      if(array[i]<min)
        {
         index=i;
         min=array[i];
        }
     }
//--- 3. 返回最小值索引
   return(index);
  }
```
