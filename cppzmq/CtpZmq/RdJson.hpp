#include "document.h"
#include "stringbuffer.h"
#include "writer.h"


//--- 定义全局json对象
namespace rj = rapidjson;


class CRdJson
{
public:
	rj::Document m_doc;
	rj::Document::AllocatorType m_allocator;

	rj::StringBuffer  m_buffer;
	rj::Writer<rj::StringBuffer> m_writer;

	std::string	charstr;		// 将char转为string

public:
	CRdJson();
	~CRdJson();

	bool add_member(const char* key, char value);
	bool add_member(const char* key, const char* value);
	bool add_member(const char* key, const int value);
	bool add_member(const char* key, const unsigned value);
	bool add_member(const char* key, const int64_t value);
	bool add_member(const char* key, const uint64_t value);
	bool add_member(const char* key, const double value);
	bool add_member(const char* key, const float value);

	bool remove_members();
	const char* get_string();
};

CRdJson::CRdJson()
{
	m_doc.SetObject();
}

CRdJson::~CRdJson()
{

}

inline bool CRdJson::add_member(const char* key, char value)
{
	charstr = value;
	add_member(key, charstr.c_str());
	return true;
}

inline bool CRdJson::add_member(const char* key, const char* value)
{
	m_doc.AddMember(rj::StringRef(key), rj::Value().SetString(value, m_allocator).Move(), m_allocator);
	return true;
}

inline bool CRdJson::add_member(const char* key, const int value)
{
	m_doc.AddMember(rj::StringRef(key), rj::Value().SetInt(value).Move(), m_allocator);
	return true;
}

inline bool CRdJson::add_member(const char* key, const unsigned value)
{
	m_doc.AddMember(rj::StringRef(key), rj::Value().SetUint(value).Move(), m_allocator);
	return true;
}

inline bool CRdJson::add_member(const char* key, const int64_t value)
{
	m_doc.AddMember(rj::StringRef(key), rj::Value().SetInt64(value).Move(), m_allocator);
	return true;
}

inline bool CRdJson::add_member(const char* key, const uint64_t value)
{
	m_doc.AddMember(rj::StringRef(key), rj::Value().SetUint64(value).Move(), m_allocator);
	return true;
}

inline bool CRdJson::add_member(const char* key, const double value)
{
	m_doc.AddMember(rj::StringRef(key), rj::Value().SetDouble(value).Move(), m_allocator);
	return true;
}

inline bool CRdJson::add_member(const char* key, const float value)
{
	m_doc.AddMember(rj::StringRef(key), rj::Value().SetFloat(value).Move(), m_allocator);
	return true;
}

inline bool CRdJson::remove_members()
{
	m_doc.RemoveAllMembers();

	return false;
}

inline const char* CRdJson::get_string()
{
	m_buffer.Clear();
	m_writer.Reset(m_buffer);


	m_doc.Accept(m_writer);
	//转化为string
	return(m_buffer.GetString());
}

