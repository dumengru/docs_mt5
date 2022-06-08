#include "CtpMd.hpp"
#include "CtpTd.hpp"


bool start_trade()
{
	std::string host("tcp://122.51.136.165:20002"),broker(""),user("1493"),password("123456"),appid(""),authcode("");

	CtpTd CtpTd(host.c_str(), broker.c_str(), user.c_str(), password.c_str(), appid.c_str(), authcode.c_str());
	CtpTd.TdBind();
	CtpTd.TdResponse();

	return(true);
}

bool start_md()
{
	const char* host = "tcp://122.51.136.165:20004";
	
	CCtpMd CtpMd(host);
	CtpMd.MdBind();
	CtpMd.MdResponse();
	return(true);
}

int main()
{
	start_trade();
	// start_md();
	return(1);
}