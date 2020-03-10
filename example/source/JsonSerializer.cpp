#include "JsonSerializer.h"

JsonSerializer::JsonSerializer(const std::string& filePath) : m_filePath(filePath)
{
	m_currentValue = &m_root;
}

void JsonSerializer::Clear()
{
	Serializer::Clear();
	Json::StreamWriterBuilder builder;
	const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
	writer->write(m_root, &std::cout);
}

void JsonSerializer::SerializePointers()
{
	Json::Value pointersValue;
	for (auto& it : m_pointersToSerialize)
	{
		Json::Value objectValue;
		auto valueAddress = it.first;
		auto typeInfo = it.second;
		auto auctualTypeInfo = typeInfo->pointerParams.getActualTypeInfo(valueAddress);

		objectValue["address"] = reinterpret_cast<uintptr_t>(valueAddress);
		objectValue["type"] = auctualTypeInfo->GetName();
		auto temp = m_currentValue;

		Json::Value value;
		m_currentValue = &value;

		SerializeByType(*auctualTypeInfo, valueAddress);
		objectValue["value"] = value;

		pointersValue.append(objectValue);

		m_serializedPointers.insert(valueAddress);
		m_currentValue = temp;
	}
	m_pointersToSerialize.clear();

	m_root["pointers"] = pointersValue;
}

void JsonSerializer::DeserializePointers()
{
	Json::Value pointersValue = m_root["pointers"];

	auto& typeInfoCollection = TypeInfoCollection::GetInstance();

	for (auto it = pointersValue.begin(); it != pointersValue.end(); ++it)
	{
		const bool hasAddress = it->isMember("address");
		const bool hasType = it->isMember("type");
		const bool hasValue = it->isMember("value");
		if (hasAddress && hasType)
		{
			const uintptr_t v = static_cast<uintptr_t>((*it)["address"].asUInt64());
			const auto typeName = (*it)["type"].asString();
			Json::Value value = (*it)["value"];

			auto findIt = m_pointersToDeserialize.find(v);
			if (findIt != m_pointersToDeserialize.end())
			{
				const auto& typeInfo = typeInfoCollection.GetTypeInfo(typeName);
				auto actualData = findIt->second;

				void* valueBuffer = nullptr;
				size_t bufferSize = 0U;

				auto temp = m_currentValue;
				m_currentValue = &value;
				typeInfo->createDefaultValue(valueBuffer, bufferSize);

				DeserializeByType(*typeInfo, valueBuffer);

				m_currentValue = temp;

				for (auto dataAddress : actualData)
				{
					*reinterpret_cast<void**>(dataAddress) = valueBuffer;
				}
			}
		}
	}
}

void JsonSerializer::SerializeInternal(const ObjectDesc& objectDesc, void* object)
{
	Json::Value objectValue;
	const auto& properties = objectDesc.GetProperties();

	auto currentRoot = m_currentValue;
	for (const auto prop : properties)
	{
		const auto& typeInfo = prop.second->GetTypeInfo();
		void* data = prop.second->GetValue(object);

		Json::Value propertyValue;
		m_currentValue = &propertyValue;

		SerializeByType(typeInfo, data);

		objectValue[prop.first] = propertyValue;
	}
	(*currentRoot)[objectDesc.GetName()] = objectValue;
	m_currentValue = currentRoot;
}

void JsonSerializer::DeserializeInternal(const ObjectDesc& objectDesc, void* object)
{
	auto currentRoot = m_currentValue;
	Json::Value child = (*currentRoot)[objectDesc.GetName()];

	const auto& properties = objectDesc.GetProperties();

	for (const auto prop : properties)
	{
		const auto& typeInfo = prop.second->GetTypeInfo();
		void* data = prop.second->GetValue(object);

		const bool isMember = child.isMember(prop.first);
		if (isMember)
		{
			Json::Value propertyValue = child[prop.first];
			m_currentValue = &propertyValue;

			DeserializeByType(typeInfo, data);

			prop.second->SetValue(object, data);
		}
	}
	m_currentValue = currentRoot;
}

