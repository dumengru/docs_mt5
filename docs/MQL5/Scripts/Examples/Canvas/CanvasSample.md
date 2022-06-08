# CanvasSample.mq5

source: `{{ page.path }}`

自定义图形示例

```cpp
#include <Canvas\Canvas.mqh>
//+------------------------------------------------------------------+
//| inputs                                                           |
//+------------------------------------------------------------------+
input int      Width=800;
input int      Height=600;
//+------------------------------------------------------------------+
//| Script program start function                                    |
//+------------------------------------------------------------------+
int OnStart(void)
  {
   int total=1024;
   int limit=MathMax(Width,Height);
   int x1,x2,x3,y1,y2,y3,r;
   int x[],y[];
//--- 检查
   if(Width<100 || Height<100)
     {
      Print("Too simple.");
      return(-1);
     }
//--- 创建图形
   CCanvas canvas;
   if(!canvas.CreateBitmapLabel("SampleCanvas",0,0,Width,Height,COLOR_FORMAT_ARGB_RAW))
     {
      Print("Error creating canvas: ",GetLastError());
      return(-1);
     }
//--- 填充颜色, 更新
   canvas.Erase(XRGB(0x1F,0x1F,0x1F));
   canvas.Update();
//--- 设置随机种子
   srand(GetTickCount());
//--- 绘制像素
   for(int i=0;i<total && !IsStopped();i++)
     {
      x1=rand()%limit;
      y1=rand()%limit;
      canvas.PixelSet(x1,y1,RandomRGB());
      canvas.Update();
     }
//--- 填充颜色, 更新
   canvas.Erase(XRGB(0x1F,0x1F,0x1F));
   canvas.Update();
//--- 绘制水平线/垂直线
   for(int i=0;i<total && !IsStopped();i++)
     {
      x1=rand()%limit;
      x2=rand()%limit;
      y1=rand()%limit;
      y2=rand()%limit;
      if(i%2==0)
         canvas.LineHorizontal(x1,x2,y1,RandomRGB());
      else
         canvas.LineVertical(x1,y1,y2,RandomRGB());
      canvas.Update();
     }
//--- 绘制线
   for(int i=0;i<total && !IsStopped();i++)
     {
      x1=rand()%limit;
      x2=rand()%limit;
      y1=rand()%limit;
      y2=rand()%limit;
      canvas.Line(x1,y1,x2,y2,RandomRGB());
      canvas.Update();
     }
//--- 填充颜色, 更新
   canvas.Erase(XRGB(0x1F,0x1F,0x1F));
   canvas.Update();
//--- 绘制填充圆形
   for(int i=0;i<total && !IsStopped();i++)
     {
      x1=rand()%limit;
      y1=rand()%limit;
      r =rand()%limit;
      canvas.FillCircle(x1,y1,r,RandomRGB());
      canvas.Update();
     }
//--- 绘制圆形
   for(int i=0;i<total && !IsStopped();i++)
     {
      x1=rand()%limit;
      y1=rand()%limit;
      r =rand()%limit;
      canvas.Circle(x1,y1,r,RandomRGB());
      canvas.Update();
     }
//--- 填充颜色, 更新
   canvas.Erase(XRGB(0x1F,0x1F,0x1F));
   canvas.Update();
//--- 绘制填充矩形
   for(int i=0;i<total && !IsStopped();i++)
     {
      x1=rand()%limit;
      y1=rand()%limit;
      x2=rand()%limit;
      y2=rand()%limit;
      canvas.FillRectangle(x1,y1,x2,y2,RandomRGB());
      canvas.Update();
     }
//--- 绘制矩形
   for(int i=0;i<total && !IsStopped();i++)
     {
      x1=rand()%limit;
      y1=rand()%limit;
      x2=rand()%limit;
      y2=rand()%limit;
      canvas.Rectangle(x1,y1,x2,y2,RandomRGB());
      canvas.Update();
     }
//--- 填充颜色, 更新
   canvas.Erase(XRGB(0x1F,0x1F,0x1F));
   canvas.Update();
//--- 绘制填充三角形
   for(int i=0;i<total && !IsStopped();i++)
     {
      x1=rand()%limit;
      y1=rand()%limit;
      x2=rand()%limit;
      y2=rand()%limit;
      x3=rand()%limit;
      y3=rand()%limit;
      canvas.FillTriangle(x1,y1,x2,y2,x3,y3,RandomRGB());
      canvas.Update();
     }
//--- 绘制三角形
   for(int i=0;i<total && !IsStopped();i++)
     {
      x1=rand()%limit;
      y1=rand()%limit;
      x2=rand()%limit;
      y2=rand()%limit;
      x3=rand()%limit;
      y3=rand()%limit;
      canvas.Triangle(x1,y1,x2,y2,x3,y3,RandomRGB());
      canvas.Update();
     }
//--- 填充颜色, 更新
   canvas.Erase(XRGB(0x1F,0x1F,0x1F));
   canvas.Update();
//---
   ArrayResize(x,10);
   ArrayResize(y,10);
//--- 绘制多线
   for(int i=0;i<10;i++)
     {
      x[i]=rand()%Width;
      y[i]=rand()%Height;
     }
   canvas.Polyline(x,y,RandomRGB());
   canvas.Update();
//--- 绘制多边形
   for(int i=0;i<10;i++)
     {
      x[i]=rand()%Width;
      y[i]=rand()%Height;
     }
   canvas.Polygon(x,y,RandomRGB());
   canvas.Update();
//--- filling
   for(int i=0;i<total && !IsStopped();i++)
     {
      int xf=rand()%Width;
      int yf=rand()%Height;
      canvas.Fill(xf,yf,RandomRGB());
      canvas.Update();
     }
//--- 填充颜色, 更新
   canvas.Erase(XRGB(0x1F,0x1F,0x1F));
   canvas.Update();
//--- 绘制文本
   string text;
   x1=Width/2;
   y1=Height/2;
   r =y1-50;
   for(int i=0;i<8;i++)
     {
      double a=i*M_PI_4;
      uint clr=RandomRGB();
      int  deg=(int)(180*a/M_PI);
      x2=x1+(int)(r*cos(a));
      y2=y1-(int)(r*sin(a));
      canvas.LineAA(x1,y1,x2,y2,clr,STYLE_DASHDOTDOT);
      text="Angle "+IntegerToString(deg);
      canvas.FontSet(canvas.FontNameGet(),canvas.FontSizeGet(),canvas.FontFlagsGet(),10*deg);
      canvas.TextOut(x2,y2,text,clr,TA_RIGHT|TA_BOTTOM);
      canvas.Update();
     }
//--- finish
   ObjectDelete(0,"SampleCanvas");
   canvas.Destroy();
   return(0);
  }
//+------------------------------------------------------------------+
//| 随机颜色+------------------------------------------------------------------+
uint RandomRGB(void)
  {
   return(XRGB(rand()%255,rand()%255,rand()%255));
  }
//+------------------------------------------------------------------+
```