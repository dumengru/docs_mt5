# DrawFlow.mqh

source: `{{ page.path }}`

定义OrderFlow绘图流程

```cpp
#include "Flow.mqh"
#include <Canvas\Canvas.mqh>
#include <Charts\Chart.mqh>


class CDrawFlow:public CFlow
{
   CCanvas  m_canvas;
   CChart   m_chart;

//--- 画布大小(初始不能为0)
   int      canvas_width;
   int      canvas_height;
//--- 绘图区设置(绘图区和画布间隔, K线区和绘图区间隔margin)
   int      canvas_left;
   int      canvas_right;
   int      canvas_top;
   int      canvas_bottom;
   int      canvas_margin;
   int      canvas_xsize;
   int      canvas_ysize;
   
   int      width_font;       // 字体宽(高是宽的2倍)
   int      width_flow;       // 一个flow的宽度
   int      width_k;          // k线一半宽度 3 
   int      width_poc;        // poc一半宽度
   int      width_scale;      // 刻度线长度
//--- 价格时间范围(毫秒)
   ulong    flow_fromtime;
   ulong    flow_totime;
   
//--- 计算过程中用到的临时变量
   double   tmp_yprice;       // y轴价格临时变量(用来计算坐标)
   int      tmp_kclr;         // 颜色临时变量
   int      tmp_ktotals;      // 图表K线数量
   uint     tmp_ksize;        // 一根k内价格数量
   string   tmp_xlabel;       // x轴标签
   string   tmp_blabel;       // 主动卖标签
   string   tmp_slabel;       // 主动卖标签
   string   tmp_dlabel;       // delta标签
   CPoint   tmp_point;        // 坐标点临时变量
   CPoint   origin_point;     // 原点坐标
   CPoint   open_point;       // K线高开低收价坐标
   CPoint   high_point;
   CPoint   low_point;
   CPoint   close_point;   

public:
   // 创建图表
   bool        CreateChart();
   // 创建画布
   bool        CreateCanvas();
   // 更新数据
   bool        Update();
   // 启动
   bool        RunFlow();
   
   CDrawFlow();
   ~CDrawFlow();
};


CDrawFlow::CDrawFlow(): canvas_width(1),
                        canvas_height(1),
                        canvas_left(30),
                        canvas_right(60),
                        canvas_top(30),
                        canvas_bottom(60),
                        canvas_margin(30),
                        canvas_xsize(0),
                        canvas_ysize(0),
                        width_font(0),
                        width_flow(0),
                        width_poc(0),
                        width_k(3),
                        width_scale(5),
                        flow_fromtime(0),
                        flow_totime(0),
                        tmp_yprice(0),
                        tmp_kclr(0),
                        tmp_ktotals(0),
                        tmp_ksize(0),
                        tmp_xlabel(""),
                        tmp_blabel(""),
                        tmp_slabel(""),
                        tmp_dlabel("")          
{
   tmp_point.x=0;
   tmp_point.y=0;

   open_point.x=0;
   open_point.y=0;

   high_point.x=0;
   high_point.y=0;
   
   low_point.x=0;
   low_point.y=0;
   
   close_point.x=0;
   close_point.y=0;
}

CDrawFlow::~CDrawFlow()
{
   m_canvas.Destroy();
   m_chart.Detach();
   CFlow::Clear();
}


bool CDrawFlow::CreateCanvas()
{

   if(!m_canvas.CreateBitmapLabel("FlowCanvas",0,0,canvas_width,canvas_height,COLOR_FORMAT_ARGB_RAW))
     {
      Print("画布创建失败: ",GetLastError());
      return(false);
     }

   return(true);
}

bool CDrawFlow::Update(void)
{
//--- 更新画布大小
   canvas_width=m_chart.WidthInPixels();
   canvas_height=m_chart.HeightInPixels(0);
   m_canvas.Resize(canvas_width,canvas_height);

//--- 字体大小 -50对应宽/高6px, 1个Flow宽度=6*6+2*2=40
//--- 左右各3个数字+1个2px间隔
   m_canvas.FontSizeSet(-50);
   width_font=m_canvas.TextWidth("0");  // 字体宽
   width_flow=6*width_font+4;
   width_poc=3*width_font+2;           // poc宽度

   // 确定K线坐标轴
   origin_point.x=canvas_left+canvas_margin;
   origin_point.y=canvas_height-canvas_bottom-canvas_margin;
   canvas_xsize=canvas_width-canvas_left-canvas_right-canvas_margin;
   canvas_ysize=canvas_height-canvas_top-canvas_bottom-canvas_margin*2;

   // 根据K线区宽度确定K线数量(即分钟数)
   tmp_ktotals=int(canvas_xsize/width_flow);
   
   return(true);
}


bool CDrawFlow::RunFlow(void)
{
   // 若图表大小发生变化
   if(canvas_width!=m_chart.WidthInPixels() || canvas_height!=m_chart.HeightInPixels(0))
      Update();

   // 获取对应时间数据
   flow_totime=0;
   // iTime获取的数据总量需要-1
   flow_fromtime=ulong(iTime(m_symbol,PERIOD_M1,tmp_ktotals-1))*1000;
   if(!CFlow::GetTickData(flow_fromtime,flow_totime))
   {
      Print("Tick数据获取失败");
      return(false);
   };
   // 计算数据
   if(!CFlow::Calculate() || tmp_ktotals!=CFlow::x_label.Total())
   {
      Print("ktotals:",tmp_ktotals," label:",CFlow::x_label.Total(),"不相等");
      Print("flow_totime:",iTime(m_symbol,PERIOD_M1,tmp_ktotals-1)," flow_fromtime:",iTime(m_symbol,PERIOD_M1,0));
      return(false);
   }
   //--- 获取完价格后设置坐标系
   CFlow::SetCoordinate(origin_point.x,origin_point.y,canvas_xsize,canvas_ysize);
   
   
   // 清空画板
   m_canvas.Erase(ARGB(255, 175,238,238));
   // 定义矩形绘图区
   m_canvas.Rectangle(canvas_left, canvas_top, canvas_width-canvas_right, canvas_height-canvas_bottom, clrBlack);

   // 逐个绘图OrderFlow
   for(int i=0;i<tmp_ktotals;i++)
   {
      // 计算x坐标(原点+gap), y坐标原点-margin
      tmp_point.x=CFlow::x_step*i+CFlow::c_origin.x;
      tmp_point.y=CFlow::c_origin.y+canvas_margin;
      // 绘制横坐标刻度线
      m_canvas.Line(tmp_point.x,tmp_point.y,tmp_point.x,tmp_point.y-width_scale,clrBlack);
      // 获取分钟数字作为x轴标签
      tmp_xlabel=StringSubstr(CFlow::x_label.At(i),14,2);
      m_canvas.TextOut(tmp_point.x,tmp_point.y,tmp_xlabel,clrBlack,TA_TOP|TA_CENTER);
      
      // 绘制蜡烛图
      // 1. 计算x坐标
      open_point.x=tmp_point.x;high_point.x=tmp_point.x;low_point.x=tmp_point.x;close_point.x=tmp_point.x;
      // 2. 计算y坐标
      tmp_yprice=CFlow::order_data[i].o_open;
      CFlow::CalYCoordinate(tmp_yprice,open_point.y);
      tmp_yprice=CFlow::order_data[i].o_high;
      CFlow::CalYCoordinate(tmp_yprice,high_point.y);
      tmp_yprice=CFlow::order_data[i].o_low;
      CFlow::CalYCoordinate(tmp_yprice,low_point.y);
      tmp_yprice=CFlow::order_data[i].o_close;
      CFlow::CalYCoordinate(tmp_yprice,close_point.y);
      
      // 定义K线颜色
      tmp_kclr=(
               open_point.y>close_point.y?ARGB(255,255,0,0) :
               open_point.y<close_point.y?ARGB(255,0,255,0):
               ARGB(255,0,0,255)
      );
      // 绘制K线:(最高最低价的线被设置为分割线)
      m_canvas.FillRectangle(open_point.x-width_k,open_point.y,close_point.x+width_k,close_point.y,tmp_kclr);
      m_canvas.Line(high_point.x,high_point.y,low_point.x,low_point.y,clrBlack);
      
      // 绘制OrderFlow
      tmp_ksize=CFlow::order_data[i].o_data_total;
      for(uint j=0;j<tmp_ksize;j++)
      {
         tmp_yprice=CFlow::order_data[i].o_price[j];
         CFlow::CalYCoordinate(tmp_yprice,tmp_point.y);
         tmp_blabel=DoubleToString(CFlow::order_data[i].o_buy[j],0);
         tmp_slabel=DoubleToString(CFlow::order_data[i].o_sell[j],0);

         // 主动卖在左, 主动买在右
         m_canvas.TextOut(tmp_point.x-width_k,tmp_point.y,tmp_slabel,clrBlack,TA_RIGHT|TA_VCENTER);
         m_canvas.TextOut(tmp_point.x+width_k,tmp_point.y,tmp_blabel,clrBlack,TA_LEFT|TA_VCENTER);
      }
      // 绘制delta和poc
      {
         // delta在最高价之上1个字符高度
         tmp_dlabel=DoubleToString(CFlow::order_data[i].o_delta,0);
         m_canvas.TextOut(high_point.x,high_point.y-width_font,tmp_dlabel,clrBlack,TA_CENTER|TA_BOTTOM);
         
         // poc用矩形绘制
         tmp_yprice=CFlow::order_data[i].o_poc;
         CFlow::CalYCoordinate(tmp_yprice,tmp_point.y);
         m_canvas.Rectangle(tmp_point.x-width_poc,tmp_point.y+width_font,tmp_point.x+width_poc,tmp_point.y-width_font,ARGB(250,255,0,0));
      }
   }

   //--- 绘制y轴
   for(int i=0;i<CFlow::y_label.Total();i++)
   {
      // 计算x和y坐标
      tmp_point.x=canvas_width-canvas_right;
      tmp_yprice=CFlow::y_label.At(i);
      CFlow::CalYCoordinate(tmp_yprice,tmp_point.y);
      // 绘制纵坐标刻度线
      m_canvas.Line(tmp_point.x-width_scale,tmp_point.y,tmp_point.x,tmp_point.y,clrBlack);
      // y_label离线太近, 因此右移3个坐标
      m_canvas.TextOut(tmp_point.x+3,tmp_point.y,DoubleToString(CFlow::y_label[i],Digits()),clrBlack,TA_LEFT|TA_VCENTER);
   }

//--- 设置透明度
   m_canvas.TransparentLevelSet(255);
   m_canvas.Update();
   
   return(true);
}

bool CDrawFlow::CreateChart()
{
   //--- 使用当前图表
   m_chart.Attach(0);
   //--- 图表样式
   m_chart.Mode(CHART_CANDLES);
   m_chart.BringToTop();
   //--- 图表缩放比例
   m_chart.Scale(5.0);
   //--- 是否展示网格
   m_chart.ShowGrid(true);
   //--- 鼠标拖拽日期可实现缩放
   m_chart.ShowDateScale(false);
   
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
   
   return(true);
}
```