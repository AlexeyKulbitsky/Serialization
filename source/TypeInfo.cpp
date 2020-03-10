#include "TypeInfo.h"

TypeInfoCollection& TypeInfoCollection::GetInstance()
{
	static TypeInfoCollection typeInfoCollection;
	return typeInfoCollection;
}

TypeInfo* TypeInfoCollection::GetTypeInfo(const std::string& typeName)
{
	auto findResult = m_typeInfos.find(typeName);
	assert(findResult != m_typeInfos.end());

	return &(findResult->second);
}