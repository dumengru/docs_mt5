#include "Utility.hpp"

#define TDADDRPUB "tcp://*:5554"     // ���׶��ĵ�ַ
#define TDADDRREP "tcp://*:5555"     // ���������ַ


class CtpTd : public CThostFtdcTraderSpi
{
public:

	CThostFtdcTraderApi* td_api;
	//--- ctp���
	const char*		td_host;
	const char*		td_broker;
	const char*		td_user;
	const char*		td_password;
	const char*		td_appid;
	const char*		td_authcode;
	// --- ����
	int				td_front_id;
	int				td_session_id;
	unsigned int	td_order_ref;
	unsigned int    td_request_id;

	//--- zmq���
	void*			td_context;
	void*			td_socket_pub;
	void*			td_socket_rep;
	char			td_send_pub[2048];
	char			td_send_rep[128];		// һ��ֻ�ظ�yes
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
		// ����Api, ��ע��Spi
		td_api = CThostFtdcTraderApi::CreateFtdcTraderApi();
		td_api->RegisterSpi(this);

		// �½�socket
		td_context = zmq_ctx_new();
		td_socket_pub = zmq_socket(td_context, ZMQ_PUB);
		td_socket_rep = zmq_socket(td_context, ZMQ_REP);

		// ��ʼ������
		memset(td_send_pub, 0x00, sizeof(td_send_pub));
		memset(td_send_rep, 0x00, sizeof(td_send_rep));
		memset(td_recv_rep, 0x00, sizeof(td_recv_rep));
		td_json_pub = new CRdJson();
		td_json_rep = new CRdJson();
	}

	~CtpTd()
	{
		td_api->Release();

		// ��� -> �Ͽ� -> �ر�
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

	//--- 1. Init -> �Զ��ص����� -> ��֤
	void OnFrontConnected()
	{
		printf("Connected.\n");
		// ��֤
		CThostFtdcReqAuthenticateField Req;
		memset(&Req, 0x00, sizeof(Req));
		strncpy_s(Req.BrokerID, td_broker, sizeof(Req.BrokerID) - 1);
		strncpy_s(Req.UserID, td_user, sizeof(Req.UserID) - 1);
		strncpy_s(Req.AuthCode, td_authcode, sizeof(Req.AuthCode) - 1);
		strncpy_s(Req.AppID, td_appid, sizeof(Req.AppID) - 1);
		td_api->ReqAuthenticate(&Req, td_request_id++);
	}
	//--- ���ӶϿ�
	void OnFrontDisconnected(int nReason)
	{
		printf("Disconnected.\n");
	}

	//--- 2. �ص���֤ -> ��¼
	void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo->ErrorID != 0)
			printf("OnRspAuthenticate:%s\n", pRspInfo->ErrorMsg);
		else
			printf("Authenticate succeeded.\n");

		// ��¼
		printf("Login...\n");
		CThostFtdcReqUserLoginField Req;

		memset(&Req, 0x00, sizeof(Req));
		strncpy_s(Req.BrokerID, td_broker, sizeof(Req.BrokerID) - 1);
		strncpy_s(Req.UserID, td_user, sizeof(Req.UserID) - 1);
		strncpy_s(Req.Password, td_password, sizeof(Req.Password) - 1);
		td_api->ReqUserLogin(&Req, td_request_id++);
	}

	//--- 3. �ص���¼ -> ȷ�Ͻ��㵥 -> ׼�������ź�
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("Login failed. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}
		printf("Login succeeded.TradingDay:%s,FrontID=%d,SessionID=%d\n", pRspUserLogin->TradingDay, pRspUserLogin->FrontID, pRspUserLogin->SessionID);

		// ��¼����Ψһ�ֶ�
		td_front_id = pRspUserLogin->FrontID;
		td_session_id = pRspUserLogin->SessionID;
		td_order_ref = 1;

		// ȷ�Ͻ��㵥
		CThostFtdcSettlementInfoConfirmField SettlementInfoConfirmField;
		memset(&SettlementInfoConfirmField, 0x00, sizeof(SettlementInfoConfirmField));
		strncpy_s(SettlementInfoConfirmField.BrokerID, pRspUserLogin->BrokerID, sizeof(SettlementInfoConfirmField.BrokerID) - 1);
		strncpy_s(SettlementInfoConfirmField.InvestorID, pRspUserLogin->UserID, sizeof(SettlementInfoConfirmField.InvestorID) - 1);
		strncpy_s(SettlementInfoConfirmField.ConfirmDate, pRspUserLogin->TradingDay, sizeof(SettlementInfoConfirmField.ConfirmDate) - 1);
		strncpy_s(SettlementInfoConfirmField.ConfirmTime, pRspUserLogin->LoginTime, sizeof(SettlementInfoConfirmField.ConfirmTime) - 1);
		td_api->ReqSettlementInfoConfirm(&SettlementInfoConfirmField, td_request_id++);

		// �����ź�
		_semaphore.signal();
	}

	//--- ����
	bool TdBind()
	{
		// ��zmq
		zmq_bind(td_socket_pub, TDADDRPUB);
		zmq_bind(td_socket_rep, TDADDRREP);

		// ע�ύ��ǰ��, ����˽����, ���Ĺ�����, ��ʼ��
		td_api->RegisterFront(const_cast<char*>(td_host));
		td_api->SubscribePrivateTopic(THOST_TERT_QUICK);
		td_api->SubscribePublicTopic(THOST_TERT_QUICK);
		td_api->Init();
		printf("�汾��:%s, ������:%s\n", td_api->GetApiVersion(), td_api->GetTradingDay());

		return(true);
	}

	bool TdResponse()
	{
		strcpy_s(td_send_pub, "����������:pub/sub");
		zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);

		while (true)
		{
			_semaphore.wait();
			// ����ָ��
			zmq_recv(td_socket_rep, td_recv_rep, sizeof(td_recv_rep), 0);
			printf("recv:%s\n", td_recv_rep);
			strcpy_s(td_send_rep, "yes");
			zmq_send(td_socket_rep, td_send_rep, sizeof(td_send_rep), 0);

			// ����json����
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

			// �ж�ִ������
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
				printf("�����ں���:%s", func.c_str());
				strcpy_s(td_send_pub, "func Error!");
				zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
				break;
			case(1):
				printf("��������ʧ��\n"); break;
			case(2):
				printf("�������󳬹������\n"); break;
			case(3):
				printf("ÿ�뷢�����󳬹������\n"); break;
			default:
				printf("CTP����ɹ�\n");
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

	// �µ�Ӧ��
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

	// ����Ӧ��
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

	//--- ��ѯ�����б�
	void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			printf("OnRspQryDepthMarketData. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}

		td_json_pub->add_member("������", pDepthMarketData->TradingDay);
		td_json_pub->add_member("��Լ����", pDepthMarketData->InstrumentID);
		td_json_pub->add_member("����������", pDepthMarketData->ExchangeID);
		td_json_pub->add_member("��Լ�ڽ������Ĵ���", pDepthMarketData->ExchangeInstID);
		td_json_pub->add_member("���¼�", double_format(pDepthMarketData->LastPrice));
		td_json_pub->add_member("�ϴν����", double_format(pDepthMarketData->PreSettlementPrice));
		td_json_pub->add_member("������", double_format(pDepthMarketData->PreClosePrice));
		td_json_pub->add_member("��ֲ���", double_format(pDepthMarketData->PreOpenInterest));
		td_json_pub->add_member("����", double_format(pDepthMarketData->OpenPrice));
		td_json_pub->add_member("��߼�", double_format(pDepthMarketData->HighestPrice));
		td_json_pub->add_member("��ͼ�", double_format(pDepthMarketData->LowestPrice));
		td_json_pub->add_member("����", pDepthMarketData->Volume);
		td_json_pub->add_member("�ɽ����", double_format(pDepthMarketData->Turnover));
		td_json_pub->add_member("�ֲ���", double_format(pDepthMarketData->OpenInterest));
		td_json_pub->add_member("������", double_format(pDepthMarketData->ClosePrice));
		td_json_pub->add_member("���ν����", double_format(pDepthMarketData->SettlementPrice));
		td_json_pub->add_member("��ͣ���", double_format(pDepthMarketData->UpperLimitPrice));
		td_json_pub->add_member("��ͣ���", double_format(pDepthMarketData->LowerLimitPrice));
		td_json_pub->add_member("����޸�ʱ��", pDepthMarketData->UpdateTime);
		td_json_pub->add_member("����޸ĺ���", pDepthMarketData->UpdateMillisec);

		td_json_pub->add_member("�����һ", pDepthMarketData->BidPrice1);
		td_json_pub->add_member("������һ", pDepthMarketData->BidVolume1);
		td_json_pub->add_member("������һ", pDepthMarketData->AskPrice1);
		td_json_pub->add_member("������һ", pDepthMarketData->AskVolume1);
		td_json_pub->add_member("����۶�", pDepthMarketData->BidPrice2);
		td_json_pub->add_member("��������", pDepthMarketData->BidVolume2);
		td_json_pub->add_member("�����۶�", pDepthMarketData->AskPrice2);
		td_json_pub->add_member("��������", pDepthMarketData->AskVolume2);
		td_json_pub->add_member("�������", pDepthMarketData->BidPrice3);
		td_json_pub->add_member("��������", pDepthMarketData->BidVolume3);
		td_json_pub->add_member("�������", pDepthMarketData->AskPrice3);
		td_json_pub->add_member("��������", pDepthMarketData->AskVolume3);
		td_json_pub->add_member("�������", pDepthMarketData->BidPrice4);
		td_json_pub->add_member("��������", pDepthMarketData->BidVolume4);
		td_json_pub->add_member("��������", pDepthMarketData->AskPrice4);
		td_json_pub->add_member("��������", pDepthMarketData->AskVolume4);
		td_json_pub->add_member("�������", pDepthMarketData->BidPrice5);
		td_json_pub->add_member("��������", pDepthMarketData->BidVolume5);
		td_json_pub->add_member("��������", pDepthMarketData->AskPrice5);
		td_json_pub->add_member("��������", pDepthMarketData->AskVolume5);

		td_json_pub->add_member("���վ���", pDepthMarketData->AveragePrice);
		td_json_pub->add_member("ҵ������", pDepthMarketData->ActionDay);

		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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

	//--- ��ѯ������ -> ��ѯ����ź�
	void OnRspQryExchange(CThostFtdcExchangeField* pExchange, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pExchange)
		{
			td_json_pub->add_member("����������", pExchange->ExchangeID);
			td_json_pub->add_member("����������", pExchange->ExchangeName);
			td_json_pub->add_member("����������", pExchange->ExchangeProperty);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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

	//--- �����ѯ��������֤������Ӧ
	void OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField* pExchangeMarginRate, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pExchangeMarginRate)
		{
			td_json_pub->add_member("���͹�˾����", pExchangeMarginRate->BrokerID);
			td_json_pub->add_member("��Լ����", pExchangeMarginRate->InstrumentID);
			td_json_pub->add_member("Ͷ���ױ���־", pExchangeMarginRate->HedgeFlag);
			td_json_pub->add_member("��ͷ��֤����", pExchangeMarginRate->LongMarginRatioByMoney);
			td_json_pub->add_member("��ͷ��֤���", pExchangeMarginRate->LongMarginRatioByVolume);
			td_json_pub->add_member("��ͷ��֤����", pExchangeMarginRate->ShortMarginRatioByMoney);
			td_json_pub->add_member("��ͷ��֤���", pExchangeMarginRate->ShortMarginRatioByVolume);
			td_json_pub->add_member("����������", pExchangeMarginRate->ExchangeID);
		}

		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
	//--- ��ѯ��Լ�б�
	void OnRspQryInstrument(CThostFtdcInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pInstrument)
		{
			td_json_pub->add_member("��Լ����", pInstrument->InstrumentID);
			td_json_pub->add_member("����������", pInstrument->ExchangeID);
			td_json_pub->add_member("��Լ����", pInstrument->InstrumentName);
			td_json_pub->add_member("��Լ�ڽ������Ĵ���", pInstrument->ExchangeInstID);
			td_json_pub->add_member("��Ʒ����", pInstrument->ProductID);
			td_json_pub->add_member("��Ʒ����", pInstrument->ProductClass);
			td_json_pub->add_member("�������", pInstrument->DeliveryYear);
			td_json_pub->add_member("������", pInstrument->DeliveryMonth);
			td_json_pub->add_member("�м۵�����µ���", pInstrument->MaxMarketOrderVolume);
			td_json_pub->add_member("�м۵���С�µ���", pInstrument->MinMarketOrderVolume);
			td_json_pub->add_member("�޼۵�����µ���", pInstrument->MaxLimitOrderVolume);
			td_json_pub->add_member("�޼۵���С�µ���", pInstrument->MinLimitOrderVolume);
			td_json_pub->add_member("��Լ��������", pInstrument->VolumeMultiple);
			td_json_pub->add_member("��С�䶯��λ", pInstrument->PriceTick);
			td_json_pub->add_member("������", pInstrument->CreateDate);
			td_json_pub->add_member("������", pInstrument->OpenDate);
			td_json_pub->add_member("������", pInstrument->ExpireDate);
			td_json_pub->add_member("��ʼ������", pInstrument->StartDelivDate);
			td_json_pub->add_member("����������", pInstrument->EndDelivDate);
			td_json_pub->add_member("��Լ��������״̬", pInstrument->InstLifePhase);
			td_json_pub->add_member("��ǰ�Ƿ���", pInstrument->IsTrading);
			td_json_pub->add_member("�ֲ�����", pInstrument->PositionType);
			td_json_pub->add_member("�ֲ���������", pInstrument->PositionDateType);
			td_json_pub->add_member("��ͷ��֤����", pInstrument->LongMarginRatio);
			td_json_pub->add_member("��ͷ��֤����", pInstrument->ShortMarginRatio);
			td_json_pub->add_member("�Ƿ�ʹ�ô��߱�֤���㷨", pInstrument->MaxMarginSideAlgorithm);
			td_json_pub->add_member("������Ʒ����", pInstrument->UnderlyingInstrID);
			td_json_pub->add_member("��Լ������Ʒ����", pInstrument->UnderlyingMultiple);
			td_json_pub->add_member("�������", pInstrument->CombinationType);
		}

		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("��Լ����",pInstrumentCommissionRate->InstrumentID);
			td_json_pub->add_member("Ͷ���߷�Χ", pInstrumentCommissionRate->InvestorRange);
			td_json_pub->add_member("���͹�˾����", pInstrumentCommissionRate->BrokerID);
			td_json_pub->add_member("Ͷ���ߴ���", pInstrumentCommissionRate->InvestorID);
			td_json_pub->add_member("������������", pInstrumentCommissionRate->OpenRatioByMoney);
			td_json_pub->add_member("����������", pInstrumentCommissionRate->OpenRatioByVolume);
			td_json_pub->add_member("ƽ����������", pInstrumentCommissionRate->CloseRatioByMoney);
			td_json_pub->add_member("ƽ��������", pInstrumentCommissionRate->CloseRatioByVolume);
			td_json_pub->add_member("ƽ����������", pInstrumentCommissionRate->CloseTodayRatioByMoney);
			td_json_pub->add_member("ƽ��������", pInstrumentCommissionRate->CloseTodayRatioByVolume);
			td_json_pub->add_member("����������", pInstrumentCommissionRate->ExchangeID);
			td_json_pub->add_member("ҵ������", pInstrumentCommissionRate->BizType);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pInstrumentCommissionRate->InvestUnitID);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("��Լ����", pInstrumentMarginRate->InstrumentID);
			td_json_pub->add_member("Ͷ���߷�Χ", pInstrumentMarginRate->InvestorRange);
			td_json_pub->add_member("���͹�˾����", pInstrumentMarginRate->BrokerID);
			td_json_pub->add_member("Ͷ���ߴ���", pInstrumentMarginRate->InvestorID);
			td_json_pub->add_member("Ͷ���ױ���־", pInstrumentMarginRate->HedgeFlag);
			td_json_pub->add_member("��ͷ��֤����", pInstrumentMarginRate->LongMarginRatioByMoney);
			td_json_pub->add_member("��ͷ��֤���", pInstrumentMarginRate->LongMarginRatioByVolume);
			td_json_pub->add_member("��ͷ��֤����", pInstrumentMarginRate->ShortMarginRatioByMoney);
			td_json_pub->add_member("��ͷ��֤���", pInstrumentMarginRate->ShortMarginRatioByVolume);
			td_json_pub->add_member("�Ƿ���Խ�������ȡ", pInstrumentMarginRate->IsRelative);
			td_json_pub->add_member("����������", pInstrumentMarginRate->ExchangeID);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pInstrumentMarginRate->InvestUnitID);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("��Լ����", pInstrumentOrderCommRate->InstrumentID);
			td_json_pub->add_member("Ͷ���߷�Χ", pInstrumentOrderCommRate->InvestorRange);
			td_json_pub->add_member("���͹�˾����", pInstrumentOrderCommRate->BrokerID);
			td_json_pub->add_member("Ͷ���ߴ���", pInstrumentOrderCommRate->InvestorID);
			td_json_pub->add_member("Ͷ���ױ���־", pInstrumentOrderCommRate->HedgeFlag);
			td_json_pub->add_member("����������", pInstrumentOrderCommRate->OrderCommByVolume);
			td_json_pub->add_member("����������", pInstrumentOrderCommRate->OrderActionCommByVolume);
			td_json_pub->add_member("����������", pInstrumentOrderCommRate->ExchangeID);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pInstrumentOrderCommRate->InvestUnitID);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("Ͷ���ߴ���", pInvestor->InvestorID);
			td_json_pub->add_member("���͹�˾����", pInvestor->BrokerID);
			td_json_pub->add_member("Ͷ���߷������", pInvestor->InvestorGroupID);
			td_json_pub->add_member("Ͷ��������", pInvestor->InvestorName);
			td_json_pub->add_member("֤������", pInvestor->IdentifiedCardType);
			td_json_pub->add_member("֤������", pInvestor->IdentifiedCardNo);
			td_json_pub->add_member("�Ƿ��Ծ", pInvestor->IsActive);
			td_json_pub->add_member("��ϵ�绰", pInvestor->Telephone);
			td_json_pub->add_member("ͨѶ��ַ", pInvestor->Address);
			td_json_pub->add_member("��������", pInvestor->OpenDate);
			td_json_pub->add_member("�ֻ�", pInvestor->Mobile);
			td_json_pub->add_member("��������ģ�����", pInvestor->CommModelID);
			td_json_pub->add_member("��֤����ģ�����", pInvestor->MarginModelID);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("��Լ����", pInvestorPosition->InstrumentID);
			td_json_pub->add_member("���͹�˾����", pInvestorPosition->BrokerID);
			td_json_pub->add_member("Ͷ���ߴ���", pInvestorPosition->InvestorID);
			td_json_pub->add_member("�ֲֶ�շ���", pInvestorPosition->PosiDirection);
			td_json_pub->add_member("Ͷ���ױ���־", pInvestorPosition->HedgeFlag);
			td_json_pub->add_member("�ֲ�����", pInvestorPosition->PositionDate);
			td_json_pub->add_member("���ճֲ�", pInvestorPosition->YdPosition);
			td_json_pub->add_member("���ճֲ�", pInvestorPosition->Position);
			td_json_pub->add_member("��ͷ����", pInvestorPosition->LongFrozen);
			td_json_pub->add_member("��ͷ����", pInvestorPosition->ShortFrozen);
			td_json_pub->add_member("���ֶ�����", pInvestorPosition->LongFrozenAmount);
			td_json_pub->add_member("���ֶ�����", pInvestorPosition->ShortFrozenAmount);
			td_json_pub->add_member("������", pInvestorPosition->OpenVolume);
			td_json_pub->add_member("ƽ����", pInvestorPosition->CloseVolume);
			td_json_pub->add_member("���ֽ��", pInvestorPosition->OpenAmount);
			td_json_pub->add_member("ƽ�ֽ��", pInvestorPosition->CloseAmount);
			td_json_pub->add_member("�ֲֳɱ�", pInvestorPosition->PositionCost);
			td_json_pub->add_member("�ϴ�ռ�õı�֤��", pInvestorPosition->PreMargin);
			td_json_pub->add_member("ռ�õı�֤��", pInvestorPosition->UseMargin);
			td_json_pub->add_member("����ı�֤��", pInvestorPosition->FrozenMargin);
			td_json_pub->add_member("������ʽ�", pInvestorPosition->FrozenCash);
			td_json_pub->add_member("�����������", pInvestorPosition->FrozenCommission);
			td_json_pub->add_member("�ʽ���", pInvestorPosition->CashIn);
			td_json_pub->add_member("������", pInvestorPosition->Commission);
			td_json_pub->add_member("ƽ��ӯ��", pInvestorPosition->CloseProfit);
			td_json_pub->add_member("�ֲ�ӯ��", pInvestorPosition->PositionProfit);
			td_json_pub->add_member("�ϴν����", pInvestorPosition->PreSettlementPrice);
			td_json_pub->add_member("���ν����", pInvestorPosition->SettlementPrice);
			td_json_pub->add_member("������", pInvestorPosition->TradingDay);
			td_json_pub->add_member("������", pInvestorPosition->SettlementID);
			td_json_pub->add_member("���ֳɱ�", pInvestorPosition->OpenCost);
			td_json_pub->add_member("��������֤��", pInvestorPosition->ExchangeMargin);
			td_json_pub->add_member("��ϳɽ��γɵĳֲ�", pInvestorPosition->CombPosition);
			td_json_pub->add_member("��϶�ͷ����", pInvestorPosition->CombLongFrozen);
			td_json_pub->add_member("��Ͽ�ͷ����", pInvestorPosition->CombShortFrozen);
			td_json_pub->add_member("���ն���ƽ��ӯ��", pInvestorPosition->CloseProfitByDate);
			td_json_pub->add_member("��ʶԳ�ƽ��ӯ��", pInvestorPosition->CloseProfitByTrade);
			td_json_pub->add_member("���ճֲ�", pInvestorPosition->TodayPosition);
			td_json_pub->add_member("��֤����", pInvestorPosition->MarginRateByMoney);
			td_json_pub->add_member("��֤����(������)", pInvestorPosition->MarginRateByVolume);
			td_json_pub->add_member("ִ�ж���", pInvestorPosition->StrikeFrozen);
			td_json_pub->add_member("ִ�ж�����", pInvestorPosition->StrikeFrozenAmount);
			td_json_pub->add_member("����ִ�ж���", pInvestorPosition->AbandonFrozen);
			td_json_pub->add_member("����������", pInvestorPosition->ExchangeID);
			td_json_pub->add_member("ִ�ж�������", pInvestorPosition->YdStrikeFrozen);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pInvestorPosition->InvestUnitID);
			td_json_pub->add_member("�������ֲֳɱ���ֵ", pInvestorPosition->PositionCostOffset);
			td_json_pub->add_member("tas�ֲ�����", pInvestorPosition->TasPosition);
			td_json_pub->add_member("tas�ֲֳɱ�", pInvestorPosition->TasPositionCost);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("������", pInvestorPositionCombineDetail->TradingDay);
			td_json_pub->add_member("��������", pInvestorPositionCombineDetail->OpenDate);
			td_json_pub->add_member("����������", pInvestorPositionCombineDetail->ExchangeID);
			td_json_pub->add_member("������", pInvestorPositionCombineDetail->SettlementID);
			td_json_pub->add_member("���͹�˾����", pInvestorPositionCombineDetail->BrokerID);
			td_json_pub->add_member("Ͷ���ߴ���", pInvestorPositionCombineDetail->InvestorID);
			td_json_pub->add_member("��ϱ��", pInvestorPositionCombineDetail->ComTradeID);
			td_json_pub->add_member("��ϱ��", pInvestorPositionCombineDetail->TradeID);
			td_json_pub->add_member("��Լ����", pInvestorPositionCombineDetail->InstrumentID);
			td_json_pub->add_member("Ͷ���ױ���־", pInvestorPositionCombineDetail->HedgeFlag);
			td_json_pub->add_member("����", pInvestorPositionCombineDetail->Direction);
			td_json_pub->add_member("�ֲ���", pInvestorPositionCombineDetail->TotalAmt);
			td_json_pub->add_member("Ͷ���߱�֤��", pInvestorPositionCombineDetail->Margin);
			td_json_pub->add_member("��������֤��", pInvestorPositionCombineDetail->ExchMargin);
			td_json_pub->add_member("��֤����", pInvestorPositionCombineDetail->MarginRateByMoney);
			td_json_pub->add_member("��֤����(������)", pInvestorPositionCombineDetail->MarginRateByVolume);
			td_json_pub->add_member("���ȱ��", pInvestorPositionCombineDetail->LegID);
			td_json_pub->add_member("���ȳ���", pInvestorPositionCombineDetail->LegMultiple);
			td_json_pub->add_member("��ϳֲֺ�Լ����", pInvestorPositionCombineDetail->CombInstrumentID);
			td_json_pub->add_member("�ɽ����", pInvestorPositionCombineDetail->TradeGroupID);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pInvestorPositionCombineDetail->InvestUnitID);
		}

		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("��Լ����", pInvestorPositionDetail->InstrumentID);
			td_json_pub->add_member("���͹�˾����", pInvestorPositionDetail->BrokerID);
			td_json_pub->add_member("Ͷ���ߴ���", pInvestorPositionDetail->InvestorID);
			td_json_pub->add_member("Ͷ���ױ���־", pInvestorPositionDetail->HedgeFlag);
			td_json_pub->add_member("����", pInvestorPositionDetail->Direction);
			td_json_pub->add_member("��������", pInvestorPositionDetail->OpenDate);
			td_json_pub->add_member("�ɽ����", pInvestorPositionDetail->TradeID);
			td_json_pub->add_member("����", pInvestorPositionDetail->Volume);
			td_json_pub->add_member("���ּ�", pInvestorPositionDetail->OpenPrice);
			td_json_pub->add_member("������", pInvestorPositionDetail->TradingDay);
			td_json_pub->add_member("������", pInvestorPositionDetail->SettlementID);
			td_json_pub->add_member("�ɽ�����", pInvestorPositionDetail->TradeType);
			td_json_pub->add_member("��Ϻ�Լ����", pInvestorPositionDetail->CombInstrumentID);
			td_json_pub->add_member("����������", pInvestorPositionDetail->ExchangeID);
			td_json_pub->add_member("���ն���ƽ��ӯ��", pInvestorPositionDetail->CloseProfitByDate);
			td_json_pub->add_member("��ʶԳ�ƽ��ӯ��", pInvestorPositionDetail->CloseProfitByTrade);
			td_json_pub->add_member("���ն��гֲ�ӯ��", pInvestorPositionDetail->PositionProfitByDate);
			td_json_pub->add_member("��ʶԳ�ֲ�ӯ��", pInvestorPositionDetail->PositionProfitByTrade);
			td_json_pub->add_member("Ͷ���߱�֤��", pInvestorPositionDetail->Margin);
			td_json_pub->add_member("��������֤��", pInvestorPositionDetail->ExchMargin);
			td_json_pub->add_member("��֤����", pInvestorPositionDetail->MarginRateByMoney);
			td_json_pub->add_member("��֤����(������)", pInvestorPositionDetail->MarginRateByVolume);
			td_json_pub->add_member("������", pInvestorPositionDetail->LastSettlementPrice);
			td_json_pub->add_member("�����", pInvestorPositionDetail->SettlementPrice);
			td_json_pub->add_member("ƽ����", pInvestorPositionDetail->CloseVolume);
			td_json_pub->add_member("ƽ�ֽ��", pInvestorPositionDetail->CloseAmount);
			td_json_pub->add_member("�ȿ���ƽʣ������", pInvestorPositionDetail->TimeFirstVolume);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pInvestorPositionDetail->InvestUnitID);
			td_json_pub->add_member("����ֱֲ�־", pInvestorPositionDetail->SpecPosiType);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("���͹�˾����", pInvestUnit->BrokerID);
			td_json_pub->add_member("Ͷ���ߴ���", pInvestUnit->InvestorID);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pInvestUnit->InvestUnitID);
			td_json_pub->add_member("Ͷ���ߵ�Ԫ����", pInvestUnit->InvestorUnitName);
			td_json_pub->add_member("Ͷ���߷������", pInvestUnit->InvestorGroupID);
			td_json_pub->add_member("��������ģ�����", pInvestUnit->CommModelID);
			td_json_pub->add_member("��֤����ģ�����", pInvestUnit->MarginModelID);
			td_json_pub->add_member("�ʽ��˺�", pInvestUnit->AccountID);
			td_json_pub->add_member("���ִ���", pInvestUnit->CurrencyID);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("���͹�˾����", pOrder->BrokerID);
			td_json_pub->add_member("Ͷ���ߴ���", pOrder->InvestorID);
			td_json_pub->add_member("��Լ����", pOrder->InstrumentID);
			td_json_pub->add_member("��������", pOrder->OrderRef);
			td_json_pub->add_member("�û�����", pOrder->UserID);
			td_json_pub->add_member("�����۸�����", pOrder->OrderPriceType);
			td_json_pub->add_member("��������", pOrder->Direction);
			td_json_pub->add_member("��Ͽ�ƽ��־", pOrder->CombOffsetFlag);
			td_json_pub->add_member("���Ͷ���ױ���־", pOrder->CombHedgeFlag);
			td_json_pub->add_member("�۸�", pOrder->LimitPrice);
			td_json_pub->add_member("����", pOrder->VolumeTotalOriginal);
			td_json_pub->add_member("��Ч������", pOrder->TimeCondition);
			td_json_pub->add_member("GTD����", pOrder->GTDDate);
			td_json_pub->add_member("�ɽ�������", pOrder->VolumeCondition);
			td_json_pub->add_member("��С�ɽ���", pOrder->MinVolume);
			td_json_pub->add_member("��������", pOrder->ContingentCondition);
			td_json_pub->add_member("ֹ���", pOrder->StopPrice);
			td_json_pub->add_member("ǿƽԭ��", pOrder->ForceCloseReason);
			td_json_pub->add_member("�Զ������־", pOrder->IsAutoSuspend);
			td_json_pub->add_member("ҵ��Ԫ", pOrder->BusinessUnit);
			td_json_pub->add_member("������", pOrder->RequestID);
			td_json_pub->add_member("���ر������", pOrder->OrderLocalID);
			td_json_pub->add_member("����������", pOrder->ExchangeID);
			td_json_pub->add_member("��Ա����", pOrder->ParticipantID);
			td_json_pub->add_member("�ͻ�����", pOrder->ClientID);
			td_json_pub->add_member("��Լ�ڽ������Ĵ���", pOrder->ExchangeInstID);
			td_json_pub->add_member("����������Ա����", pOrder->TraderID);
			td_json_pub->add_member("��װ���", pOrder->InstallID);
			td_json_pub->add_member("�����ύ״̬", pOrder->OrderSubmitStatus);
			td_json_pub->add_member("������ʾ���", pOrder->NotifySequence);
			td_json_pub->add_member("������", pOrder->TradingDay);
			td_json_pub->add_member("������", pOrder->SettlementID);
			td_json_pub->add_member("�������", pOrder->OrderSysID);
			td_json_pub->add_member("������Դ", pOrder->OrderSource);
			td_json_pub->add_member("����״̬", pOrder->OrderStatus);
			td_json_pub->add_member("��������", pOrder->OrderType);
			td_json_pub->add_member("��ɽ�����", pOrder->VolumeTraded);
			td_json_pub->add_member("ʣ������", pOrder->VolumeTotal);
			td_json_pub->add_member("��������", pOrder->InsertDate);
			td_json_pub->add_member("ί��ʱ��", pOrder->InsertTime);
			td_json_pub->add_member("����ʱ��", pOrder->ActiveTime);
			td_json_pub->add_member("����ʱ��", pOrder->SuspendTime);
			td_json_pub->add_member("����޸�ʱ��", pOrder->UpdateTime);
			td_json_pub->add_member("����ʱ��", pOrder->CancelTime);
			td_json_pub->add_member("����޸Ľ���������Ա����", pOrder->ActiveTraderID);
			td_json_pub->add_member("�����Ա���", pOrder->ClearingPartID);
			td_json_pub->add_member("���", pOrder->SequenceNo);
			td_json_pub->add_member("ǰ�ñ��", pOrder->FrontID);
			td_json_pub->add_member("�Ự���", pOrder->SessionID);
			td_json_pub->add_member("�û��˲�Ʒ��Ϣ", pOrder->UserProductInfo);
			td_json_pub->add_member("״̬��Ϣ", pOrder->StatusMsg);
			td_json_pub->add_member("�û�ǿ����־", pOrder->UserForceClose);
			td_json_pub->add_member("�����û�����", pOrder->ActiveUserID);
			td_json_pub->add_member("���͹�˾�������", pOrder->BrokerOrderSeq);
			td_json_pub->add_member("��ر���", pOrder->RelativeOrderSysID);
			td_json_pub->add_member("֣�����ɽ�����", pOrder->ZCETotalTradedVolume);
			td_json_pub->add_member("��������־", pOrder->IsSwapOrder);
			td_json_pub->add_member("Ӫҵ�����", pOrder->BranchID);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pOrder->InvestUnitID);
			td_json_pub->add_member("�ʽ��˺�", pOrder->AccountID);
			td_json_pub->add_member("���ִ���", pOrder->CurrencyID);
			td_json_pub->add_member("IP��ַ", pOrder->IPAddress);
			td_json_pub->add_member("Mac��ַ", pOrder->MacAddress);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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

	//--- ��ѯƷ�� -> ��ѯ����ź�
	void OnRspQryProduct(CThostFtdcProductField* pProduct, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pProduct)
		{
			td_json_pub->add_member("��Ʒ����", pProduct->ProductID);
			td_json_pub->add_member("��Ʒ����", pProduct->ProductName);
			td_json_pub->add_member("����������", pProduct->ExchangeID);
			td_json_pub->add_member("��Ʒ����", pProduct->ProductClass);
			td_json_pub->add_member("��Լ��������", pProduct->VolumeMultiple);
			td_json_pub->add_member("��С�䶯��λ", pProduct->PriceTick);
			td_json_pub->add_member("�м۵�����µ���", pProduct->MaxMarketOrderVolume);
			td_json_pub->add_member("�м۵���С�µ���", pProduct->MinMarketOrderVolume);
			td_json_pub->add_member("�޼۵�����µ���", pProduct->MaxLimitOrderVolume);
			td_json_pub->add_member("�޼۵���С�µ���", pProduct->MinLimitOrderVolume);
			td_json_pub->add_member("�ֲ�����", pProduct->PositionType);
			td_json_pub->add_member("�ֲ���������", pProduct->PositionDateType);
			td_json_pub->add_member("ƽ�ִ�������", pProduct->CloseDealType);
			td_json_pub->add_member("���ױ�������", pProduct->TradeCurrencyID);
			td_json_pub->add_member("��Ѻ�ʽ���÷�Χ", pProduct->MortgageFundUseRange);
			td_json_pub->add_member("��������Ʒ����", pProduct->ExchangeProductID);
			td_json_pub->add_member("��Լ������Ʒ����", pProduct->UnderlyingMultiple);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("���͹�˾����", pTrade->BrokerID);
			td_json_pub->add_member("Ͷ���ߴ���", pTrade->InvestorID);
			td_json_pub->add_member("��Լ����", pTrade->InstrumentID);
			td_json_pub->add_member("��������", pTrade->OrderRef);
			td_json_pub->add_member("�û�����", pTrade->UserID);
			td_json_pub->add_member("����������", pTrade->ExchangeID);
			td_json_pub->add_member("�ɽ����", pTrade->TradeID);
			td_json_pub->add_member("��������", pTrade->Direction);
			td_json_pub->add_member("�������", pTrade->OrderSysID);
			td_json_pub->add_member("��Ա����", pTrade->ParticipantID);
			td_json_pub->add_member("�ͻ�����", pTrade->ClientID);
			td_json_pub->add_member("���׽�ɫ", pTrade->TradingRole);
			td_json_pub->add_member("��Լ�ڽ������Ĵ���", pTrade->ExchangeInstID);
			td_json_pub->add_member("��ƽ��־", pTrade->OffsetFlag);
			td_json_pub->add_member("Ͷ���ױ���־", pTrade->HedgeFlag);
			td_json_pub->add_member("�۸�", pTrade->Price);
			td_json_pub->add_member("����", pTrade->Volume);
			td_json_pub->add_member("�ɽ�ʱ��", pTrade->TradeDate);
			td_json_pub->add_member("�ɽ�ʱ��", pTrade->TradeTime);
			td_json_pub->add_member("�ɽ�����", pTrade->TradeType);
			td_json_pub->add_member("�ɽ�����Դ", pTrade->PriceSource);
			td_json_pub->add_member("����������Ա����", pTrade->TraderID);
			td_json_pub->add_member("���ر������", pTrade->OrderLocalID);
			td_json_pub->add_member("�����Ա���", pTrade->ClearingPartID);
			td_json_pub->add_member("ҵ��Ԫ", pTrade->BusinessUnit);
			td_json_pub->add_member("���", pTrade->SequenceNo);
			td_json_pub->add_member("������", pTrade->TradingDay);
			td_json_pub->add_member("������", pTrade->SettlementID);
			td_json_pub->add_member("���͹�˾�������", pTrade->BrokerOrderSeq);
			td_json_pub->add_member("�ɽ���Դ", pTrade->TradeSource);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pTrade->InvestUnitID);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("���͹�˾����", pTradingAccount->BrokerID);
			td_json_pub->add_member("Ͷ�����ʺ�", pTradingAccount->AccountID);
			td_json_pub->add_member("�ϴδ���", pTradingAccount->PreDeposit);
			td_json_pub->add_member("�ϴν���׼����", pTradingAccount->PreBalance);
			td_json_pub->add_member("�ϴ�ռ�õı�֤��", pTradingAccount->PreMargin);
			td_json_pub->add_member("��Ϣ����", pTradingAccount->InterestBase);
			td_json_pub->add_member("��Ϣ����", pTradingAccount->Interest);
			td_json_pub->add_member("�����", pTradingAccount->Deposit);
			td_json_pub->add_member("������", pTradingAccount->Withdraw);
			td_json_pub->add_member("����ı�֤��", pTradingAccount->FrozenMargin);
			td_json_pub->add_member("������ʽ�", pTradingAccount->FrozenCash);
			td_json_pub->add_member("�����������", pTradingAccount->FrozenCommission);
			td_json_pub->add_member("��ǰ��֤���ܶ�", pTradingAccount->CurrMargin);
			td_json_pub->add_member("�ʽ���", pTradingAccount->CashIn);
			td_json_pub->add_member("������", pTradingAccount->Commission);
			td_json_pub->add_member("ƽ��ӯ��", pTradingAccount->CloseProfit);
			td_json_pub->add_member("�ֲ�ӯ��", pTradingAccount->PositionProfit);
			td_json_pub->add_member("�ڻ�����׼����", pTradingAccount->Balance);
			td_json_pub->add_member("�����ʽ�", pTradingAccount->Available);
			td_json_pub->add_member("��ȡ�ʽ�", pTradingAccount->WithdrawQuota);
			td_json_pub->add_member("����׼����", pTradingAccount->Reserve);
			td_json_pub->add_member("������", pTradingAccount->TradingDay);
			td_json_pub->add_member("������", pTradingAccount->SettlementID);
			td_json_pub->add_member("��������֤��", pTradingAccount->ExchangeMargin);
			td_json_pub->add_member("�����ڻ�����׼����", pTradingAccount->ReserveBalance);
			td_json_pub->add_member("ҵ������", pTradingAccount->BizType);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("Ͷ���ߴ���", pTradingCode->InvestorID);
			td_json_pub->add_member("���͹�˾����", pTradingCode->BrokerID);
			td_json_pub->add_member("����������", pTradingCode->ExchangeID);
			td_json_pub->add_member("�ͻ�����", pTradingCode->ClientID);
			td_json_pub->add_member("�Ƿ��Ծ", pTradingCode->IsActive);
			td_json_pub->add_member("���ױ�������", pTradingCode->ClientIDType);
			td_json_pub->add_member("Ӫҵ�����", pTradingCode->BranchID);
			td_json_pub->add_member("ҵ������", pTradingCode->BizType);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pTradingCode->InvestUnitID);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
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
			td_json_pub->add_member("���͹�˾����", pTradingNotice->BrokerID);
			td_json_pub->add_member("Ͷ���߷�Χ", pTradingNotice->InvestorRange);
			td_json_pub->add_member("Ͷ���ߴ���", pTradingNotice->InvestorID);
			td_json_pub->add_member("����ϵ�к�", pTradingNotice->SequenceSeries);
			td_json_pub->add_member("�û�����", pTradingNotice->UserID);
			td_json_pub->add_member("����ʱ��", pTradingNotice->SendTime);
			td_json_pub->add_member("���к�", pTradingNotice->SequenceNo);
			td_json_pub->add_member("��Ϣ����", pTradingNotice->FieldContent);
			td_json_pub->add_member("Ͷ�ʵ�Ԫ����", pTradingNotice->InvestUnitID);
		}
		if (bIsLast) {
			//--- ��ѯ���, ��������, ���json, �����ź�
			strcpy_s(td_send_pub, td_json_pub->get_string());
			zmq_send(td_socket_pub, td_send_pub, sizeof(td_send_pub), 0);
			td_json_pub->remove_members();
			_semaphore.signal();
		}
	}
};
