# StdLibErr.mqh

source: `{{ page.path }}`

定义一些报错的宏信息:
- 无效句柄
- 无效缓冲区
- 无效节点
- 空数组

```cpp
#define ERR_USER_INVALID_HANDLE                            1
#define ERR_USER_INVALID_BUFF_NUM                          2
#define ERR_USER_ITEM_NOT_FOUND                            3
#define ERR_USER_ARRAY_IS_EMPTY                            1000
```