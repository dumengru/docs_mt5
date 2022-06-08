# ExpertTrailing.mqh

source: `{{ page.path }}`

定义尾随算法基准类

只有接口

```cpp
#include "ExpertBase.mqh"
//+------------------------------------------------------------------+
//| Class CExpertTrailing.                                           |
//| Purpose: Base class traling stops.                               |
//| Derives from class CExpertBase.                                  |
//+------------------------------------------------------------------+
class CExpertTrailing : public CExpertBase
  {
public:
                     CExpertTrailing(void);
                    ~CExpertTrailing(void);
   //---
   virtual bool      CheckTrailingStopLong(CPositionInfo *position,double &sl,double &tp)  { return(false); }
   virtual bool      CheckTrailingStopShort(CPositionInfo *position,double &sl,double &tp) { return(false); }
  };
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
CExpertTrailing::CExpertTrailing(void)
  {
  }
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
CExpertTrailing::~CExpertTrailing(void)
  {
  }
```