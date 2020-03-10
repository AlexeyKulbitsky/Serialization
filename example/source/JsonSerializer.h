#ifndef JSON_SERIALIZER_INCLUDE
#define JSON_SERIALIZER_INCLUDE

#include "Serializer.h"

#include <json/json.h>

class JsonSerializer : public Serializer
{
public:
	JsonSerializer(const std::string& filePath);

	void Clear() override;

private:
	std::string m_filePath;
	Json::Value m_root;
	Json::Value* m_currentValue;

public:
	void SerializePointers() override;
	void DeserializePointers() override;

protected:
	void SerializeInternal(const ObjectDesc& objectDesc, void* object) override final;
	void DeserializeInternal(const ObjectDesc& objectDesc, void* object) override final;

	void SerializeByType(const TypeInfo& typeInfo, void* data);
	void DeserializeByType(const TypeInfo& typeInfo, void* data);
};

#endif