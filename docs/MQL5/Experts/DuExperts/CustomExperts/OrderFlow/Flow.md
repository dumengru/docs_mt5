# Flow.mqh

source: `{{ page.path }}`

定义OrderFlow基类, 包括数据存储和计算逻辑

```cpp
#include <Arrays\ArrayString.mqh>
#include <Arrays\ArrayDouble.mqh>

//--- 定义坐标点
struct PPoint
  {
   int               x;                   // 坐标: x
   int               y;                   // 坐标: y
  };

//--- 定义OrderFlow数据类
class COrderData
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
   COrderData();
   ~COrderData();
};
COrderData::COrderData(void):o_data_total(0)
{}
COrderData::~COrderData(void)
{}

//--- 重新分配内存
void COrderData::Resize(void)
{
   ArrayResize(o_price,o_data_total);
   ArrayResize(o_buy,o_data_total);
   ArrayResize(o_sell,o_data_total);
   ArrayResize(o_total,o_data_total);
}

//--- 计算delta, poc, high, low
void COrderData::Calculate(void)
{
   for(uint i=0;i<o_data_total;i++)
   {
      o_delta+=int(o_buy[i]-o_sell[i]);
   }
   o_poc=o_price[ArrayMaximum(o_total)];
   o_high=o_price[ArrayMaximum(o_price)];
   o_low=o_price[ArrayMinimum(o_price)];
}

//--- 定义OrderFlow计算类
class CFlow
{
public:
   string            m_symbol;      // 品种
   double            m_adjust;      // 间隔点数
   double            m_multiple;    // 品种乘数(用于计算基准价)
   int               m_data_total;  // 数据量
   MqlTick           m_ticks[];     // 原始Tick数据
   COrderData        *order_data[]; // 标准数据数组

   // 坐标相关
   CArrayString      x_label;       // 存储x轴标签(时间)(不排序数组)
   CArrayDouble      y_label;       // 存储y轴标签(价格)(排序数组)
   //--- x, y轴计算逻辑不同, x轴简单计算, y轴需要比例计算
   int               x_step;        // x轴标签间隔
   int               y_length;      // y轴绘图区长度
   double            y_psize;       // y轴价格长度(最大值价格-最小值)
   PPoint            c_origin;      // 左下角绘图起点坐标
   double            base_price;    // 标准价格
   
   // 临时变量
   COrderData        *tmp_order;    // 数据指针
   MqlDateTime       temp_dt;       // 时间结构体 

public:
   //--- 构造函数输入品种名称和间隔点数
   CFlow();
   ~CFlow();

   bool        Init(const string &symbol,const int &adjust,bool is_custom=false);        // 填充数据
   bool        GetTickData(const datetime &from_time,const datetime to_time=0);
   bool        Calculate();
   template<typename T>
   int         ArraySearch(const T &arr[], T &value);
   bool        SetCoordinate(const int &x,const int &y,const int &x_size,const int &y_size);  // 构造坐标系
   bool        CalCoordinate(const datetime &x,const double &y,PPoint &cp);      // 计算y坐标
   bool        CalYCoordinate(const double &y,int &cy);
   bool        Clear();       // 清空动态数组
   bool        InitialData();    // 初始化数据

};

CFlow::CFlow():m_symbol(""),
               m_data_total(0),
               x_step(0),
               y_length(0),
               y_psize(0),
               base_price(0),
               m_multiple(0)
               
{
//--- 初始化左下角起始坐标
   c_origin.x=0;
   c_origin.y=0;
}

CFlow::~CFlow(void)
{
   if(!Clear())
      Alert("order_data 未正常释放");
}

bool CFlow::Clear(void)
{
   x_label.Clear();
   y_label.Clear();

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

//--- 设置品种, 计算点值
bool CFlow::Init(const string &symbol,const int &adjust,bool is_custom=false)
{
   if(!SymbolExist(symbol,is_custom))
   {
      Print("交易品种不存在");
      return(false);
   }
   m_symbol=symbol;
   // 利用点数调整点值
   m_adjust=Point()*adjust;
   // 计算品种乘数
   m_multiple=MathPow(10,Digits());

   return(true);
}

//--- 获取Tick数据
bool CFlow::GetTickData(const datetime &from_time,const datetime to_time=0)
{
   bool res=CopyTicksRange(m_symbol,m_ticks,COPY_TICKS_TRADE,from_time,to_time)>0? true:false;
   return(res);
}

//--- 计算数据
bool CFlow::Calculate()
{
   // 每次重新计算都需要清空数据
   if(!Clear())
   {
      Print("旧数据未清空");
      return(false);
   }

   int ticks_size=ArraySize(m_ticks);
   if(ticks_size<3)
      return(false);

   datetime tmp_time=0;
   for(int i=0;i<ticks_size;i++)
   {  
//--- flags统计: buy: 312,56   sell: 344,88   buy/sell:120,376
      if(m_ticks[i].flags==120 || m_ticks[i].flags==376)
      {
         // Print("过滤flags: 120");
         continue;
      }
      
      // 以分钟为单位, 将S设为0
      TimeToStruct(m_ticks[i].time,temp_dt);
      temp_dt.sec=0;
      m_ticks[i].time=StructToTime(temp_dt);

      if(tmp_time!=m_ticks[i].time)
      {
         // 出现新的一分钟
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
         order_data[m_data_total-1]=new COrderData;
         
         // 填充时间和开盘价
         order_data[m_data_total-1].o_time=m_ticks[i].time;
         order_data[m_data_total-1].o_open=m_ticks[i].last;
         // 添加x_label
         x_label.Add(TimeToString(m_ticks[i].time));
         
         // 更新标记时间
         tmp_time=m_ticks[i].time;
      }
      // 获取数据指针
      tmp_order=order_data[m_data_total-1];
      
      // 计算标准价格
      base_price=NormalizeDouble(m_ticks[i].last-double(int(m_ticks[i].last*m_multiple)%int(m_adjust*m_multiple))/m_multiple,Digits());
      
      // 将价格添加到y_label
      y_label.Sort();
      if(y_label.Search(base_price)<0)
         y_label.Add(base_price);
      
      // 搜索标准价格是否已保存
      int p_index=ArraySearch(tmp_order.o_price,base_price);
      if(p_index<0)
      {
         // 没找到, 说明是新价格
         // 增加OrderData数组空间
         tmp_order.o_data_total+=1;
         tmp_order.Resize();
         // 填充价格和量, 一定要用基准价填充
         tmp_order.o_price[tmp_order.o_data_total-1]=base_price;
         tmp_order.o_total[tmp_order.o_data_total-1]=m_ticks[i].volume;
         if(m_ticks[i].flags==312 || m_ticks[i].flags==56)
         {
            // 主动买更新, 主动卖为0
            tmp_order.o_buy[tmp_order.o_data_total-1]=m_ticks[i].volume;
            tmp_order.o_sell[tmp_order.o_data_total-1]=0;
         }
         else if(m_ticks[i].flags==344 || m_ticks[i].flags==88)
         {
            // 主动卖更新, 主动买为0
            tmp_order.o_buy[tmp_order.o_data_total-1]=0;
            tmp_order.o_sell[tmp_order.o_data_total-1]=m_ticks[i].volume;
         }
         else
         {
            ArrayPrint(m_ticks);
            Print("新成交量flag超出预期: ",m_ticks[i].flags," time:",m_ticks[i].time, " msc: ",m_ticks[i].time_msc);
         }
      }
      else
      {
         // 找到了, 说明价格已存在
         if(m_ticks[i].flags==312 || m_ticks[i].flags==56)
         {
            // 主动买更新, 总成交更新
            tmp_order.o_total[p_index]+=m_ticks[i].volume;
            tmp_order.o_buy[p_index]+=m_ticks[i].volume;
         }
         else if(m_ticks[i].flags==344 || m_ticks[i].flags==88)
         {
            // 主动卖更新, 总成交更新
            tmp_order.o_total[p_index]+=m_ticks[i].volume;
            tmp_order.o_sell[p_index]+=m_ticks[i].volume;
         }
         else
         {
            ArrayPrint(m_ticks);
            Print("旧成交量flag超出预期: ",m_ticks[i].flags," time:",m_ticks[i].time, " msc: ",m_ticks[i].time_msc);
         }
      }
   }

   // 数据遍历完毕, 计算最后一分钟的delta和poc, 记录收盘价
   order_data[m_data_total-1].Calculate();
   order_data[m_data_total-1].o_close=m_ticks[ticks_size-1].last;
   
   return(true);
}

//--- 输入左下角坐标(x, y)和绘图区宽高, 计算绘图相关数据
bool CFlow::SetCoordinate(const int &x,const int &y,const int &x_size,const int &y_size)
{
   if(x_label.Total()<3 || y_label.Total()<3)
   {
      Print("x|y轴数据量太少");
      return(false);
   }
   
   // 计算x轴标签距离
   x_step=int((x_size-x_size%x_label.Total())/x_label.Total());
   // 计算y轴价格差
   y_psize=y_label.At(y_label.Total()-1)-y_label.At(0);
   
   y_length=y_size;
   c_origin.x=x;
   c_origin.y=y;
   
   return(true);
}

//--- 输入时间和价格, 输出对应坐标
bool CFlow::CalCoordinate(const datetime &x,const double &y,PPoint &cp)
{
   if(x_step==0)
   {
      Print("坐标系未初始化");
      return(false);
   }
   
   // x_label是未排序数组, 只能线性搜索
   int x_index=x_label.SearchLinear(TimeToString(x));
   if(x_index<0)
   {
      Print("未找到x坐标");
      return(false);
   }
   else
   {
      // 计算出对应X坐标
      cp.x=c_origin.x+x_index*x_step;
   }

   // 计算y坐标(由于最高价和最低价啊不标准,y坐标可能超过最大值)
   cp.y=c_origin.y-int((y-y_label.At(0))/y_psize*y_length);
   
   return(true);
}

//--- 输入价格计算y坐标
bool CFlow::CalYCoordinate(const double &y,int &cy)
{
   // 计算y坐标
   cy=c_origin.y-int((y-y_label.At(0))/y_psize*y_length);
   return(true);
}

//--- 创建数组搜索模板, 找到返回索引, 没找到返回-1
template<typename T>
int CFlow::ArraySearch(const T &arr[], T &value)
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
```