void JsonSerializer::SerializeByType(const TypeInfo& typeInfo, void* data)
{
	switch (typeInfo.type)
	{
	case TypeInfo::Fundamental:
	case TypeInfo::Enum:
	{
		const auto typeIndex = typeInfo.fundamentalTypeParams.typeIndex;

		switch (typeIndex)
		{
		case BT_INT_32:
			*m_currentValue = *(reinterpret_cast<int*>(data));
			break;
		case BT_FLOAT:
			*m_currentValue = *(reinterpret_cast<float*>(data));
			break;
		case BT_DOUBLE:
			*m_currentValue = *(reinterpret_cast<double*>(data));
			break;
		default:
			*m_currentValue = "none";
			break;
		}
	}
	break;
	case TypeInfo::Class:
	{
		auto& factoryInstance = ObjectFactory::GetInstance();
		const auto id = typeInfo.GetObjectDescId();

		const auto& childObjectDesc = factoryInstance.GetObjectDesc(id);

		auto temp = m_currentValue;

		Json::Value objectValue;
		const auto& childProperties = childObjectDesc.GetProperties();

		for (const auto childProp : childProperties)
		{
			const auto& propertyTypeInfo = childProp.second->GetTypeInfo();
			void* childData = childProp.second->GetValue(data);

			Json::Value propertyValue;
			m_currentValue = &propertyValue;

			SerializeByType(propertyTypeInfo, childData);

			objectValue[childProp.first] = propertyValue;
		}

		(*temp) = objectValue;
	}
	break;
	case TypeInfo::String:
	{
		auto str = reinterpret_cast<std::string*>(data);
		*m_currentValue = (*str);
	}
	break;
	case TypeInfo::Pointer:
	{
		void* actualPtr = *reinterpret_cast<void**>(data);
		if (actualPtr != nullptr)
		{
			const bool isNotSerializedYet = m_serializedPointers.find(actualPtr) == m_serializedPointers.end();
			const bool isNotWaitingForSerialization = m_pointersToSerialize.find(actualPtr) == m_pointersToSerialize.end();
			if (isNotSerializedYet && isNotWaitingForSerialization)
			{
				auto underlyingTypeInfo = typeInfo.pointerParams.underlyingType;
				m_pointersToSerialize[actualPtr] = underlyingTypeInfo;
			}
			else
			{
				// Do nothing. Everything is ready
			}

			*m_currentValue = reinterpret_cast<uintptr_t>(actualPtr);
		}
	}
	break;
	case TypeInfo::Array:
	{
		size_t elementsCount = 0U;
		const auto& elementTypeInfo = typeInfo.arrayParams.elementTypeInfo;

		if (typeInfo.arrayParams.arrayType == TypeInfo::ArrayType::Vector)
		{
			elementsCount = typeInfo.arrayParams.getSize(data);
		}
		else
		{
			elementsCount = typeInfo.arrayParams.elementsCount;
		}

		auto temp = m_currentValue;
		for (size_t i = 0U; i < elementsCount; ++i)
		{
			Json::Value itemValue;
			m_currentValue = &itemValue;
			void* currentDataAddress = typeInfo.arrayParams.getItem(data, i);
			SerializeByType(*elementTypeInfo, currentDataAddress);
			temp->append(itemValue);
		}
		m_currentValue = temp;
	}
	break;
	case TypeInfo::Map:
	{
		auto it = typeInfo.mapParams.getIterator(data);
		auto temp = m_currentValue;
		while (typeInfo.mapParams.isIteratorValid(it, data))
		{
			Json::Value itemValue;
			Json::Value keyValue;
			Json::Value valueValue;

			m_currentValue = &keyValue;
			auto key = typeInfo.mapParams.getKey(it);
			SerializeByType(*typeInfo.mapParams.keyTypeInfo, (void*)key);

			m_currentValue = &valueValue;
			auto value = typeInfo.mapParams.getValue(it);
			SerializeByType(*typeInfo.mapParams.valueTypeInfo, value);

			typeInfo.mapParams.incrementIterator(it);

			itemValue["key"] = keyValue;
			itemValue["value"] = valueValue;
			temp->append(itemValue);
		}
		m_currentValue = temp;
	}
	break;
	default:
		break;
	}
}

