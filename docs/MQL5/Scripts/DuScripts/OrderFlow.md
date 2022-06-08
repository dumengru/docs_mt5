# OrderFlow

source: `{{ page.path }}`

## OrderFlow源码

```cpp
//+------------------------------------------------------------------+
//|                                                    FootPrint.mq5 |
//|                                  Copyright 2022, MetaQuotes Ltd. |
//|                                             https://www.mql5.com |
//+------------------------------------------------------------------+
#include <Arrays\ArrayString.mqh>
#include <Arrays\ArrayDouble.mqh>
#include <Canvas\Canvas.mqh>
#include <Charts\Chart.mqh>


string SYMBOL="FUTGCJUN22";
ENUM_TIMEFRAMES PERIOD=PERIOD_M1;
int COUNT=5000;      // 获取最近条少条Tick数据
int ADJUST=2;        // 数据间隔几个步长

CCanvas     m_canvas;
CChart      m_chart;


class OrderData
{
public:
   datetime    o_time;        // 发生时间
   int         o_delta;       // Delta: 主动买-主动卖
   double      o_poc;         // 成交量最大价格
   
   double      o_price[];     // 成交价格
   ulong       o_buy[];       // 主动买量
   ulong       o_sell[];      // 主动卖量
   
   uint        o_data_total;  // 数据量
   ulong       o_total[];     // 总量(为计算POC)
   
   // 高开低收价, 方便绘制K线图
   double      o_open;
   double      o_close;
   double      o_high;
   double      o_low;
   
   void        Resize();      // 重新分配内存
   void        Calculate();   // 计算delta, poc, high, low
   OrderData();
   ~OrderData();
};
OrderData::OrderData(void):o_data_total(0)
{}
OrderData::~OrderData(void)
{}
//--- 重新分配内存
void OrderData::Resize(void)
{
   ArrayResize(o_price,o_data_total);
   ArrayResize(o_buy,o_data_total);
   ArrayResize(o_sell,o_data_total);
   ArrayResize(o_total,o_data_total);
}

//--- 计算delta, poc, high, low
void OrderData::Calculate(void)
{
   for(uint i=0;i<o_data_total;i++)
   {
      o_delta+=int(o_buy[i]-o_sell[i]);
   }
   o_poc=o_price[ArrayMaximum(o_total)];
   o_high=o_price[ArrayMaximum(o_price)];
   o_low=o_price[ArrayMinimum(o_price)];
}


class OrderFlow
{
   string            m_symbol;
   ENUM_TIMEFRAMES   m_period;
   int               m_count;
   double            m_adjust;      // 间隔点数
   int               m_data_total;  // 数据量
   
   
   MqlTick           m_ticks[];     // 原始Tick数据
   MqlDateTime       m_dt;          // 时间结构体
public:
   OrderData         *order_data[]; // 标准数据数组
public:
   //--- 坐标相关
   CArrayString      x_label;       // 存储x轴标签
   CArrayString      x_time;        // 存储x轴时间
   CArrayDouble      y_label;       // 存储y轴标签
   //--- x, y轴计算逻辑不同, x轴简单计算, y轴需要比例计算
   int               x_gap;         // x轴标签间隔
   int               y_length;      // y轴绘图区长度
   double            y_psize;       // y轴价格长度(最大值价格-最小值)
   CPoint            c_origin;      // 左下角绘图起点坐标
   

public:
   OrderFlow();
   ~OrderFlow();
   
   bool        Init();        // 填充数据
   bool        SetCoordinate(const int &x,const int &y,const int &x_size,const int &y_size);  // 构造坐标系
   bool        CalCoordinate(const datetime &x,const double &y,CPoint &cp);      // 计算y坐标
   bool        CalYCoordinate(const double &y,int &cy);
   bool        Clear();       // 清空动态数组
   void        OrderPrint();

};
void OrderFlow::OrderPrint(void)
{
/*
   for(int i=0;i<m_data_total;i++)
   {
      Print("时间: ",order_data[i].o_time);
      Print("delta: ",order_data[i].o_delta, "poc: ",order_data[i].o_poc, "total:");
      Print("open: ",order_data[i].o_open, "high: ", order_data[i].o_high, "low: ", order_data[i].o_low, "close: ", order_data[i].o_close);
      ArrayPrint(order_data[i].o_price);
   }
*/
   
   int x=x_label.Total();
   int y=y_label.Total();
   

   Print("x_label: ", x, " y_label: ", y);
   for(int i=0;i<x;i++)
   {
      Print("x: ", x_label.At(i));
   }
   for(int i=0;i<y;i++)
   {
      Print("y: ", y_label.At(i));
   }
}

OrderFlow::OrderFlow(void):m_symbol(SYMBOL),
                           m_period(PERIOD),
                           m_count(COUNT),
                           m_data_total(0),
                           x_gap(0),
                           y_length(0)
{
   m_adjust=Point()*ADJUST;
   c_origin.x=0;
   c_origin.y=0;
}

OrderFlow::~OrderFlow(void)
{
   if(!Clear())
      Alert("order_data 未正常释放");
}

bool OrderFlow::Clear(void)
{
   for(int i=0;i<m_data_total;i++)
     {
      if(CheckPointer(order_data[i])==POINTER_DYNAMIC)
         delete order_data[i];
      order_data[i]=NULL;
     }
   m_data_total=0;
   
   if(ArrayResize(order_data,0)==-1)
      return(false);
   return(true);
}

bool OrderFlow::Init(void)
{
   // 获取数据
   if(CopyTicks(m_symbol,m_ticks,COPY_TICKS_TRADE,0,m_count)<0)
      return(false);

   // 设置标记时间和价格
   datetime flag_time=0;
   double   flag_price;
   double   base_price;    // 标准价格
   OrderData *flag_order;
   
   for(int i=0;i<m_count;i++)
   {  
//--- flags统计: buy: 312,56   sell: 344,88   buy/sell:120
      if(m_ticks[i].flags==120)
      {
         // Print("过滤flags: 120");
         continue;
      }
      
      // 以分钟为单位, 将S设为0
      TimeToStruct(m_ticks[i].time,m_dt);
      m_dt.sec=0;
      m_ticks[i].time=StructToTime(m_dt);
      
      if(flag_time!=m_ticks[i].time)
      {
         if(m_data_total!=0)
         {
            // 计算前一分钟的delta和poc
            order_data[m_data_total-1].Calculate();
            order_data[m_data_total-1].o_close=m_ticks[i-1].last;
         }
      
         // 增加数据空间
         m_data_total+=1;
         ArrayResize(order_data,m_data_total);
         // 添加新数据指针
         order_data[m_data_total-1]=new OrderData;
         
         // 填充时间和开盘价
         order_data[m_data_total-1].o_time=m_ticks[i].time;
         order_data[m_data_total-1].o_open=m_ticks[i].last;
         // 添加x_label
         x_label.Add(IntegerToString(m_dt.min));
         x_time.Add(TimeToString(m_ticks[i].time));
         
         // 更新标记时间
         flag_time=m_ticks[i].time;
      }
      // 获取数据指针
      flag_order=order_data[m_data_total-1];
      
      // 计算标准价格
      flag_price=m_ticks[i].last;
      int multiple=int(MathPow(10,Digits()));
      base_price=NormalizeDouble(flag_price-double(int(flag_price*multiple)%int(m_adjust*multiple))/double(multiple),Digits());
      
      // 将价格添加到y_label
      y_label.Sort();
      if(y_label.Search(base_price)<0)
         y_label.Add(base_price);
      
      // 搜索标准价格是否已保存
      int p_index=ArraySearch(flag_order.o_price,base_price);
      if(p_index<0)
      {
         // 没找到, 说明是新价格
         // 增加空间
         flag_order.o_data_total+=1;
         flag_order.Resize();
         // 填充价格和量, 一定要用基准价填充
         flag_order.o_price[flag_order.o_data_total-1]=base_price;
         flag_order.o_total[flag_order.o_data_total-1]=m_ticks[i].volume;
         if(m_ticks[i].flags==312 || m_ticks[i].flags==56)
         {
            // 主动买
            flag_order.o_buy[flag_order.o_data_total-1]=m_ticks[i].volume;
            flag_order.o_sell[flag_order.o_data_total-1]=0;
         }
         else if(m_ticks[i].flags==344 || m_ticks[i].flags==88)
         {
            // 主动卖
            flag_order.o_buy[flag_order.o_data_total-1]=0;
            flag_order.o_sell[flag_order.o_data_total-1]=m_ticks[i].volume;
         }
         else
         {
            Print("新成交量flag超出预期: ",m_ticks[i].flags," time:",m_ticks[i].time, " msc: ",m_ticks[i].time_msc);
         }
      }
      else
      {
         // 找到了, 说明价格已存在
         if(m_ticks[i].flags==312 || m_ticks[i].flags==56)
         {
            // 主动买
            flag_order.o_total[p_index]+=m_ticks[i].volume;
            flag_order.o_buy[p_index]+=m_ticks[i].volume;
         }
         else if(m_ticks[i].flags==344 || m_ticks[i].flags==88)
         {
            // 主动卖
            flag_order.o_total[p_index]+=m_ticks[i].volume;
            flag_order.o_sell[p_index]+=m_ticks[i].volume;
         }
         else
         {
            Print("旧成交量flag超出预期: ",m_ticks[i].flags," time:",m_ticks[i].time, " msc: ",m_ticks[i].time_msc);
         }
      }
   }

   // 计算最后一分钟的delta和poc, 记录收盘价
   order_data[m_data_total-1].Calculate();
   order_data[m_data_total-1].o_close=m_ticks[m_count-1].last;
   
   return(true);
}

//--- 输入左下角坐标(x, y)和绘图区宽高
bool OrderFlow::SetCoordinate(const int &x,const int &y,const int &x_size,const int &y_size)
{
   // 计算x轴标签距离
   x_gap=int((x_size-x_size%x_label.Total())/x_label.Total());
   // 计算y轴价格差
   y_psize=y_label.At(y_label.Total()-1)-y_label.At(0);
   
   y_length=y_size;
   c_origin.x=x;
   c_origin.y=y;
   
   
   return(true);
}

bool OrderFlow::CalCoordinate(const datetime &x,const double &y,CPoint &cp)
{
   if(x_gap==0)
   {
      Print("坐标系未初始化");
      return(false);
   }
   
   // x_time是未排序数组, 只能线性搜索
   int x_index=x_time.SearchLinear(TimeToString(x));
   if(x_index<0)
   {
      Print("未找到x坐标");
      return(false);
   }
   else
   {
      // 计算出对应X坐标
      cp.x=c_origin.x+x_index*x_gap;
   }

   // 计算y坐标(由于最高价和最低价啊不标准,y坐标可能超过最大值)
   cp.y=c_origin.y-int((y-y_label.At(0))/y_psize*y_length);
   
   return(true);
}

//--- 输入价格计算y坐标
bool OrderFlow::CalYCoordinate(const double &y,int &cy)
{
   // 计算y坐标
   cy=c_origin.y-int((y-y_label.At(0))/y_psize*y_length);

   return(true);
}

bool DrawOrderFlow()
{
  //--- 创建图表
   CreateChart();
  
   //--- 创建画板
   int width=int(m_chart.WidthInPixels()/3*2);
   int height=m_chart.HeightInPixels(0);
   if(!m_canvas.CreateBitmapLabel("SampleCanvas",0,0,width,height,COLOR_FORMAT_ARGB_RAW))
   // if(!m_canvas.Create("SampleCanvas",width,height,COLOR_FORMAT_ARGB_RAW))
     {
      Print("Error creating canvas: ",GetLastError());
      return(false);
     }
//--- 填充颜色, 设置字体大小
   m_canvas.Erase(ARGB(255, 175,238,238));
   m_canvas.FontSizeSet(-60);
   
//--- 定义矩形绘图区(绘图区坐标相对画板变动)
   int m_left=20, m_top=20, m_right=60, m_bottom=50;    // 边距
   m_canvas.Rectangle(m_left, m_top, width-m_right, height-m_bottom, clrBlack);
   
   OrderFlow m_order;
   m_order.Init();
   // m_order.OrderPrint();
   
   // 确定主图和坐标轴间隔
   int p_margin=40;     // 图形中心区域和图框左,下,上间隔
   int x=m_left+p_margin, y=height-m_bottom-p_margin;
   int x_size=width-m_left-m_right-p_margin, y_size=height-m_top-m_bottom-p_margin*2;
   m_order.SetCoordinate(x,y,x_size,y_size);
   
   //--- 绘制x轴
   double y_tmp;        // y坐标临时变量
   int clrKline;        // 颜色临时变量
   uint k_size;         // K线数据量临时变量
   string label_buy, label_sell, label_delta;    // 标签临时变量
   
   int line_size=5;     // 坐标轴标记线长度
   int  w_font=m_canvas.TextWidth("00"),h_font=int(m_canvas.TextHeight("00")/2);  // 字体宽和高/2
   int k_gap=3;         // k线一半宽度
   CPoint c_point;      // 坐标轴坐标
   CPoint open_point, high_point, low_point, close_point;   // K线高开低收价坐标
   for(int i=0;i<m_order.x_label.Total();i++)
   {
      // 计算x坐标(原点+gap), y坐标原点-margin
      c_point.x=m_order.x_gap*i+m_order.c_origin.x;
      c_point.y=m_order.c_origin.y+p_margin;
      // 绘制横坐标记号点
      m_canvas.Line(c_point.x,c_point.y,c_point.x,c_point.y-line_size,clrBlack);
      m_canvas.TextOut(c_point.x,c_point.y,m_order.x_label.At(i),clrBlack,TA_TOP|TA_CENTER);
      
      // 绘制蜡烛图
      // 1. 计算x坐标
      open_point.x=c_point.x;high_point.x=c_point.x;low_point.x=c_point.x;close_point.x=c_point.x;
      // 2. 计算y坐标
      y_tmp=m_order.order_data[i].o_open;
      m_order.CalYCoordinate(y_tmp,open_point.y);
      y_tmp=m_order.order_data[i].o_high;
      m_order.CalYCoordinate(y_tmp,high_point.y);
      y_tmp=m_order.order_data[i].o_low;
      m_order.CalYCoordinate(y_tmp,low_point.y);
      y_tmp=m_order.order_data[i].o_close;
      m_order.CalYCoordinate(y_tmp,close_point.y);
      
      // 定义K线颜色
      clrKline=(
               open_point.y>close_point.y?ARGB(255,255,0,0) :
               open_point.y<close_point.y?ARGB(255,0,255,0):
               ARGB(255,0,0,255)
      );
      // 绘制K线:(最高最低价的线被设置为分割线)
      m_canvas.FillRectangle(open_point.x-k_gap,open_point.y,close_point.x+k_gap,close_point.y,clrKline);
      m_canvas.Line(high_point.x,high_point.y,low_point.x,low_point.y,clrBlack);
      
      // 绘制OrderFlow
      k_size=m_order.order_data[i].o_data_total;
      for(uint j=0;j<k_size;j++)
      {
         y_tmp=m_order.order_data[i].o_price[j];
         m_order.CalYCoordinate(y_tmp,c_point.y);
         label_buy=DoubleToString(m_order.order_data[i].o_buy[j],0);
         label_sell=DoubleToString(m_order.order_data[i].o_sell[j],0);

         // 主动卖在左, 主动买在右
         m_canvas.TextOut(c_point.x-k_gap,c_point.y,label_sell,clrBlack,TA_RIGHT|TA_VCENTER);
         m_canvas.TextOut(c_point.x+k_gap,c_point.y,label_buy,clrBlack,TA_LEFT|TA_VCENTER);
      }
      // 绘制delta和poc
      {
         // delta在最高价之上1个字符高度
         label_delta=DoubleToString(m_order.order_data[i].o_delta,0);
         m_canvas.TextOut(high_point.x,high_point.y-h_font,label_delta,clrBlack,TA_CENTER|TA_BOTTOM);
         
         // poc用矩形绘制
         y_tmp=m_order.order_data[i].o_poc;
         m_order.CalYCoordinate(y_tmp,c_point.y);
         m_canvas.Rectangle(c_point.x-w_font-k_gap,c_point.y+h_font,c_point.x+w_font+k_gap,c_point.y-h_font,ARGB(250,255,0,0));
      }

   }
   //--- 绘制y轴
   for(int i=0;i<m_order.y_label.Total();i++)
   {
      // 计算x和y坐标
      c_point.x=width-m_right;
      y_tmp=m_order.y_label.At(i);
      m_order.CalYCoordinate(y_tmp,c_point.y);
      // 绘制纵坐标记号点
      m_canvas.Line(c_point.x-line_size,c_point.y,c_point.x,c_point.y,clrBlack);
      // y_label离线太近, 因此右移3个坐标
      m_canvas.TextOut(c_point.x+3,c_point.y,DoubleToString(m_order.y_label[i],Digits()),clrBlack,TA_LEFT|TA_VCENTER);
   }

//--- 设置透明度
   m_canvas.TransparentLevelSet(255);
   m_canvas.Update();
   
   return(true);
}

void CloseOrderFlose()
{
   m_canvas.Destroy();
   m_chart.Detach();
}

//--- 创建数组搜索模板, 找到返回索引, 没找到返回-1
template<typename T>
int ArraySearch(const T &arr[], T &value)
{
   
   int size=ArraySize(arr);
   if(size==0)
      return(-1);
   for(int i=0;i<size;i++)
   {
      // 浮点数比较
      if(MathAbs(value-arr[i])<0.00000001)
         return(i);
   }
   return(-1);
}

void CreateChart()
{
   //--- 使用当前图表
   m_chart.Attach(0);
   //--- 图表样式
   m_chart.Mode(CHART_CANDLES);
   m_chart.BringToTop();
   //--- 图表缩放比例
   m_chart.Scale(8.0);
   //--- 是否展示网格
   m_chart.ShowGrid(true);
   //--- 鼠标拖拽日期可实现缩放
   m_chart.ShowDateScale(true);
   
   //--- 颜色
   m_chart.ColorBackground(clrMintCream);  // 前景色
   m_chart.ColorForeground(clrBlack);      // 背景色
   m_chart.ColorGrid(clrLightSkyBlue);     // 网格色
   m_chart.ColorBarUp(clrRed);             // 阳线色
   m_chart.ColorBarDown(clrLimeGreen);     // 阴线色
   m_chart.ColorCandleBull(clrPink);       // 阳实体色
   m_chart.ColorCandleBear(clrPaleGreen);  // 阴实体色
   m_chart.ColorChartLine(clrGold);        // 十字星色
   
   //----------- 以下属性暂不需要设置 ----------//
   //--- 图表是否根据价格上下滚动
   m_chart.ScaleFix(false);
   //--- 展示全部价格数据, 及图表比例尺(PPB为true才可用)
   m_chart.ScalePPB(false);
   m_chart.PointsPerBar(1.0);
   //--- 将全部数据放在一张图
   m_chart.ScaleFix_11(false);
}

//--- 调整COUNT数值, 适应图表变化
bool AdjustCount()
{
   MqlTick m_ticks[];
   datetime delta_time; // 时间差
   MqlDateTime m_dt;    // 时间结构体
   
   while(!IsStopped())
   {
      if(CopyTicks(SYMBOL,m_ticks,COPY_TICKS_TRADE,0,COUNT)<0)
         return(false);
      else
      {
         delta_time=TimeCurrent()-m_ticks[0].time;
         TimeToStruct(delta_time,m_dt);
         
         //--- 控制K线数量在20-30之间
         if(m_dt.hour*60+m_dt.min<27)
            COUNT+=200;
         else if(m_dt.hour*60+m_dt.min>30)
            COUNT-=200;
         else
            break;
      }
   }
   return(true);
}


int OnStart()
{
   int count=0;  // 计时器
   while(!IsStopped())
   {
      if(count==0)
         AdjustCount();
      else if(count>1)     // 每刷新3次图表调整一次数据量
         count=-1;
      count+=1;
 
      // 3s刷新1次图表
      Sleep(3000);
      if(!DrawOrderFlow())
         break;
   }
   
   CloseOrderFlose();
   return(1);
}
```