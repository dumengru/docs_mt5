# CalendarInfo.mq5

source: `{{ page.path }}`

## 获取经济日历数据


```cpp
#include <Files\FileTxt.mqh>


input int filter_imp=0;                            // 重要性小于2的事件会被过滤
input datetime datetime_from=D'2010.01.01 00:00';   // 事件起始时间


// 将要保留的事件内容结构体
struct CalendarInfo
{
   string   country_name;        // 国家名称
   ulong    country_id;          // 国家id
   string   event_name;          // 事件名称
   ulong    event_id;            // 事件id
   int      event_imp;           // 事件重要性
   datetime event_time;          // 事件发生时间
};


void OnStart() 
  { 
   CalendarInfo   cal_infos[];
   int c_total=1000;    // 数组大小
   int c_count=0;       // 计数器
   ArrayResize(cal_infos,c_total);

//--- 从经济日历中获得国家列表 
   MqlCalendarCountry countries[];
   MqlCalendarEvent events[];
   MqlCalendarValue values[];

   // 获取数组大小
   int      c_size;           // 国家长度
   int      e_size;           // 事件长度
   int      v_size;           // 值长度
   
   // 遍历国家
   int count=CalendarCountries(countries); 
   c_size=ArraySize(countries);
   for(int i=0; i<c_size; i++)
   {
      // 遍历事件
      ArrayFree(events);
      CalendarEventByCountry(countries[i].code,events);
      e_size=ArraySize(events);
      for(int j=0; j<e_size; j++)
      {
         // 过滤时间不固定和重要性低的事件
         if(events[j].time_mode!=0 || int(events[j].importance)<filter_imp)
            continue;

         // 遍历值
         ArrayFree(values);
         CalendarValueHistoryByEvent(events[j].id,values,datetime_from);
         v_size=ArraySize(values);
         for(int k=0; k<v_size; k++)
         {
                  
            // 填充国家名称
            cal_infos[c_count].country_name=countries[i].name;
            // 填充国家id
            cal_infos[c_count].country_id=events[j].country_id;
            cal_infos[c_count].event_name=events[j].name;
            cal_infos[c_count].event_id=events[j].id;
            cal_infos[c_count].event_imp=int(events[j].importance);
            // 填充日期
            cal_infos[c_count].event_time=values[k].time;
            c_count+=1;
            
            if(c_total-c_count<100)
            {
               c_total+=100;
               ArrayResize(cal_infos,c_total);
            }
         }
      }
   }
   
   // 清楚多余数据
   ArrayResize(cal_infos,c_count);
   printf("共有数据 %d 条", c_count);
   
   // 保存数据
   save_data(cal_infos);
   
}
//+------------------------------------------------------------------+
// 保存数据
void save_data(CalendarInfo &cal[])
{
   string file_name="calendar_data.csv";
   CFileTxt ct;
   ct.Open(file_name,FILE_CSV|FILE_WRITE);
   // 先写标题
   ct.WriteString("国家名称,国家ID,事件名称,事件ID,重要性,时间\n");
   
   // 再填充内容
   int c_size=ArraySize(cal);
   for(int i=0;i<c_size;i++)
   {
      ct.WriteString(cal[i].country_name);
      ct.WriteString(",");
      ct.WriteString(IntegerToString(cal[i].country_id));
      ct.WriteString(",");
      ct.WriteString(cal[i].event_name);
      ct.WriteString(",");
      ct.WriteString(IntegerToString(cal[i].event_id));
      ct.WriteString(",");
      ct.WriteString(IntegerToString(cal[i].event_imp));
      ct.WriteString(",");
      ct.WriteString(TimeToString(cal[i].event_time));
      ct.WriteString("\n");
   }
   ct.Close();
}
```