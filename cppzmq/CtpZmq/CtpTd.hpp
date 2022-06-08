#include "Utility.hpp"

#define TDADDRPUB "tcp://*:5554"     // 交易订阅地址
#define TDADDRREP "tcp://*:5555"     // 交易请求地址


class CtpTd : public CThostFtdcTraderSpi
{
public:

	CThostFtdcTraderApi* td_api;
	//--- ctp相关
	const char*		td_host;
	const char*		td_broker;
	const char*		td_user;
	const char*		td_password;
	const char*		td_appid;
	const char*		td_authcode;
	// --- 报单
	int				td_front_id;
	int				td_session_id;
	unsigned int	td_order_ref;
	unsigned int    td_request_id;

	//--- zmq相关
	void*			td_context;
	void*			td_socket_pub;
	void*			td_socket_rep;
	char			td_send_pub[2048];
	char			td_send_rep[128];		// 一般只回复yes
	char			td_recv_rep[1024];
	CRdJson*		td_json_pub;
	CRdJson*		td_json_rep;

public:
	CtpTd(const char* host, const char* broker, const char* user, const char* password, const char* appid, const char* authcode) :
		td_host(host),
		td_broker(broker),
		td_user(user),
		td_password(password),
		td_appid(appid),
		td_authcode(authcode),
		td_front_id(0),
		td_session_id(0),
		td_order_ref(0),
		td_request_id(0)
	{
		// 创建Api, 并注册Spi
		td_api = CThostFtdcTraderApi::CreateFtdcTraderApi();
		td_api->RegisterSpi(this);

		// 新建socket
		td_context = zmq_ctx_new();
		td_socket_pub = zmq_socket(td_context, ZMQ_PUB);
		td_socket_rep = zmq_socket(td_context, ZMQ_REP);

		// 初始化数据
		memset(td_send_pub, 0x00, sizeof(td_send_pub));
		memset(td_send_rep, 0x00, sizeof(td_send_rep));
		memset(td_recv_rep, 0x00, sizeof(td_recv_rep));
		td_json_pub = new CRdJson();
		td_json_rep = new CRdJson();
	}

	~CtpTd()
	{
		td_api->Release();

		// 解绑 -> 断开 -> 关闭
		zmq_unbind(td_socket_pub, TDADDRPUB);
		zmq_unbind(td_socket_rep, TDADDRREP);
		zmq_disconnect(td_socket_pub, TDADDRPUB);
		zmq_disconnect(td_socket_rep, TDADDRREP);
		zmq_close(td_socket_pub);
		zmq_close(td_socket_rep);
		zmq_ctx_shutdown(td_context);
		zmq_ctx_term(td_context);

		delete td_json_pub;
		delete td_json_rep;
	}

	//--- 1. Init -> 自动回调连接 -> 认证
	void OnFrontConnected()
	{
		printf("Connected.\n");
		// 认证
		CThostFtdcReqAuthenticateField Req;
		memset(&Req, 0x00, sizeof(Req));
		strncpy_s(Req.BrokerID, td_broker, sizeof(Req.BrokerID) - 1);
		strncpy_s(Req.UserID, td_user, sizeof(Req.UserID) - 1);
		strncpy_s(Req.AuthCode, td_authcode, sizeof(Req.AuthCode) - 1);
		strncpy_s(Req.AppID, td_appid, sizeof(Req.AppID) - 1);
		td_api->ReqAuthenticate(&Req, td_request_id++);
	}
	//--- 连接断开
	void OnFrontDisconnected(int nReason)
	{
		printf("Disconnected.\n");
	}

