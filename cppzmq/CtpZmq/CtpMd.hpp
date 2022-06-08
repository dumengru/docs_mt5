#include "Utility.hpp"


#define MDADDRPUB "tcp://*:5556"     // 行情订阅地址
#define MDADDRREP "tcp://*:5557"     // 行情请求地址


class CCtpMd :CThostFtdcMdSpi
{
public:
	CThostFtdcMdApi* md_api;
	// ctp相关
	const char*		md_host;		// 行情地址
	// zmq相关
	void*			md_context;
	void*			md_socket_pub;
	void*			md_socket_rep;
	char			md_send_pub[2048];
	char			md_send_rep[128];
	char			md_recv_rep[1024];
	CRdJson*		md_json_pub;
	CRdJson*		md_json_rep;

public:
	CCtpMd(const char * host):
		md_host(host)
	{
		md_api = CThostFtdcMdApi::CreateFtdcMdApi();
		md_api->RegisterSpi(this);

		// 新建socket
		md_context = zmq_ctx_new();
		md_socket_pub = zmq_socket(md_context, ZMQ_PUB);
		md_socket_rep = zmq_socket(md_context, ZMQ_REP);

		memset(&md_send_rep, 0x00, sizeof(md_send_rep));
		memset(&md_recv_rep, 0x00, sizeof(md_recv_rep));
		memset(&md_send_pub, 0x00, sizeof(md_send_pub));

		md_json_pub = new CRdJson();
		md_json_rep = new CRdJson();
	}
	~CCtpMd() 
	{
		md_api->Release();
		// 解绑 -> 断开 -> 关闭
		zmq_unbind(md_socket_pub, MDADDRPUB);
		zmq_unbind(md_socket_rep, MDADDRREP);
		zmq_disconnect(md_socket_pub, MDADDRPUB);
		zmq_disconnect(md_socket_rep, MDADDRREP);
		zmq_close(md_socket_pub);
		zmq_close(md_socket_rep);
		zmq_ctx_shutdown(md_context);
		zmq_ctx_term(md_context);

		delete md_json_pub;
		delete md_json_rep;
	}

	void OnFrontConnected()
	{
		CThostFtdcReqUserLoginField LoginField{ 0 };
		md_api->ReqUserLogin(&LoginField, 1);
	}

