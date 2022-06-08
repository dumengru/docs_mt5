# FileTxt.mqh

source: `{{ page.path }}`

处理文本文件

```cpp
#include "File.mqh"
//+------------------------------------------------------------------+
//| Class CFileTxt                                                   |
//| Purpose: Class of operations with text files.                    |
//|          Derives from class CFile.                               |
//+------------------------------------------------------------------+
class CFileTxt : public CFile
  {
public:
                     CFileTxt(void);
                    ~CFileTxt(void);
   //--- 打开文件
   int               Open(const string file_name,const int open_flags);
   //--- 写/读字符串
   uint              WriteString(const string value);
   string            ReadString(void);
  };
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
CFileTxt::CFileTxt(void)
  {
  }
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
CFileTxt::~CFileTxt(void)
  {
  }
//+------------------------------------------------------------------+
//| Open the text file                                               |
//+------------------------------------------------------------------+
int CFileTxt::Open(const string file_name,const int open_flags)
  {
   return(CFile::Open(file_name,open_flags|FILE_TXT));
  }
//+------------------------------------------------------------------+
//| Writing string to file                                           |
//+------------------------------------------------------------------+
uint CFileTxt::WriteString(const string value)
  {
//--- check handle
   if(m_handle!=INVALID_HANDLE)
      return(FileWriteString(m_handle,value));
//--- failure
   return(0);
  }
//+------------------------------------------------------------------+
//| Reading string from file                                         |
//+------------------------------------------------------------------+
string CFileTxt::ReadString(void)
  {
//--- check handle
   if(m_handle!=INVALID_HANDLE)
      return(FileReadString(m_handle));
//--- failure
   return("");
  }
//+------------------------------------------------------------------+
```