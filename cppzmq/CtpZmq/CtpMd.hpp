#include "Utility.hpp"


#define MDADDRPUB "tcp://*:5556"     // ���鶩�ĵ�ַ
#define MDADDRREP "tcp://*:5557"     // ���������ַ


class CCtpMd :CThostFtdcMdSpi
{
public:
	CThostFtdcMdApi* md_api;
	// ctp���
	const char*		md_host;		// �����ַ
	// zmq���
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

		// �½�socket
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
		// ��� -> �Ͽ� -> �ر�
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

	//--- ��¼
	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) 
	{
		if (pRspInfo && pRspInfo->ErrorID != 0)
		{
			printf("Login failed. %d - %s\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		}
		printf("�汾��:%s, ������:%s\n",md_api->GetApiVersion(), md_api->GetTradingDay());
		// �����ź�
		_semaphore.signal();
	}

	//--- ����
	bool MdBind()
	{
		// ��socket
		zmq_bind(md_socket_pub, MDADDRPUB);
		zmq_bind(md_socket_rep, MDADDRREP);

		md_api->RegisterFront((char*)md_host);
		md_api->Init();
		return(true);
	}

	bool MdResponse()
	{
		strcpy_s(md_send_pub, "����������:pub/sub");
		zmq_send(md_socket_pub, md_send_pub, sizeof(md_send_pub), 0);

		while (true)
		{
			_semaphore.wait();
			// ����ָ��
			zmq_recv(md_socket_rep, md_recv_rep, sizeof(md_recv_rep), 0);
			printf("recv:%s", md_recv_rep);
			strcpy_s(md_send_rep, "yes");
			zmq_send(md_socket_rep, md_send_rep, sizeof(md_send_rep), 0);

			// ����json����
			md_json_rep->m_doc.Parse(md_recv_rep);
			std::string func = md_json_rep->m_doc["func"].GetString();
			const char* params = md_json_rep->m_doc["params"].GetString();

			int req_result = 
				func == "SubscribeMarketData" ?	SubscribeMarketData(params) :
				printf("�����ڴ˺���");

			switch (req_result)
			{
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

	bool SubscribeMarketData(const char *params)
	{
		// ���ն�������
		std::string symbols=params;
		char* instruments[100]{ 0 };
		std::vector<std::string> result = split(symbols, ";");
		for (int i = 0; i < result.size(); i++)
		{
			instruments[i] = (char*)result[i].c_str();
			printf("����Ʒ��:%s \n", result[i].c_str());
		}
		md_api->SubscribeMarketData(instruments, result.size());

		return(true);
	}

	//--- ��������
	void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
	{
		md_json_pub->add_member("������", pDepthMarketData->TradingDay);
		md_json_pub->add_member("��Լ����", pDepthMarketData->InstrumentID);
		md_json_pub->add_member("����������", pDepthMarketData->ExchangeID);
		md_json_pub->add_member("��Լ�ڽ������Ĵ���", pDepthMarketData->ExchangeInstID);
		md_json_pub->add_member("���¼�", pDepthMarketData->LastPrice);
		md_json_pub->add_member("�ϴν����", pDepthMarketData->PreSettlementPrice);
		md_json_pub->add_member("������", pDepthMarketData->PreClosePrice);
		md_json_pub->add_member("��ֲ���", pDepthMarketData->PreOpenInterest);
		md_json_pub->add_member("����", pDepthMarketData->OpenPrice);
		md_json_pub->add_member("��߼�", pDepthMarketData->HighestPrice);
		md_json_pub->add_member("��ͼ�", pDepthMarketData->LowestPrice);
		md_json_pub->add_member("����", pDepthMarketData->Volume);
		md_json_pub->add_member("�ɽ����", pDepthMarketData->Turnover);
		md_json_pub->add_member("�ֲ���", pDepthMarketData->OpenInterest);
		md_json_pub->add_member("������", pDepthMarketData->ClosePrice);
		md_json_pub->add_member("���ν����", pDepthMarketData->SettlementPrice);
		md_json_pub->add_member("��ͣ���", pDepthMarketData->UpperLimitPrice);
		md_json_pub->add_member("��ͣ���", pDepthMarketData->LowerLimitPrice);
		md_json_pub->add_member("����޸�ʱ��", pDepthMarketData->UpdateTime);
		md_json_pub->add_member("����޸ĺ���", pDepthMarketData->UpdateMillisec);
		md_json_pub->add_member("�����һ", pDepthMarketData->BidPrice1);
		md_json_pub->add_member("������һ", pDepthMarketData->BidVolume1);
		md_json_pub->add_member("������һ", pDepthMarketData->AskPrice1);
		md_json_pub->add_member("������һ", pDepthMarketData->AskVolume1);
		md_json_pub->add_member("����۶�", pDepthMarketData->BidPrice2);
		md_json_pub->add_member("��������", pDepthMarketData->BidVolume2);
		md_json_pub->add_member("�����۶�", pDepthMarketData->AskPrice2);
		md_json_pub->add_member("��������", pDepthMarketData->AskVolume2);
		md_json_pub->add_member("�������", pDepthMarketData->BidPrice3);
		md_json_pub->add_member("��������", pDepthMarketData->BidVolume3);
		md_json_pub->add_member("��������", pDepthMarketData->AskPrice3);
		md_json_pub->add_member("��������", pDepthMarketData->AskVolume3);
		md_json_pub->add_member("�������", pDepthMarketData->BidPrice4);
		md_json_pub->add_member("��������", pDepthMarketData->BidVolume4);
		md_json_pub->add_member("��������", pDepthMarketData->AskPrice4);
		md_json_pub->add_member("��������", pDepthMarketData->AskVolume4);
		md_json_pub->add_member("�������", pDepthMarketData->BidPrice5);
		md_json_pub->add_member("��������", pDepthMarketData->BidVolume5);
		md_json_pub->add_member("��������", pDepthMarketData->AskPrice5);
		md_json_pub->add_member("��������", pDepthMarketData->AskVolume5);
		md_json_pub->add_member("���վ���", pDepthMarketData->AveragePrice);
		md_json_pub->add_member("ҵ������", pDepthMarketData->ActionDay);

		//--- ��������
		strcpy_s(md_send_pub,md_json_pub->get_string());
		zmq_send(md_socket_pub, md_send_pub, sizeof(md_send_pub), 0);
		printf("��������:%s\n", md_send_pub);
		md_json_pub->remove_members();
	}
};