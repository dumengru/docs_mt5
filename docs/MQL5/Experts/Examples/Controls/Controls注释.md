# Controls注释

source: `{{ page.path }}`

面板和对话框

## 主程序

```cpp
//--- 导入自定义面板类
#include "ControlsDialog.mqh"
//--- 创建面板对象
CControlsDialog ExtDialog;

int OnInit()
  {
//--- 创建面板
   if(!ExtDialog.Create(0,"Controls",0,20,20,360,324))
     return(INIT_FAILED);
//--- 运行对象
   ExtDialog.Run();
//--- succeed
   return(INIT_SUCCEEDED);
  }
//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
  {
//--- 删除对象
   ExtDialog.Destroy(reason);
  }
//+------------------------------------------------------------------+
//| Expert chart event function                                      |
//+------------------------------------------------------------------+
void OnChartEvent(const int id,         // event ID  
                  const long& lparam,   // event parameter of the long type
                  const double& dparam, // event parameter of the double type
                  const string& sparam) // event parameter of the string type
  {
   //--- 自定义图形事件
   ExtDialog.ChartEvent(id,lparam,dparam,sparam);
  }
```

## 导入文件

```cpp
#include <Controls\Dialog.mqh>          // 对话框
#include <Controls\Button.mqh>          // 按钮
#include <Controls\Edit.mqh>            // 编辑框
#include <Controls\DatePicker.mqh>      // 日历对象
#include <Controls\ListView.mqh>        // 列表
#include <Controls\ComboBox.mqh>        // 下拉框
#include <Controls\SpinEdit.mqh>        // 旋转编辑框(每次增加固定步长值)
#include <Controls\RadioGroup.mqh>      // 单选组
#include <Controls\CheckGroup.mqh>      // 多选组
```

## 声明常量

```cpp
//--- 缩进和间隔
#define INDENT_LEFT                         (11)      // 左边距
#define INDENT_TOP                          (11)      // 上边距
#define INDENT_RIGHT                        (11)      // 右边距
#define INDENT_BOTTOM                       (11)      // 下边距
#define CONTROLS_GAP_X                      (5)       // 横向间隔
#define CONTROLS_GAP_Y                      (5)       // 纵向间隔
//--- for buttons
#define BUTTON_WIDTH                        (100)     // 按钮宽度
#define BUTTON_HEIGHT                       (20)      // 按钮高度
//--- for the indication area
#define EDIT_HEIGHT                         (20)      // 编辑框高度
//--- for group controls
#define GROUP_WIDTH                         (150)     // 选择框宽度
#define LIST_HEIGHT                         (179)     // 列表高度
#define RADIO_HEIGHT                        (56)      // 单选组高度
#define CHECK_HEIGHT                        (93)      // 多选组高度
```

## CControlsDialog

```cpp
//--- 继承自CAppDialog
class CControlsDialog : public CAppDialog
  {
private:
   CEdit             m_edit;                          // 编辑框
   CButton           m_button1;                       // 按钮1
   CButton           m_button2;                       // 按钮2
   CButton           m_button3;                       // 按钮3(固定按钮)
   CSpinEdit         m_spin_edit;                     // 旋转编辑框
   CDatePicker       m_date;                          // 日历对象
   CListView         m_list_view;                     // 列表对象
   CComboBox         m_combo_box;                     // 下拉框
   CRadioGroup       m_radio_group;                   // 单选组
   CCheckGroup       m_check_group;                   // 多选组

public:
                     CControlsDialog(void);
                    ~CControlsDialog(void);
   //--- 创建面板
   virtual bool      Create(const long chart,const string name,const int subwin,const int x1,const int y1,const int x2,const int y2);
   //--- chart event handler
   virtual bool      OnEvent(const int id,const long &lparam,const double &dparam,const string &sparam);

protected:
   //--- 创建对象
   bool              CreateEdit(void);
   bool              CreateButton1(void);
   bool              CreateButton2(void);
   bool              CreateButton3(void);
   bool              CreateSpinEdit(void);
   bool              CreateDate(void);
   bool              CreateListView(void);
   bool              CreateComboBox(void);
   bool              CreateRadioGroup(void);
   bool              CreateCheckGroup(void);
   //--- 事件处理程序
   void              OnClickButton1(void);
   void              OnClickButton2(void);
   void              OnClickButton3(void);
   void              OnChangeSpinEdit(void);
   void              OnChangeDate(void);
   void              OnChangeListView(void);
   void              OnChangeComboBox(void);
   void              OnChangeRadioGroup(void);
   void              OnChangeCheckGroup(void);
  };
```