	//--- 2. 回调认证 -> 登录
	void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo->ErrorID != 0)
			printf("OnRspAuthenticate:%s\n", pRspInfo->ErrorMsg);
		else
			printf("Authenticate succeeded.\n");

		// 登录
		printf("Login...\n");
		CThostFtdcReqUserLoginField Req;

		memset(&Req, 0x00, sizeof(Req));
		strncpy_s(Req.BrokerID, td_broker, sizeof(Req.BrokerID) - 1);
		strncpy_s(Req.UserID, td_user, sizeof(Req.UserID) - 1);
		strncpy_s(Req.Password, td_password, sizeof(Req.Password) - 1);
		td_api->ReqUserLogin(&Req, td_request_id++);
	}

	//--- 3. 回调登录 -> 确认结算单 -> 准备就绪信号
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("Login failed. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}
		printf("Login succeeded.TradingDay:%s,FrontID=%d,SessionID=%d\n", pRspUserLogin->TradingDay, pRspUserLogin->FrontID, pRspUserLogin->SessionID);

		// 记录订单唯一字段
		td_front_id = pRspUserLogin->FrontID;
		td_session_id = pRspUserLogin->SessionID;
		td_order_ref = 1;

		// 确认结算单
		CThostFtdcSettlementInfoConfirmField SettlementInfoConfirmField;
		memset(&SettlementInfoConfirmField, 0x00, sizeof(SettlementInfoConfirmField));
		strncpy_s(SettlementInfoConfirmField.BrokerID, pRspUserLogin->BrokerID, sizeof(SettlementInfoConfirmField.BrokerID) - 1);
		strncpy_s(SettlementInfoConfirmField.InvestorID, pRspUserLogin->UserID, sizeof(SettlementInfoConfirmField.InvestorID) - 1);
		strncpy_s(SettlementInfoConfirmField.ConfirmDate, pRspUserLogin->TradingDay, sizeof(SettlementInfoConfirmField.ConfirmDate) - 1);
		strncpy_s(SettlementInfoConfirmField.ConfirmTime, pRspUserLogin->LoginTime, sizeof(SettlementInfoConfirmField.ConfirmTime) - 1);
		td_api->ReqSettlementInfoConfirm(&SettlementInfoConfirmField, td_request_id++);

		// 发出信号
		_semaphore.signal();
	}

	//--- 启动
	bool TdBind()
	{
		// 绑定zmq
		zmq_bind(td_socket_pub, TDADDRPUB);
		zmq_bind(td_socket_rep, TDADDRREP);

		// 注册交易前置, 订阅私有流, 订阅公有流, 初始化
		td_api->RegisterFront(const_cast<char*>(td_host));
		td_api->SubscribePrivateTopic(THOST_TERT_QUICK);
		td_api->SubscribePublicTopic(THOST_TERT_QUICK);
		td_api->Init();
		printf("版本号:%s, 交易日:%s\n", td_api->GetApiVersion(), td_api->GetTradingDay());

		return(true);
	}

	bool TdResponse()
	{
		strcpy_s(td_send_pub, "防阻塞测试:pub/sub");
		zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);

		while (true)
		{
			_semaphore.wait();
			// 接收指令
			zmq_recv(td_socket_rep, td_recv_rep, sizeof(td_recv_rep), 0);
			printf("recv:%s\n", td_recv_rep);
			strcpy_s(td_send_rep, "yes");
			zmq_send(td_socket_rep, td_send_rep, sizeof(td_send_rep), 0);

			// 解析json数据
			td_json_rep->m_doc.Parse(td_recv_rep);
			if (td_json_rep->m_doc.HasParseError())
			{
				printf("Parse Error!\n");
				continue;
			}
			if (!td_json_rep->m_doc.HasMember("func") || !td_json_rep->m_doc.HasMember("params"))
			{
				printf("No func or params");
				continue;
			}

			// 判断执行命令
			std::string func = td_json_rep->m_doc["func"].GetString();
			rj::Value& params = td_json_rep->m_doc["params"];

			int req_result =
				func == "ReqOrderInsert"						? ReqOrderInsert(params) :
				func == "ReqOrderAction"						? ReqOrderAction(params) :
				func == "ReqQryDepthMarketData"					? ReqQryDepthMarketData(params) :
				func == "ReqQryExchange"						? ReqQryExchange(params) : 
				func == "ReqQryExchangeMarginRate"				? ReqQryExchangeMarginRate(params) : 
				func == "ReqQryInstrument"						? ReqQryInstrument(params) :
				func == "ReqQryInstrumentCommissionRate"		? ReqQryInstrumentCommissionRate(params) :
				func == "ReqQryInstrumentMarginRate"			? ReqQryInstrumentMarginRate(params) :
				func == "ReqQryInstrumentOrderCommRate"			? ReqQryInstrumentOrderCommRate(params) :
				func == "ReqQryInvestor"						? ReqQryInvestor(params) :
				func == "ReqQryInvestorPosition"				? ReqQryInvestorPosition(params) :
				func == "ReqQryInvestorPositionCombineDetail"	? ReqQryInvestorPositionCombineDetail(params) :
				func == "ReqQryInvestorPositionDetail"			? ReqQryInvestorPositionDetail(params) :
				func == "ReqQryInvestUnit"						? ReqQryInvestUnit(params) :
				func == "ReqQryOrder"							? ReqQryOrder(params) :
				func == "ReqQryProduct"							? ReqQryProduct(params) :
				func == "ReqQryTrade" ? ReqQryTrade(params) :
				func == "ReqQryTradingAccount" ? ReqQryTradingAccount(params) :
				func == "ReqQryTradingCode" ? ReqQryTradingCode(params) :
				func == "ReqQryTradingNotice" ? ReqQryTradingNotice(params) :
				func == "" ? ReqQryInvestor(params) : -100;

			switch (req_result)
			{
			case(-100):
				printf("不存在函数:%s", func.c_str());
				strcpy_s(td_send_pub, "func Error!");
				zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
				break;
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

	int ReqOrderInsert(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();
		const char* ExchangeID = params["ExchangeID"].GetString();
		int Direction = params["Direction"].GetInt();
		int CombOffsetFlag = params["CombOffsetFlag"].GetInt();
		int VolumeTotalOriginal = params["VolumeTotalOriginal"].GetInt();
		double LimitPrice = params["LimitPrice"].GetDouble();

		CThostFtdcInputOrderField Req;
		memset(&Req, 0x00, sizeof(Req));
		strncpy_s(Req.BrokerID, td_broker, sizeof(Req.BrokerID) - 1);
		strncpy_s(Req.InvestorID, td_user, sizeof(Req.InvestorID) - 1);
		strcpy_s(Req.InstrumentID, InstrumentID);
		strcpy_s(Req.ExchangeID, ExchangeID);
		Req.Direction = Direction;
		Req.CombOffsetFlag[0] = CombOffsetFlag;
		Req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		Req.VolumeTotalOriginal = VolumeTotalOriginal;
		Req.LimitPrice = LimitPrice;
		Req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		sprintf_s(Req.OrderRef, "%d", td_order_ref++);
		Req.TimeCondition = THOST_FTDC_TC_GFD;
		if (Req.OrderPriceType == THOST_FTDC_OPT_AnyPrice)
			Req.TimeCondition = THOST_FTDC_TC_IOC;
		Req.VolumeCondition = THOST_FTDC_VC_AV;
		Req.MinVolume = 1;
		Req.ContingentCondition = THOST_FTDC_CC_Immediately;
		Req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		Req.IsAutoSuspend = 0;
		Req.UserForceClose = 0;
		return td_api->ReqOrderInsert(&Req, td_request_id++);
	}

	// 下单应答
	void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspOrderInsert. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}
		printf("OnRspOrderInsert:InstrumentID:%s,ExchangeID:%s,VolumeTotalOriginal:%d,LimitPrice:%lf,RequestID:%d,InvestUnitID:%s\n", pInputOrder->InstrumentID, pInputOrder->ExchangeID, pInputOrder->VolumeTotalOriginal, pInputOrder->LimitPrice, pInputOrder->RequestID, pInputOrder->InvestUnitID);
	}

	int ReqOrderAction(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();

		CThostFtdcInputOrderActionField Req;
		memset(&Req, 0x00, sizeof(Req));
		strncpy_s(Req.BrokerID, td_broker, sizeof(Req.BrokerID) - 1);
		strncpy_s(Req.InvestorID, td_user, sizeof(Req.InvestorID) - 1);
		strncpy_s(Req.InstrumentID, InstrumentID, sizeof(Req.InstrumentID) - 1);
		Req.FrontID = td_front_id;
		Req.SessionID = td_session_id;
		sprintf_s(Req.OrderRef, "%d", td_order_ref);
		Req.ActionFlag = THOST_FTDC_AF_Delete;
		return td_api->ReqOrderAction(&Req, td_request_id++);
	}

	// 撤单应答
	void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspOrderAction. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}
		printf("OnRspOrderAction:InstrumentID:%s,ExchangeID:%s,OrderSysID:%s,FrontID:%d,SessionID:%d,OrderRef:%s,RequestID:%d,InvestUnitID:%s\n", pInputOrderAction->InstrumentID, pInputOrderAction->ExchangeID, pInputOrderAction->OrderSysID, pInputOrderAction->FrontID, pInputOrderAction->SessionID, pInputOrderAction->OrderRef, pInputOrderAction->RequestID, pInputOrderAction->InvestUnitID);
	}
	
	int ReqQryDepthMarketData(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();
		CThostFtdcQryDepthMarketDataField QryDepthMarketData = { 0 };
		strncpy_s(QryDepthMarketData.InstrumentID, InstrumentID, sizeof(QryDepthMarketData.InstrumentID) - 1);
		return td_api->ReqQryDepthMarketData(&QryDepthMarketData, td_request_id++);
	}

	//--- 查询行情列表
	void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspQryDepthMarketData. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}

		td_json_pub->add_member("交易日", pDepthMarketData->TradingDay);
		td_json_pub->add_member("合约代码", pDepthMarketData->InstrumentID);
		td_json_pub->add_member("交易所代码", pDepthMarketData->ExchangeID);
		td_json_pub->add_member("合约在交易所的代码", pDepthMarketData->ExchangeInstID);
		td_json_pub->add_member("最新价", double_format(pDepthMarketData->LastPrice));
		td_json_pub->add_member("上次结算价", double_format(pDepthMarketData->PreSettlementPrice));
		td_json_pub->add_member("昨收盘", double_format(pDepthMarketData->PreClosePrice));
		td_json_pub->add_member("昨持仓量", double_format(pDepthMarketData->PreOpenInterest));
		td_json_pub->add_member("今开盘", double_format(pDepthMarketData->OpenPrice));
		td_json_pub->add_member("最高价", double_format(pDepthMarketData->HighestPrice));
		td_json_pub->add_member("最低价", double_format(pDepthMarketData->LowestPrice));
		td_json_pub->add_member("数量", pDepthMarketData->Volume);
		td_json_pub->add_member("成交金额", double_format(pDepthMarketData->Turnover));
		td_json_pub->add_member("持仓量", double_format(pDepthMarketData->OpenInterest));
		td_json_pub->add_member("今收盘", double_format(pDepthMarketData->ClosePrice));
		td_json_pub->add_member("本次结算价", double_format(pDepthMarketData->SettlementPrice));
		td_json_pub->add_member("涨停板价", double_format(pDepthMarketData->UpperLimitPrice));
		td_json_pub->add_member("跌停板价", double_format(pDepthMarketData->LowerLimitPrice));
		td_json_pub->add_member("最后修改时间", pDepthMarketData->UpdateTime);
		td_json_pub->add_member("最后修改毫秒", pDepthMarketData->UpdateMillisec);

		td_json_pub->add_member("申买价一", pDepthMarketData->BidPrice1);
		td_json_pub->add_member("申买量一", pDepthMarketData->BidVolume1);
		td_json_pub->add_member("申卖价一", pDepthMarketData->AskPrice1);
		td_json_pub->add_member("申卖量一", pDepthMarketData->AskVolume1);
		td_json_pub->add_member("申买价二", pDepthMarketData->BidPrice2);
		td_json_pub->add_member("申买量二", pDepthMarketData->BidVolume2);
		td_json_pub->add_member("申卖价二", pDepthMarketData->AskPrice2);
		td_json_pub->add_member("申卖量二", pDepthMarketData->AskVolume2);
		td_json_pub->add_member("申买价三", pDepthMarketData->BidPrice3);
		td_json_pub->add_member("申买量三", pDepthMarketData->BidVolume3);
		td_json_pub->add_member("申买价三", pDepthMarketData->AskPrice3);
		td_json_pub->add_member("申买量三", pDepthMarketData->AskVolume3);
		td_json_pub->add_member("申买价四", pDepthMarketData->BidPrice4);
		td_json_pub->add_member("申买量四", pDepthMarketData->BidVolume4);
		td_json_pub->add_member("申卖价四", pDepthMarketData->AskPrice4);
		td_json_pub->add_member("申卖量四", pDepthMarketData->AskVolume4);
		td_json_pub->add_member("申买价五", pDepthMarketData->BidPrice5);
		td_json_pub->add_member("申买量五", pDepthMarketData->BidVolume5);
		td_json_pub->add_member("申卖价五", pDepthMarketData->AskPrice5);
		td_json_pub->add_member("申卖量五", pDepthMarketData->AskVolume5);

		td_json_pub->add_member("当日均价", pDepthMarketData->AveragePrice);
		td_json_pub->add_member("业务日期", pDepthMarketData->ActionDay);

		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryExchange(rj::Value& params)
	{
		const char* ExchangeID = params["ExchangeID"].GetString();

		CThostFtdcQryExchangeField QryExchange = { 0 };
		strncpy_s(QryExchange.ExchangeID, ExchangeID, sizeof(QryExchange.ExchangeID) - 1);
		return td_api->ReqQryExchange(&QryExchange, td_request_id++);
	}

	//--- 查询交易所 -> 查询完毕信号
	void OnRspQryExchange(CThostFtdcExchangeField* pExchange, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pExchange)
		{
			td_json_pub->add_member("交易所代码", pExchange->ExchangeID);
			td_json_pub->add_member("交易所名称", pExchange->ExchangeName);
			td_json_pub->add_member("交易所属性", pExchange->ExchangeProperty);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryExchangeMarginRate(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();

		CThostFtdcQryExchangeMarginRateField ExchangeMarginRate{ 0 };
		strncpy_s(ExchangeMarginRate.BrokerID, td_broker, sizeof(ExchangeMarginRate.BrokerID) - 1);
		strncpy_s(ExchangeMarginRate.InstrumentID, InstrumentID, sizeof(ExchangeMarginRate.InstrumentID) - 1);
		ExchangeMarginRate.HedgeFlag = 1;
		return td_api->ReqQryExchangeMarginRate(&ExchangeMarginRate, td_request_id++);
	}

	//--- 请求查询交易所保证金率响应
	void OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField* pExchangeMarginRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pExchangeMarginRate)
		{
			td_json_pub->add_member("经纪公司代码", pExchangeMarginRate->BrokerID);
			td_json_pub->add_member("合约代码", pExchangeMarginRate->InstrumentID);
			td_json_pub->add_member("投机套保标志", pExchangeMarginRate->HedgeFlag);
			td_json_pub->add_member("多头保证金率", pExchangeMarginRate->LongMarginRatioByMoney);
			td_json_pub->add_member("多头保证金费", pExchangeMarginRate->LongMarginRatioByVolume);
			td_json_pub->add_member("空头保证金率", pExchangeMarginRate->ShortMarginRatioByMoney);
			td_json_pub->add_member("空头保证金费", pExchangeMarginRate->ShortMarginRatioByVolume);
			td_json_pub->add_member("交易所代码", pExchangeMarginRate->ExchangeID);
		}

		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryInstrument(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();

		CThostFtdcQryInstrumentField QryInstrument = { 0 };
		strncpy_s(QryInstrument.InstrumentID, InstrumentID, sizeof(QryInstrument.InstrumentID) - 1);
		return td_api->ReqQryInstrument(&QryInstrument, td_request_id++);
	}
	//--- 查询合约列表
	void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInstrument)
		{
			td_json_pub->add_member("合约代码", pInstrument->InstrumentID);
			td_json_pub->add_member("交易所代码", pInstrument->ExchangeID);
			td_json_pub->add_member("合约名称", pInstrument->InstrumentName);
			td_json_pub->add_member("合约在交易所的代码", pInstrument->ExchangeInstID);
			td_json_pub->add_member("产品代码", pInstrument->ProductID);
			td_json_pub->add_member("产品类型", pInstrument->ProductClass);
			td_json_pub->add_member("交割年份", pInstrument->DeliveryYear);
			td_json_pub->add_member("交割月", pInstrument->DeliveryMonth);
			td_json_pub->add_member("市价单最大下单量", pInstrument->MaxMarketOrderVolume);
			td_json_pub->add_member("市价单最小下单量", pInstrument->MinMarketOrderVolume);
			td_json_pub->add_member("限价单最大下单量", pInstrument->MaxLimitOrderVolume);
			td_json_pub->add_member("限价单最小下单量", pInstrument->MinLimitOrderVolume);
			td_json_pub->add_member("合约数量乘数", pInstrument->VolumeMultiple);
			td_json_pub->add_member("最小变动价位", pInstrument->PriceTick);
			td_json_pub->add_member("创建日", pInstrument->CreateDate);
			td_json_pub->add_member("上市日", pInstrument->OpenDate);
			td_json_pub->add_member("到期日", pInstrument->ExpireDate);
			td_json_pub->add_member("开始交割日", pInstrument->StartDelivDate);
			td_json_pub->add_member("结束交割日", pInstrument->EndDelivDate);
			td_json_pub->add_member("合约生命周期状态", pInstrument->InstLifePhase);
			td_json_pub->add_member("当前是否交易", pInstrument->IsTrading);
			td_json_pub->add_member("持仓类型", pInstrument->PositionType);
			td_json_pub->add_member("持仓日期类型", pInstrument->PositionDateType);
			td_json_pub->add_member("多头保证金率", pInstrument->LongMarginRatio);
			td_json_pub->add_member("空头保证金率", pInstrument->ShortMarginRatio);
			td_json_pub->add_member("是否使用大额单边保证金算法", pInstrument->MaxMarginSideAlgorithm);
			td_json_pub->add_member("基础商品代码", pInstrument->UnderlyingInstrID);
			td_json_pub->add_member("合约基础商品乘数", pInstrument->UnderlyingMultiple);
			td_json_pub->add_member("组合类型", pInstrument->CombinationType);
		}

		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryInstrumentCommissionRate(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();

		CThostFtdcQryInstrumentCommissionRateField InstrumentCommissionRate = { 0 };
		strncpy_s(InstrumentCommissionRate.InstrumentID, InstrumentID, sizeof(InstrumentCommissionRate.InstrumentID) - 1);
		strncpy_s(InstrumentCommissionRate.BrokerID, td_broker, sizeof(InstrumentCommissionRate.BrokerID) - 1);
		strncpy_s(InstrumentCommissionRate.InvestorID, td_user, sizeof(InstrumentCommissionRate.InvestorID) - 1);
		return td_api->ReqQryInstrumentCommissionRate(&InstrumentCommissionRate, td_request_id++);
	}

	void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField* pInstrumentCommissionRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInstrumentCommissionRate)
		{
			td_json_pub->add_member("合约代码",pInstrumentCommissionRate->InstrumentID);
			td_json_pub->add_member("投资者范围", pInstrumentCommissionRate->InvestorRange);
			td_json_pub->add_member("经纪公司代码", pInstrumentCommissionRate->BrokerID);
			td_json_pub->add_member("投资者代码", pInstrumentCommissionRate->InvestorID);
			td_json_pub->add_member("开仓手续费率", pInstrumentCommissionRate->OpenRatioByMoney);
			td_json_pub->add_member("开仓手续费", pInstrumentCommissionRate->OpenRatioByVolume);
			td_json_pub->add_member("平仓手续费率", pInstrumentCommissionRate->CloseRatioByMoney);
			td_json_pub->add_member("平仓手续费", pInstrumentCommissionRate->CloseRatioByVolume);
			td_json_pub->add_member("平今手续费率", pInstrumentCommissionRate->CloseTodayRatioByMoney);
			td_json_pub->add_member("平今手续费", pInstrumentCommissionRate->CloseTodayRatioByVolume);
			td_json_pub->add_member("交易所代码", pInstrumentCommissionRate->ExchangeID);
			td_json_pub->add_member("业务类型", pInstrumentCommissionRate->BizType);
			td_json_pub->add_member("投资单元代码", pInstrumentCommissionRate->InvestUnitID);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryInstrumentMarginRate(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();

		CThostFtdcQryInstrumentMarginRateField InstrumentMarginRate = { 0 };
		strncpy_s(InstrumentMarginRate.InstrumentID, InstrumentID, sizeof(InstrumentMarginRate.InstrumentID) - 1);
		strncpy_s(InstrumentMarginRate.BrokerID, td_broker, sizeof(InstrumentMarginRate.BrokerID) - 1);
		strncpy_s(InstrumentMarginRate.InvestorID, td_user, sizeof(InstrumentMarginRate.InvestorID) - 1);
		InstrumentMarginRate.HedgeFlag = '1';
		return td_api->ReqQryInstrumentMarginRate(&InstrumentMarginRate, td_request_id++);
	}

	void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField* pInstrumentMarginRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInstrumentMarginRate)
		{
			td_json_pub->add_member("合约代码", pInstrumentMarginRate->InstrumentID);
			td_json_pub->add_member("投资者范围", pInstrumentMarginRate->InvestorRange);
			td_json_pub->add_member("经纪公司代码", pInstrumentMarginRate->BrokerID);
			td_json_pub->add_member("投资者代码", pInstrumentMarginRate->InvestorID);
			td_json_pub->add_member("投机套保标志", pInstrumentMarginRate->HedgeFlag);
			td_json_pub->add_member("多头保证金率", pInstrumentMarginRate->LongMarginRatioByMoney);
			td_json_pub->add_member("多头保证金费", pInstrumentMarginRate->LongMarginRatioByVolume);
			td_json_pub->add_member("空头保证金率", pInstrumentMarginRate->ShortMarginRatioByMoney);
			td_json_pub->add_member("空头保证金费", pInstrumentMarginRate->ShortMarginRatioByVolume);
			td_json_pub->add_member("是否相对交易所收取", pInstrumentMarginRate->IsRelative);
			td_json_pub->add_member("交易所代码", pInstrumentMarginRate->ExchangeID);
			td_json_pub->add_member("投资单元代码", pInstrumentMarginRate->InvestUnitID);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryInstrumentOrderCommRate(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();

		CThostFtdcQryInstrumentOrderCommRateField InstrumentOrderCommRate = { 0 };
		strncpy_s(InstrumentOrderCommRate.InstrumentID, InstrumentID, sizeof(InstrumentOrderCommRate.InstrumentID) - 1);
		strncpy_s(InstrumentOrderCommRate.BrokerID, td_broker, sizeof(InstrumentOrderCommRate.BrokerID) - 1);
		strncpy_s(InstrumentOrderCommRate.InvestorID, td_user, sizeof(InstrumentOrderCommRate.InvestorID) - 1);
		return td_api->ReqQryInstrumentOrderCommRate(&InstrumentOrderCommRate, td_request_id++);
	}

	void OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField* pInstrumentOrderCommRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInstrumentOrderCommRate)
		{
			td_json_pub->add_member("合约代码", pInstrumentOrderCommRate->InstrumentID);
			td_json_pub->add_member("投资者范围", pInstrumentOrderCommRate->InvestorRange);
			td_json_pub->add_member("经纪公司代码", pInstrumentOrderCommRate->BrokerID);
			td_json_pub->add_member("投资者代码", pInstrumentOrderCommRate->InvestorID);
			td_json_pub->add_member("投机套保标志", pInstrumentOrderCommRate->HedgeFlag);
			td_json_pub->add_member("报单手续费", pInstrumentOrderCommRate->OrderCommByVolume);
			td_json_pub->add_member("撤单手续费", pInstrumentOrderCommRate->OrderActionCommByVolume);
			td_json_pub->add_member("交易所代码", pInstrumentOrderCommRate->ExchangeID);
			td_json_pub->add_member("投资单元代码", pInstrumentOrderCommRate->InvestUnitID);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryInvestor(rj::Value& params)
	{
		CThostFtdcQryInvestorField Investor = { 0 };
		strncpy_s(Investor.BrokerID, td_broker, sizeof(Investor.BrokerID) - 1);
		strncpy_s(Investor.InvestorID, td_user, sizeof(Investor.InvestorID) - 1);
		return td_api->ReqQryInvestor(&Investor, td_request_id++);
	}

	void OnRspQryInvestor(CThostFtdcInvestorField* pInvestor, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInvestor)
		{
			td_json_pub->add_member("投资者代码", pInvestor->InvestorID);
			td_json_pub->add_member("经纪公司代码", pInvestor->BrokerID);
			td_json_pub->add_member("投资者分组代码", pInvestor->InvestorGroupID);
			td_json_pub->add_member("投资者名称", pInvestor->InvestorName);
			td_json_pub->add_member("证件类型", pInvestor->IdentifiedCardType);
			td_json_pub->add_member("证件号码", pInvestor->IdentifiedCardNo);
			td_json_pub->add_member("是否活跃", pInvestor->IsActive);
			td_json_pub->add_member("联系电话", pInvestor->Telephone);
			td_json_pub->add_member("通讯地址", pInvestor->Address);
			td_json_pub->add_member("开户日期", pInvestor->OpenDate);
			td_json_pub->add_member("手机", pInvestor->Mobile);
			td_json_pub->add_member("手续费率模板代码", pInvestor->CommModelID);
			td_json_pub->add_member("保证金率模板代码", pInvestor->MarginModelID);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryInvestorPosition(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();

		CThostFtdcQryInvestorPositionField InvestorPosition = { 0 };
		strncpy_s(InvestorPosition.InstrumentID, InstrumentID, sizeof(InvestorPosition.InstrumentID) - 1);
		strncpy_s(InvestorPosition.BrokerID, td_broker, sizeof(InvestorPosition.BrokerID) - 1);
		strncpy_s(InvestorPosition.InvestorID, td_user, sizeof(InvestorPosition.InvestorID) - 1);
		return td_api->ReqQryInvestorPosition(&InvestorPosition, td_request_id++);
	}

	void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInvestorPosition)
		{
			td_json_pub->add_member("合约代码", pInvestorPosition->InstrumentID);
			td_json_pub->add_member("经纪公司代码", pInvestorPosition->BrokerID);
			td_json_pub->add_member("投资者代码", pInvestorPosition->InvestorID);
			td_json_pub->add_member("持仓多空方向", pInvestorPosition->PosiDirection);
			td_json_pub->add_member("投机套保标志", pInvestorPosition->HedgeFlag);
			td_json_pub->add_member("持仓日期", pInvestorPosition->PositionDate);
			td_json_pub->add_member("上日持仓", pInvestorPosition->YdPosition);
			td_json_pub->add_member("今日持仓", pInvestorPosition->Position);
			td_json_pub->add_member("多头冻结", pInvestorPosition->LongFrozen);
			td_json_pub->add_member("空头冻结", pInvestorPosition->ShortFrozen);
			td_json_pub->add_member("开仓冻结金额", pInvestorPosition->LongFrozenAmount);
			td_json_pub->add_member("开仓冻结金额", pInvestorPosition->ShortFrozenAmount);
			td_json_pub->add_member("开仓量", pInvestorPosition->OpenVolume);
			td_json_pub->add_member("平仓量", pInvestorPosition->CloseVolume);
			td_json_pub->add_member("开仓金额", pInvestorPosition->OpenAmount);
			td_json_pub->add_member("平仓金额", pInvestorPosition->CloseAmount);
			td_json_pub->add_member("持仓成本", pInvestorPosition->PositionCost);
			td_json_pub->add_member("上次占用的保证金", pInvestorPosition->PreMargin);
			td_json_pub->add_member("占用的保证金", pInvestorPosition->UseMargin);
			td_json_pub->add_member("冻结的保证金", pInvestorPosition->FrozenMargin);
			td_json_pub->add_member("冻结的资金", pInvestorPosition->FrozenCash);
			td_json_pub->add_member("冻结的手续费", pInvestorPosition->FrozenCommission);
			td_json_pub->add_member("资金差额", pInvestorPosition->CashIn);
			td_json_pub->add_member("手续费", pInvestorPosition->Commission);
			td_json_pub->add_member("平仓盈亏", pInvestorPosition->CloseProfit);
			td_json_pub->add_member("持仓盈亏", pInvestorPosition->PositionProfit);
			td_json_pub->add_member("上次结算价", pInvestorPosition->PreSettlementPrice);
			td_json_pub->add_member("本次结算价", pInvestorPosition->SettlementPrice);
			td_json_pub->add_member("交易日", pInvestorPosition->TradingDay);
			td_json_pub->add_member("结算编号", pInvestorPosition->SettlementID);
			td_json_pub->add_member("开仓成本", pInvestorPosition->OpenCost);
			td_json_pub->add_member("交易所保证金", pInvestorPosition->ExchangeMargin);
			td_json_pub->add_member("组合成交形成的持仓", pInvestorPosition->CombPosition);
			td_json_pub->add_member("组合多头冻结", pInvestorPosition->CombLongFrozen);
			td_json_pub->add_member("组合空头冻结", pInvestorPosition->CombShortFrozen);
			td_json_pub->add_member("逐日盯市平仓盈亏", pInvestorPosition->CloseProfitByDate);
			td_json_pub->add_member("逐笔对冲平仓盈亏", pInvestorPosition->CloseProfitByTrade);
			td_json_pub->add_member("今日持仓", pInvestorPosition->TodayPosition);
			td_json_pub->add_member("保证金率", pInvestorPosition->MarginRateByMoney);
			td_json_pub->add_member("保证金率(按手数)", pInvestorPosition->MarginRateByVolume);
			td_json_pub->add_member("执行冻结", pInvestorPosition->StrikeFrozen);
			td_json_pub->add_member("执行冻结金额", pInvestorPosition->StrikeFrozenAmount);
			td_json_pub->add_member("放弃执行冻结", pInvestorPosition->AbandonFrozen);
			td_json_pub->add_member("交易所代码", pInvestorPosition->ExchangeID);
			td_json_pub->add_member("执行冻结的昨仓", pInvestorPosition->YdStrikeFrozen);
			td_json_pub->add_member("投资单元代码", pInvestorPosition->InvestUnitID);
			td_json_pub->add_member("大商所持仓成本差值", pInvestorPosition->PositionCostOffset);
			td_json_pub->add_member("tas持仓手数", pInvestorPosition->TasPosition);
			td_json_pub->add_member("tas持仓成本", pInvestorPosition->TasPositionCost);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryInvestorPositionCombineDetail(rj::Value& params)
	{
		const char* CombInstrumentID = params["CombInstrumentID"].GetString();

		CThostFtdcQryInvestorPositionCombineDetailField PositionCombineDetail = { 0 };
		strncpy_s(PositionCombineDetail.CombInstrumentID, CombInstrumentID, sizeof(PositionCombineDetail.CombInstrumentID) - 1);
		strncpy_s(PositionCombineDetail.BrokerID, td_broker, sizeof(PositionCombineDetail.BrokerID) - 1);
		strncpy_s(PositionCombineDetail.InvestorID, td_user, sizeof(PositionCombineDetail.InvestorID) - 1);
		return td_api->ReqQryInvestorPositionCombineDetail(&PositionCombineDetail, td_request_id++);
	}

	void OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField* pInvestorPositionCombineDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInvestorPositionCombineDetail)
		{
			td_json_pub->add_member("交易日", pInvestorPositionCombineDetail->TradingDay);
			td_json_pub->add_member("开仓日期", pInvestorPositionCombineDetail->OpenDate);
			td_json_pub->add_member("交易所代码", pInvestorPositionCombineDetail->ExchangeID);
			td_json_pub->add_member("结算编号", pInvestorPositionCombineDetail->SettlementID);
			td_json_pub->add_member("经纪公司代码", pInvestorPositionCombineDetail->BrokerID);
			td_json_pub->add_member("投资者代码", pInvestorPositionCombineDetail->InvestorID);
			td_json_pub->add_member("组合编号", pInvestorPositionCombineDetail->ComTradeID);
			td_json_pub->add_member("撮合编号", pInvestorPositionCombineDetail->TradeID);
			td_json_pub->add_member("合约代码", pInvestorPositionCombineDetail->InstrumentID);
			td_json_pub->add_member("投机套保标志", pInvestorPositionCombineDetail->HedgeFlag);
			td_json_pub->add_member("买卖", pInvestorPositionCombineDetail->Direction);
			td_json_pub->add_member("持仓量", pInvestorPositionCombineDetail->TotalAmt);
			td_json_pub->add_member("投资者保证金", pInvestorPositionCombineDetail->Margin);
			td_json_pub->add_member("交易所保证金", pInvestorPositionCombineDetail->ExchMargin);
			td_json_pub->add_member("保证金率", pInvestorPositionCombineDetail->MarginRateByMoney);
			td_json_pub->add_member("保证金率(按手数)", pInvestorPositionCombineDetail->MarginRateByVolume);
			td_json_pub->add_member("单腿编号", pInvestorPositionCombineDetail->LegID);
			td_json_pub->add_member("单腿乘数", pInvestorPositionCombineDetail->LegMultiple);
			td_json_pub->add_member("组合持仓合约编码", pInvestorPositionCombineDetail->CombInstrumentID);
			td_json_pub->add_member("成交组号", pInvestorPositionCombineDetail->TradeGroupID);
			td_json_pub->add_member("投资单元代码", pInvestorPositionCombineDetail->InvestUnitID);
		}

		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryInvestorPositionDetail(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();

		CThostFtdcQryInvestorPositionDetailField PositionDetail = { 0 };
		strncpy_s(PositionDetail.InstrumentID, InstrumentID, sizeof(PositionDetail.InstrumentID) - 1);
		strncpy_s(PositionDetail.BrokerID, td_broker, sizeof(PositionDetail.BrokerID) - 1);
		strncpy_s(PositionDetail.InvestorID, td_user, sizeof(PositionDetail.InvestorID) - 1);
		return td_api->ReqQryInvestorPositionDetail(&PositionDetail, td_request_id++);
	}

	void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField* pInvestorPositionDetail, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInvestorPositionDetail)
		{
			td_json_pub->add_member("合约代码", pInvestorPositionDetail->InstrumentID);
			td_json_pub->add_member("经纪公司代码", pInvestorPositionDetail->BrokerID);
			td_json_pub->add_member("投资者代码", pInvestorPositionDetail->InvestorID);
			td_json_pub->add_member("投机套保标志", pInvestorPositionDetail->HedgeFlag);
			td_json_pub->add_member("买卖", pInvestorPositionDetail->Direction);
			td_json_pub->add_member("开仓日期", pInvestorPositionDetail->OpenDate);
			td_json_pub->add_member("成交编号", pInvestorPositionDetail->TradeID);
			td_json_pub->add_member("数量", pInvestorPositionDetail->Volume);
			td_json_pub->add_member("开仓价", pInvestorPositionDetail->OpenPrice);
			td_json_pub->add_member("交易日", pInvestorPositionDetail->TradingDay);
			td_json_pub->add_member("结算编号", pInvestorPositionDetail->SettlementID);
			td_json_pub->add_member("成交类型", pInvestorPositionDetail->TradeType);
			td_json_pub->add_member("组合合约代码", pInvestorPositionDetail->CombInstrumentID);
			td_json_pub->add_member("交易所代码", pInvestorPositionDetail->ExchangeID);
			td_json_pub->add_member("逐日盯市平仓盈亏", pInvestorPositionDetail->CloseProfitByDate);
			td_json_pub->add_member("逐笔对冲平仓盈亏", pInvestorPositionDetail->CloseProfitByTrade);
			td_json_pub->add_member("逐日盯市持仓盈亏", pInvestorPositionDetail->PositionProfitByDate);
			td_json_pub->add_member("逐笔对冲持仓盈亏", pInvestorPositionDetail->PositionProfitByTrade);
			td_json_pub->add_member("投资者保证金", pInvestorPositionDetail->Margin);
			td_json_pub->add_member("交易所保证金", pInvestorPositionDetail->ExchMargin);
			td_json_pub->add_member("保证金率", pInvestorPositionDetail->MarginRateByMoney);
			td_json_pub->add_member("保证金率(按手数)", pInvestorPositionDetail->MarginRateByVolume);
			td_json_pub->add_member("昨结算价", pInvestorPositionDetail->LastSettlementPrice);
			td_json_pub->add_member("结算价", pInvestorPositionDetail->SettlementPrice);
			td_json_pub->add_member("平仓量", pInvestorPositionDetail->CloseVolume);
			td_json_pub->add_member("平仓金额", pInvestorPositionDetail->CloseAmount);
			td_json_pub->add_member("先开先平剩余数量", pInvestorPositionDetail->TimeFirstVolume);
			td_json_pub->add_member("投资单元代码", pInvestorPositionDetail->InvestUnitID);
			td_json_pub->add_member("特殊持仓标志", pInvestorPositionDetail->SpecPosiType);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryInvestUnit(rj::Value& params)
	{
		CThostFtdcQryInvestUnitField InvestUnit = { 0 };
		strncpy_s(InvestUnit.BrokerID, td_broker, sizeof(InvestUnit.BrokerID) - 1);
		strncpy_s(InvestUnit.InvestorID, td_user, sizeof(InvestUnit.InvestorID) - 1);
		return td_api->ReqQryInvestUnit(&InvestUnit, td_request_id++);
	}

	void OnRspQryInvestUnit(CThostFtdcInvestUnitField* pInvestUnit, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInvestUnit)
		{
			td_json_pub->add_member("经纪公司代码", pInvestUnit->BrokerID);
			td_json_pub->add_member("投资者代码", pInvestUnit->InvestorID);
			td_json_pub->add_member("投资单元代码", pInvestUnit->InvestUnitID);
			td_json_pub->add_member("投资者单元名称", pInvestUnit->InvestorUnitName);
			td_json_pub->add_member("投资者分组代码", pInvestUnit->InvestorGroupID);
			td_json_pub->add_member("手续费率模板代码", pInvestUnit->CommModelID);
			td_json_pub->add_member("保证金率模板代码", pInvestUnit->MarginModelID);
			td_json_pub->add_member("资金账号", pInvestUnit->AccountID);
			td_json_pub->add_member("币种代码", pInvestUnit->CurrencyID);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryOrder(rj::Value& params)
	{
		const char* ExchangeID = params["ExchangeID"].GetString();

		CThostFtdcQryOrderField Order = { 0 };
		strncpy_s(Order.ExchangeID, ExchangeID, sizeof(Order.ExchangeID) - 1);
		strncpy_s(Order.BrokerID, td_broker, sizeof(Order.BrokerID) - 1);
		strncpy_s(Order.InvestorID, td_user, sizeof(Order.InvestorID) - 1);
		return td_api->ReqQryOrder(&Order, td_request_id++);
	}

	void OnRspQryOrder(CThostFtdcOrderField* pOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pOrder)
		{
			td_json_pub->add_member("经纪公司代码", pOrder->BrokerID);
			td_json_pub->add_member("投资者代码", pOrder->InvestorID);
			td_json_pub->add_member("合约代码", pOrder->InstrumentID);
			td_json_pub->add_member("报单引用", pOrder->OrderRef);
			td_json_pub->add_member("用户代码", pOrder->UserID);
			td_json_pub->add_member("报单价格条件", pOrder->OrderPriceType);
			td_json_pub->add_member("买卖方向", pOrder->Direction);
			td_json_pub->add_member("组合开平标志", pOrder->CombOffsetFlag);
			td_json_pub->add_member("组合投机套保标志", pOrder->CombHedgeFlag);
			td_json_pub->add_member("价格", pOrder->LimitPrice);
			td_json_pub->add_member("数量", pOrder->VolumeTotalOriginal);
			td_json_pub->add_member("有效期类型", pOrder->TimeCondition);
			td_json_pub->add_member("GTD日期", pOrder->GTDDate);
			td_json_pub->add_member("成交量类型", pOrder->VolumeCondition);
			td_json_pub->add_member("最小成交量", pOrder->MinVolume);
			td_json_pub->add_member("触发条件", pOrder->ContingentCondition);
			td_json_pub->add_member("止损价", pOrder->StopPrice);
			td_json_pub->add_member("强平原因", pOrder->ForceCloseReason);
			td_json_pub->add_member("自动挂起标志", pOrder->IsAutoSuspend);
			td_json_pub->add_member("业务单元", pOrder->BusinessUnit);
			td_json_pub->add_member("请求编号", pOrder->RequestID);
			td_json_pub->add_member("本地报单编号", pOrder->OrderLocalID);
			td_json_pub->add_member("交易所代码", pOrder->ExchangeID);
			td_json_pub->add_member("会员代码", pOrder->ParticipantID);
			td_json_pub->add_member("客户代码", pOrder->ClientID);
			td_json_pub->add_member("合约在交易所的代码", pOrder->ExchangeInstID);
			td_json_pub->add_member("交易所交易员代码", pOrder->TraderID);
			td_json_pub->add_member("安装编号", pOrder->InstallID);
			td_json_pub->add_member("报单提交状态", pOrder->OrderSubmitStatus);
			td_json_pub->add_member("报单提示序号", pOrder->NotifySequence);
			td_json_pub->add_member("交易日", pOrder->TradingDay);
			td_json_pub->add_member("结算编号", pOrder->SettlementID);
			td_json_pub->add_member("报单编号", pOrder->OrderSysID);
			td_json_pub->add_member("报单来源", pOrder->OrderSource);
			td_json_pub->add_member("报单状态", pOrder->OrderStatus);
			td_json_pub->add_member("报单类型", pOrder->OrderType);
			td_json_pub->add_member("今成交数量", pOrder->VolumeTraded);
			td_json_pub->add_member("剩余数量", pOrder->VolumeTotal);
			td_json_pub->add_member("报单日期", pOrder->InsertDate);
			td_json_pub->add_member("委托时间", pOrder->InsertTime);
			td_json_pub->add_member("激活时间", pOrder->ActiveTime);
			td_json_pub->add_member("挂起时间", pOrder->SuspendTime);
			td_json_pub->add_member("最后修改时间", pOrder->UpdateTime);
			td_json_pub->add_member("撤销时间", pOrder->CancelTime);
			td_json_pub->add_member("最后修改交易所交易员代码", pOrder->ActiveTraderID);
			td_json_pub->add_member("结算会员编号", pOrder->ClearingPartID);
			td_json_pub->add_member("序号", pOrder->SequenceNo);
			td_json_pub->add_member("前置编号", pOrder->FrontID);
			td_json_pub->add_member("会话编号", pOrder->SessionID);
			td_json_pub->add_member("用户端产品信息", pOrder->UserProductInfo);
			td_json_pub->add_member("状态信息", pOrder->StatusMsg);
			td_json_pub->add_member("用户强评标志", pOrder->UserForceClose);
			td_json_pub->add_member("操作用户代码", pOrder->ActiveUserID);
			td_json_pub->add_member("经纪公司报单编号", pOrder->BrokerOrderSeq);
			td_json_pub->add_member("相关报单", pOrder->RelativeOrderSysID);
			td_json_pub->add_member("郑商所成交数量", pOrder->ZCETotalTradedVolume);
			td_json_pub->add_member("互换单标志", pOrder->IsSwapOrder);
			td_json_pub->add_member("营业部编号", pOrder->BranchID);
			td_json_pub->add_member("投资单元代码", pOrder->InvestUnitID);
			td_json_pub->add_member("资金账号", pOrder->AccountID);
			td_json_pub->add_member("币种代码", pOrder->CurrencyID);
			td_json_pub->add_member("IP地址", pOrder->IPAddress);
			td_json_pub->add_member("Mac地址", pOrder->MacAddress);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryProduct(rj::Value& params)
	{
		const char* ProductID = params["ProductID"].GetString();

		CThostFtdcQryProductField QryProduct = { 0 };
		strncpy_s(QryProduct.ProductID, ProductID, sizeof(QryProduct.ProductID) - 1);
		return td_api->ReqQryProduct(&QryProduct, td_request_id++);
	}

	//--- 查询品种 -> 查询完毕信号
	void OnRspQryProduct(CThostFtdcProductField* pProduct, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pProduct)
		{
			td_json_pub->add_member("产品代码", pProduct->ProductID);
			td_json_pub->add_member("产品名称", pProduct->ProductName);
			td_json_pub->add_member("交易所代码", pProduct->ExchangeID);
			td_json_pub->add_member("产品类型", pProduct->ProductClass);
			td_json_pub->add_member("合约数量乘数", pProduct->VolumeMultiple);
			td_json_pub->add_member("最小变动价位", pProduct->PriceTick);
			td_json_pub->add_member("市价单最大下单量", pProduct->MaxMarketOrderVolume);
			td_json_pub->add_member("市价单最小下单量", pProduct->MinMarketOrderVolume);
			td_json_pub->add_member("限价单最大下单量", pProduct->MaxLimitOrderVolume);
			td_json_pub->add_member("限价单最小下单量", pProduct->MinLimitOrderVolume);
			td_json_pub->add_member("持仓类型", pProduct->PositionType);
			td_json_pub->add_member("持仓日期类型", pProduct->PositionDateType);
			td_json_pub->add_member("平仓处理类型", pProduct->CloseDealType);
			td_json_pub->add_member("交易币种类型", pProduct->TradeCurrencyID);
			td_json_pub->add_member("质押资金可用范围", pProduct->MortgageFundUseRange);
			td_json_pub->add_member("交易所产品代码", pProduct->ExchangeProductID);
			td_json_pub->add_member("合约基础商品乘数", pProduct->UnderlyingMultiple);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryTrade(rj::Value& params)
	{
		const char* InstrumentID = params["InstrumentID"].GetString();

		CThostFtdcQryTradeField Trade = { 0 };
		strncpy_s(Trade.InstrumentID, InstrumentID, sizeof(Trade.InstrumentID) - 1);
		strncpy_s(Trade.BrokerID, td_broker, sizeof(Trade.BrokerID) - 1);
		strncpy_s(Trade.InvestorID, td_user, sizeof(Trade.InvestorID) - 1);
		return td_api->ReqQryTrade(&Trade, td_request_id++);
	}

	void OnRspQryTrade(CThostFtdcTradeField* pTrade, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pTrade)
		{
			td_json_pub->add_member("经纪公司代码", pTrade->BrokerID);
			td_json_pub->add_member("投资者代码", pTrade->InvestorID);
			td_json_pub->add_member("合约代码", pTrade->InstrumentID);
			td_json_pub->add_member("报单引用", pTrade->OrderRef);
			td_json_pub->add_member("用户代码", pTrade->UserID);
			td_json_pub->add_member("交易所代码", pTrade->ExchangeID);
			td_json_pub->add_member("成交编号", pTrade->TradeID);
			td_json_pub->add_member("买卖方向", pTrade->Direction);
			td_json_pub->add_member("报单编号", pTrade->OrderSysID);
			td_json_pub->add_member("会员代码", pTrade->ParticipantID);
			td_json_pub->add_member("客户代码", pTrade->ClientID);
			td_json_pub->add_member("交易角色", pTrade->TradingRole);
			td_json_pub->add_member("合约在交易所的代码", pTrade->ExchangeInstID);
			td_json_pub->add_member("开平标志", pTrade->OffsetFlag);
			td_json_pub->add_member("投机套保标志", pTrade->HedgeFlag);
			td_json_pub->add_member("价格", pTrade->Price);
			td_json_pub->add_member("数量", pTrade->Volume);
			td_json_pub->add_member("成交时期", pTrade->TradeDate);
			td_json_pub->add_member("成交时间", pTrade->TradeTime);
			td_json_pub->add_member("成交类型", pTrade->TradeType);
			td_json_pub->add_member("成交价来源", pTrade->PriceSource);
			td_json_pub->add_member("交易所交易员代码", pTrade->TraderID);
			td_json_pub->add_member("本地报单编号", pTrade->OrderLocalID);
			td_json_pub->add_member("结算会员编号", pTrade->ClearingPartID);
			td_json_pub->add_member("业务单元", pTrade->BusinessUnit);
			td_json_pub->add_member("序号", pTrade->SequenceNo);
			td_json_pub->add_member("交易日", pTrade->TradingDay);
			td_json_pub->add_member("结算编号", pTrade->SettlementID);
			td_json_pub->add_member("经纪公司报单编号", pTrade->BrokerOrderSeq);
			td_json_pub->add_member("成交来源", pTrade->TradeSource);
			td_json_pub->add_member("投资单元代码", pTrade->InvestUnitID);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryTradingAccount(rj::Value& params)
	{
		CThostFtdcQryTradingAccountField TradingAccount = { 0 };
		strncpy_s(TradingAccount.BrokerID, td_broker, sizeof(TradingAccount.BrokerID) - 1);
		strncpy_s(TradingAccount.InvestorID, td_user, sizeof(TradingAccount.InvestorID) - 1);
		return td_api->ReqQryTradingAccount(&TradingAccount, td_request_id++);
	}

	void OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pTradingAccount)
		{
			td_json_pub->add_member("经纪公司代码", pTradingAccount->BrokerID);
			td_json_pub->add_member("投资者帐号", pTradingAccount->AccountID);
			td_json_pub->add_member("上次存款额", pTradingAccount->PreDeposit);
			td_json_pub->add_member("上次结算准备金", pTradingAccount->PreBalance);
			td_json_pub->add_member("上次占用的保证金", pTradingAccount->PreMargin);
			td_json_pub->add_member("利息基数", pTradingAccount->InterestBase);
			td_json_pub->add_member("利息收入", pTradingAccount->Interest);
			td_json_pub->add_member("入金金额", pTradingAccount->Deposit);
			td_json_pub->add_member("出金金额", pTradingAccount->Withdraw);
			td_json_pub->add_member("冻结的保证金", pTradingAccount->FrozenMargin);
			td_json_pub->add_member("冻结的资金", pTradingAccount->FrozenCash);
			td_json_pub->add_member("冻结的手续费", pTradingAccount->FrozenCommission);
			td_json_pub->add_member("当前保证金总额", pTradingAccount->CurrMargin);
			td_json_pub->add_member("资金差额", pTradingAccount->CashIn);
			td_json_pub->add_member("手续费", pTradingAccount->Commission);
			td_json_pub->add_member("平仓盈亏", pTradingAccount->CloseProfit);
			td_json_pub->add_member("持仓盈亏", pTradingAccount->PositionProfit);
			td_json_pub->add_member("期货结算准备金", pTradingAccount->Balance);
			td_json_pub->add_member("可用资金", pTradingAccount->Available);
			td_json_pub->add_member("可取资金", pTradingAccount->WithdrawQuota);
			td_json_pub->add_member("基本准备金", pTradingAccount->Reserve);
			td_json_pub->add_member("交易日", pTradingAccount->TradingDay);
			td_json_pub->add_member("结算编号", pTradingAccount->SettlementID);
			td_json_pub->add_member("交易所保证金", pTradingAccount->ExchangeMargin);
			td_json_pub->add_member("保底期货结算准备金", pTradingAccount->ReserveBalance);
			td_json_pub->add_member("业务类型", pTradingAccount->BizType);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryTradingCode(rj::Value& params)
	{
		const char* ExchangeID = params["ExchangeID"].GetString();

		CThostFtdcQryTradingCodeField TradingCode = { 0 };
		strncpy_s(TradingCode.ExchangeID, ExchangeID, sizeof(TradingCode.ExchangeID) - 1);
		strncpy_s(TradingCode.BrokerID, td_broker, sizeof(TradingCode.BrokerID) - 1);
		strncpy_s(TradingCode.InvestorID, td_user, sizeof(TradingCode.InvestorID) - 1);
		return td_api->ReqQryTradingCode(&TradingCode, td_request_id++);
	}

	void OnRspQryTradingCode(CThostFtdcTradingCodeField* pTradingCode, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pTradingCode)
		{
			td_json_pub->add_member("投资者代码", pTradingCode->InvestorID);
			td_json_pub->add_member("经纪公司代码", pTradingCode->BrokerID);
			td_json_pub->add_member("交易所代码", pTradingCode->ExchangeID);
			td_json_pub->add_member("客户代码", pTradingCode->ClientID);
			td_json_pub->add_member("是否活跃", pTradingCode->IsActive);
			td_json_pub->add_member("交易编码类型", pTradingCode->ClientIDType);
			td_json_pub->add_member("营业部编号", pTradingCode->BranchID);
			td_json_pub->add_member("业务类型", pTradingCode->BizType);
			td_json_pub->add_member("投资单元代码", pTradingCode->InvestUnitID);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}

	int ReqQryTradingNotice(rj::Value& params)
	{
		CThostFtdcQryTradingNoticeField TradingNotice = { 0 };
		strncpy_s(TradingNotice.BrokerID, td_broker, sizeof(TradingNotice.BrokerID) - 1);
		strncpy_s(TradingNotice.InvestorID, td_user, sizeof(TradingNotice.InvestorID) - 1);
		return td_api->ReqQryTradingNotice(&TradingNotice, td_request_id++);
	}

	void OnRspQryTradingNotice(CThostFtdcTradingNoticeField* pTradingNotice, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pTradingNotice)
		{
			td_json_pub->add_member("经纪公司代码", pTradingNotice->BrokerID);
			td_json_pub->add_member("投资者范围", pTradingNotice->InvestorRange);
			td_json_pub->add_member("投资者代码", pTradingNotice->InvestorID);
			td_json_pub->add_member("序列系列号", pTradingNotice->SequenceSeries);
			td_json_pub->add_member("用户代码", pTradingNotice->UserID);
			td_json_pub->add_member("发送时间", pTradingNotice->SendTime);
			td_json_pub->add_member("序列号", pTradingNotice->SequenceNo);
			td_json_pub->add_member("消息正文", pTradingNotice->FieldContent);
			td_json_pub->add_member("投资单元代码", pTradingNotice->InvestUnitID);
		}
		if (bIsLast) {
			//--- 查询完毕, 返回数据, 清空json, 发送信号
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}
};