void JsonSerializer::DeserializeByType(const TypeInfo& typeInfo, void* data)
{
	switch (typeInfo.type)
	{
	case TypeInfo::Fundamental:
	case TypeInfo::Enum:
	{
		const auto typeIndex = typeInfo.fundamentalTypeParams.typeIndex;

		switch (typeIndex)
		{
		case BT_INT_32:
			*(reinterpret_cast<int*>(data)) = m_currentValue->asInt();
			break;
		case BT_FLOAT:
			*(reinterpret_cast<float*>(data)) = m_currentValue->asFloat();
			break;
		case BT_DOUBLE:
			break;
		default:
			break;
		}
	}
	break;
	case TypeInfo::Class:
	{
		auto& factoryInstance = ObjectFactory::GetInstance();
		const auto id = typeInfo.GetObjectDescId();

		const auto& childObjectDesc = factoryInstance.GetObjectDesc(id);

		const auto& childProperties = childObjectDesc.GetProperties();

		auto child = m_currentValue;

		for (const auto childProp : childProperties)
		{
			const auto& childTypeInfo = childProp.second->GetTypeInfo();
			void* childData = childProp.second->GetValue(data);

			const bool isMember = (*child).isMember(childProp.first);
			if (isMember)
			{
				Json::Value propertyValue = (*child)[childProp.first];
				m_currentValue = &propertyValue;

				DeserializeByType(childTypeInfo, childData);

				childProp.second->SetValue(data, childData);
			}
		}
	}
	break;
	case TypeInfo::String:
	{
		auto str = reinterpret_cast<std::string*>(data);
		(*str) = m_currentValue->asString();
	}
	break;
	case TypeInfo::Pointer:
	{
		uintptr_t v = static_cast<uintptr_t>(m_currentValue->asUInt64());
		m_pointersToDeserialize[v].push_back(data);
	}
	break;
	case TypeInfo::Array:
	{
		const auto elementsCount = m_currentValue->size();
		const auto& underlyingTypeInfo = typeInfo.arrayParams.elementTypeInfo;

		if (typeInfo.arrayParams.arrayType == TypeInfo::ArrayType::Vector)
		{
			typeInfo.arrayParams.setSize(data, elementsCount);
		}

		auto temp = m_currentValue;
		for (size_t i = 0U; i < elementsCount; ++i)
		{
			Json::Value itemValue = (*temp)[i];
			m_currentValue = &itemValue;

			void* currentDataAddress = typeInfo.arrayParams.getItem(data, i);
			DeserializeByType(*underlyingTypeInfo, reinterpret_cast<void*>(currentDataAddress));
		}
		m_currentValue = temp;
	}
	break;
	case TypeInfo::Map:
	{
		const auto elementsCount = m_currentValue->size();

		void* keyBuffer = nullptr;
		void* valueBuffer = nullptr;
		size_t keyBufferSize = 0U;
		size_t valueBufferSize = 0U;

		typeInfo.mapParams.keyTypeInfo->createDefaultValue(keyBuffer, keyBufferSize);
		typeInfo.mapParams.valueTypeInfo->createDefaultValue(valueBuffer, valueBufferSize);

		auto& keyContainer = ObjectFactory::GetInstance().GetTempContainer(0U);
		auto& valueContainer = ObjectFactory::GetInstance().GetTempContainer(1U);

		auto temp = m_currentValue;
		for (size_t i = 0U; i < elementsCount; ++i)
		{
			Json::Value itemValue = (*temp)[i];

			m_currentValue = &itemValue["key"];
			DeserializeByType(*typeInfo.mapParams.keyTypeInfo, keyBuffer);

			m_currentValue = &itemValue["value"];
			DeserializeByType(*typeInfo.mapParams.valueTypeInfo, valueBuffer);

			typeInfo.mapParams.setKeyValue(data, keyBuffer, valueBuffer);
		}
		m_currentValue = temp;
		typeInfo.mapParams.keyTypeInfo->deleteValue(keyBuffer);
		typeInfo.mapParams.valueTypeInfo->deleteValue(valueBuffer);
	}
	break;
	default:
		break;
	}
}