## 事件映射

将CControlsDialog事件映射到CAppDialog中

```cpp
EVENT_MAP_BEGIN(CControlsDialog)
   ON_EVENT(ON_CLICK,m_button1,OnClickButton1)
   ON_EVENT(ON_CLICK,m_button2,OnClickButton2)
   ON_EVENT(ON_CLICK,m_button3,OnClickButton3)
   ON_EVENT(ON_CHANGE,m_spin_edit,OnChangeSpinEdit)
   ON_EVENT(ON_CHANGE,m_date,OnChangeDate)
   ON_EVENT(ON_CHANGE,m_list_view,OnChangeListView)
   ON_EVENT(ON_CHANGE,m_combo_box,OnChangeComboBox)
   ON_EVENT(ON_CHANGE,m_radio_group,OnChangeRadioGroup)
   ON_EVENT(ON_CHANGE,m_check_group,OnChangeCheckGroup)
EVENT_MAP_END(CAppDialog)
```

## 创建面板

```cpp
bool CControlsDialog::Create(const long chart,const string name,const int subwin,const int x1,const int y1,const int x2,const int y2)
  {
//--- 先创建一个AppDialog
   if(!CAppDialog::Create(chart,name,subwin,x1,y1,x2,y2))
      return(false);
//--- create dependent controls
   if(!CreateEdit())
      return(false);
   if(!CreateButton1())
      return(false);
   if(!CreateButton2())
      return(false);
   if(!CreateButton3())
      return(false);
   if(!CreateSpinEdit())
      return(false);
   if(!CreateListView())
      return(false);
   if(!CreateDate())
      return(false);
   if(!CreateRadioGroup())
      return(false);
   if(!CreateCheckGroup())
      return(false);
   if(!CreateComboBox())
      return(false);
//--- succeed
   return(true);
  }
```

## 创建对象