	//--- 登录
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
	{
		if (pRspInfo && pRspInfo->ErrorID != 0)
		{
			printf("Login failed. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
		printf("版本号:%s, 交易日:%s\n",md_api->GetApiVersion(), md_api->GetTradingDay());
		// 发出信号
		_semaphore.signal();
	}

	//--- 启动
	bool MdBind()
	{
		// 绑定socket
		zmq_bind(md_socket_pub, MDADDRPUB);
		zmq_bind(md_socket_rep, MDADDRREP);

		md_api->RegisterFront((char*)md_host);
		md_api->Init();
		return(true);
	}

	bool MdResponse()
	{
		strcpy_s(md_send_pub, "防阻塞测试:pub/sub");
		zmq_send(md_socket_pub, md_send_pub, sizeof(md_send_pub), 0);

		while (true)
		{
			_semaphore.wait();
			// 接收指令
			zmq_recv(md_socket_rep, md_recv_rep, sizeof(md_recv_rep), 0);
			printf("recv:%s", md_recv_rep);
			strcpy_s(md_send_rep, "yes");
			zmq_send(md_socket_rep, md_send_rep, sizeof(md_send_rep), 0);

			// 解析json数据
			md_json_rep->m_doc.Parse(md_recv_rep);
			std::string func = md_json_rep->m_doc["func"].GetString();
			const char* params = md_json_rep->m_doc["params"].GetString();

			int req_result = 
				func == "SubscribeMarketData" ?	SubscribeMarketData(params) :
				printf("不存在此函数");

			switch (req_result)
			{
			case(1):
				printf("网络连接失败\n"); break;
			case(2):
				printf("处理请求超过许可数\n"); break;
			case(3):
				printf("每秒发送请求超过许可数\n"); break;
			default:
				printf("CTP请求成功\n");
				break;
			}
		}
		return(true);
	}

	bool SubscribeMarketData(const char *params)
	{
		// 接收订阅请求
		std::string symbols=params;
		char* instruments[100]{ 0 };
		std::vector<std::string> result = split(symbols, ";");
		for (int i = 0; i < result.size(); i++)
		{
			instruments[i] = (char*)result[i].c_str();
			printf("订阅品种:%s \n", result[i].c_str());
		}
		md_api->SubscribeMarketData(instruments, result.size());

		return(true);
	}

	//--- 接收行情
	void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
	{
		md_json_pub->add_member("交易日", pDepthMarketData->TradingDay);
		md_json_pub->add_member("合约代码", pDepthMarketData->InstrumentID);
		md_json_pub->add_member("交易所代码", pDepthMarketData->ExchangeID);
		md_json_pub->add_member("合约在交易所的代码", pDepthMarketData->ExchangeInstID);
		md_json_pub->add_member("最新价", pDepthMarketData->LastPrice);
		md_json_pub->add_member("上次结算价", pDepthMarketData->PreSettlementPrice);
		md_json_pub->add_member("昨收盘", pDepthMarketData->PreClosePrice);
		md_json_pub->add_member("昨持仓量", pDepthMarketData->PreOpenInterest);
		md_json_pub->add_member("今开盘", pDepthMarketData->OpenPrice);
		md_json_pub->add_member("最高价", pDepthMarketData->HighestPrice);
		md_json_pub->add_member("最低价", pDepthMarketData->LowestPrice);
		md_json_pub->add_member("数量", pDepthMarketData->Volume);
		md_json_pub->add_member("成交金额", pDepthMarketData->Turnover);
		md_json_pub->add_member("持仓量", pDepthMarketData->OpenInterest);
		md_json_pub->add_member("今收盘", pDepthMarketData->ClosePrice);
		md_json_pub->add_member("本次结算价", pDepthMarketData->SettlementPrice);
		md_json_pub->add_member("涨停板价", pDepthMarketData->UpperLimitPrice);
		md_json_pub->add_member("跌停板价", pDepthMarketData->LowerLimitPrice);
		md_json_pub->add_member("最后修改时间", pDepthMarketData->UpdateTime);
		md_json_pub->add_member("最后修改毫秒", pDepthMarketData->UpdateMillisec);
		md_json_pub->add_member("申买价一", pDepthMarketData->BidPrice1);
		md_json_pub->add_member("申买量一", pDepthMarketData->BidVolume1);
		md_json_pub->add_member("申卖价一", pDepthMarketData->AskPrice1);
		md_json_pub->add_member("申卖量一", pDepthMarketData->AskVolume1);
		md_json_pub->add_member("申买价二", pDepthMarketData->BidPrice2);
		md_json_pub->add_member("申买量二", pDepthMarketData->BidVolume2);
		md_json_pub->add_member("申卖价二", pDepthMarketData->AskPrice2);
		md_json_pub->add_member("申卖量二", pDepthMarketData->AskVolume2);
		md_json_pub->add_member("申买价三", pDepthMarketData->BidPrice3);
		md_json_pub->add_member("申买量三", pDepthMarketData->BidVolume3);
		md_json_pub->add_member("申卖价三", pDepthMarketData->AskPrice3);
		md_json_pub->add_member("申卖量三", pDepthMarketData->AskVolume3);
		md_json_pub->add_member("申买价四", pDepthMarketData->BidPrice4);
		md_json_pub->add_member("申买量四", pDepthMarketData->BidVolume4);
		md_json_pub->add_member("申卖价四", pDepthMarketData->AskPrice4);
		md_json_pub->add_member("申卖量四", pDepthMarketData->AskVolume4);
		md_json_pub->add_member("申买价五", pDepthMarketData->BidPrice5);
		md_json_pub->add_member("申买量五", pDepthMarketData->BidVolume5);
		md_json_pub->add_member("申卖价五", pDepthMarketData->AskPrice5);
		md_json_pub->add_member("申卖量五", pDepthMarketData->AskVolume5);
		md_json_pub->add_member("当日均价", pDepthMarketData->AveragePrice);
		md_json_pub->add_member("业务日期", pDepthMarketData->ActionDay);

		//--- 发送数据
		strcpy_s(md_send_pub,md_json_pub->get_string());
		zmq_send(md_socket_pub, md_send_pub, sizeof(md_send_pub), 0);
		printf("发送数据:%s\n", md_send_pub);
		md_json_pub->remove_members();
	}
};