```cpp
//--- 创建编辑框
bool CControlsDialog::CreateEdit(void)
  {
//--- 坐标
   int x1=INDENT_LEFT;
   int y1=INDENT_TOP;
   int x2=ClientAreaWidth()-INDENT_RIGHT;
   int y2=y1+EDIT_HEIGHT;
//--- 创建
   if(!m_edit.Create(m_chart_id,m_name+"Edit",m_subwin,x1,y1,x2,y2))
      return(false);
//--- 只读
   if(!m_edit.ReadOnly(true))
      return(false);
//--- 添加到app
   if(!Add(m_edit))
      return(false);
//--- succeed
   return(true);
  }

//--- 创建按钮1
bool CControlsDialog::CreateButton1(void)
  {
//--- 坐标
   int x1=INDENT_LEFT;
   int y1=INDENT_TOP+(EDIT_HEIGHT+CONTROLS_GAP_Y);
   int x2=x1+BUTTON_WIDTH;
   int y2=y1+BUTTON_HEIGHT;
//--- 创建
   if(!m_button1.Create(m_chart_id,m_name+"Button1",m_subwin,x1,y1,x2,y2))
      return(false);
//--- 内容
   if(!m_button1.Text("Button1"))
      return(false);
//--- 添加到app
   if(!Add(m_button1))
      return(false);
//--- succeed
   return(true);
  }

//--- 创建按钮2
bool CControlsDialog::CreateButton2(void)
  {
//--- 坐标
   int x1=INDENT_LEFT+(BUTTON_WIDTH+CONTROLS_GAP_X);
   int y1=INDENT_TOP+(EDIT_HEIGHT+CONTROLS_GAP_Y);
   int x2=x1+BUTTON_WIDTH;
   int y2=y1+BUTTON_HEIGHT;
//--- 创建
   if(!m_button2.Create(m_chart_id,m_name+"Button2",m_subwin,x1,y1,x2,y2))
      return(false);
//--- 内容
   if(!m_button2.Text("Button2"))
      return(false);
//--- 添加到app
   if(!Add(m_button2))
      return(false);
//--- succeed
   return(true);
  }

//--- 创建按钮3
bool CControlsDialog::CreateButton3(void)
  {
//--- 坐标
   int x1=INDENT_LEFT+2*(BUTTON_WIDTH+CONTROLS_GAP_X);
   int y1=INDENT_TOP+(EDIT_HEIGHT+CONTROLS_GAP_Y);
   int x2=x1+BUTTON_WIDTH;
   int y2=y1+BUTTON_HEIGHT;
//--- 创建
   if(!m_button3.Create(m_chart_id,m_name+"Button3",m_subwin,x1,y1,x2,y2))
      return(false);
//--- 内容
   if(!m_button3.Text("Locked"))
      return(false);
//--- 添加到app
   if(!Add(m_button3))
      return(false);
//--- 锁定
   m_button3.Locking(true);
//--- succeed
   return(true);
  }

//--- 创建旋转编辑框
bool CControlsDialog::CreateSpinEdit(void)
  {
//--- 坐标
   int x1=INDENT_LEFT;
   int y1=INDENT_TOP+(EDIT_HEIGHT+CONTROLS_GAP_Y)+(BUTTON_HEIGHT+CONTROLS_GAP_Y);
   int x2=x1+GROUP_WIDTH;
   int y2=y1+EDIT_HEIGHT;
//--- 创建
   if(!m_spin_edit.Create(m_chart_id,m_name+"SpinEdit",m_subwin,x1,y1,x2,y2))
      return(false);
//--- 添加到app
   if(!Add(m_spin_edit))
      return(false);
//--- 设置最小值,最大值,默认值
   m_spin_edit.MinValue(10);
   m_spin_edit.MaxValue(1000);
   m_spin_edit.Value(100);
//--- succeed
   return(true);
  }

//--- 创建日历
bool CControlsDialog::CreateDate(void)
  {
//--- 坐标
   int x1=INDENT_LEFT+GROUP_WIDTH+2*CONTROLS_GAP_X;
   int y1=INDENT_TOP+(EDIT_HEIGHT+CONTROLS_GAP_Y)+(BUTTON_HEIGHT+CONTROLS_GAP_Y);
   int x2=x1+GROUP_WIDTH;
   int y2=y1+EDIT_HEIGHT;
//--- 创建
   if(!m_date.Create(m_chart_id,m_name+"Date",m_subwin,x1,y1,x2,y2))
      return(false);
//--- 添加到app
   if(!Add(m_date))
      return(false);
//--- 设置时间
   m_date.Value(TimeCurrent());
//--- succeed
   return(true);
  }

//--- 创建列表
bool CControlsDialog::CreateListView(void)
  {
//--- 坐标
   int x1=INDENT_LEFT+GROUP_WIDTH+2*CONTROLS_GAP_X;
   int y1=INDENT_TOP+(EDIT_HEIGHT+CONTROLS_GAP_Y)+
          (BUTTON_HEIGHT+CONTROLS_GAP_Y)+
          (EDIT_HEIGHT+2*CONTROLS_GAP_Y);
   int x2=x1+GROUP_WIDTH;
   int y2=y1+LIST_HEIGHT-CONTROLS_GAP_Y;
//--- 创建
   if(!m_list_view.Create(m_chart_id,m_name+"ListView",m_subwin,x1,y1,x2,y2))
      return(false);
//--- 添加到app
   if(!Add(m_list_view))
      return(false);
//--- 列表添加项目
   for(int i=0;i<16;i++)
      if(!m_list_view.AddItem("Item "+IntegerToString(i)))
         return(false);
//--- succeed
   return(true);
  }

//--- 创建下拉框
bool CControlsDialog::CreateComboBox(void)
  {
//--- 坐标
   int x1=INDENT_LEFT;
   int y1=INDENT_TOP+(EDIT_HEIGHT+CONTROLS_GAP_Y)+
          (BUTTON_HEIGHT+CONTROLS_GAP_Y)+
          (EDIT_HEIGHT+CONTROLS_GAP_Y);
   int x2=x1+GROUP_WIDTH;
   int y2=y1+EDIT_HEIGHT;
//--- 创建
   if(!m_combo_box.Create(m_chart_id,m_name+"ComboBox",m_subwin,x1,y1,x2,y2))
      return(false);
//--- 添加到app
   if(!Add(m_combo_box))
      return(false);
//--- 下拉框添加项目
   for(int i=0;i<16;i++)
      if(!m_combo_box.ItemAdd("Item "+IntegerToString(i)))
         return(false);
//--- succeed
   return(true);
  }

//--- 创建单选组
bool CControlsDialog::CreateRadioGroup(void)
  {
//--- 坐标
   int x1=INDENT_LEFT;
   int y1=INDENT_TOP+(EDIT_HEIGHT+CONTROLS_GAP_Y)+
          (BUTTON_HEIGHT+CONTROLS_GAP_Y)+
          (EDIT_HEIGHT+CONTROLS_GAP_Y)+
          (EDIT_HEIGHT+CONTROLS_GAP_Y);
   int x2=x1+GROUP_WIDTH;
   int y2=y1+RADIO_HEIGHT;
//--- 创建
   if(!m_radio_group.Create(m_chart_id,m_name+"RadioGroup",m_subwin,x1,y1,x2,y2))
      return(false);
//--- 添加到app
   if(!Add(m_radio_group))
      return(false);
//--- 单选组添加项目
   for(int i=0;i<3;i++)
      if(!m_radio_group.AddItem("Item "+IntegerToString(i),1<<i))
         return(false);
//--- succeed
   return(true);
  }

//--- 创建复选组
bool CControlsDialog::CreateCheckGroup(void)
  {
//--- 坐标
   int x1=INDENT_LEFT;
   int y1=INDENT_TOP+(EDIT_HEIGHT+CONTROLS_GAP_Y)+
          (BUTTON_HEIGHT+CONTROLS_GAP_Y)+
          (EDIT_HEIGHT+CONTROLS_GAP_Y)+
          (EDIT_HEIGHT+CONTROLS_GAP_Y)+
          (RADIO_HEIGHT+CONTROLS_GAP_Y);
   int x2=x1+GROUP_WIDTH;
   int y2=y1+CHECK_HEIGHT;
//--- 创建
   if(!m_check_group.Create(m_chart_id,m_name+"CheckGroup",m_subwin,x1,y1,x2,y2))
      return(false);
//--- 添加到app
   if(!Add(m_check_group))
      return(false);
//--- 复选组添加项目
   for(int i=0;i<5;i++)
      if(!m_check_group.AddItem("Item "+IntegerToString(i),1<<i))
         return(false);
//--- succeed
   return(true);
  }
```

## 触发事件

```cpp
//--- 单击按钮1
void CControlsDialog::OnClickButton1(void)
  {
   m_edit.Text(__FUNCTION__);
  }
//--- 单击按钮2
void CControlsDialog::OnClickButton2(void)
  {
   m_edit.Text(__FUNCTION__);
  }
//--- 单击按钮3
void CControlsDialog::OnClickButton3(void)
  {
   if(m_button3.Pressed())
      m_edit.Text(__FUNCTION__+"On");
   else
      m_edit.Text(__FUNCTION__+"Off");
  }
//--- 改变旋转编辑框
void CControlsDialog::OnChangeSpinEdit()
  {
   m_edit.Text(__FUNCTION__+" : Value="+IntegerToString(m_spin_edit.Value()));
  }
//--- 改变日历
void CControlsDialog::OnChangeDate(void)
  {
   m_edit.Text(__FUNCTION__+" \""+TimeToString(m_date.Value(),TIME_DATE)+"\"");
  }
//--- 改变列表
void CControlsDialog::OnChangeListView(void)
  {
   m_edit.Text(__FUNCTION__+" \""+m_list_view.Select()+"\"");
  }
//--- 改变下拉框
void CControlsDialog::OnChangeComboBox(void)
  {
   m_edit.Text(__FUNCTION__+" \""+m_combo_box.Select()+"\"");
  }
//--- 改变单选组
void CControlsDialog::OnChangeRadioGroup(void)
  {
   m_edit.Text(__FUNCTION__+" : Value="+IntegerToString(m_radio_group.Value()));
  }
//--- 改变复选组
void CControlsDialog::OnChangeCheckGroup(void)
  {
   m_edit.Text(__FUNCTION__+" : Value="+IntegerToString(m_check_group.Value()));
  }
```