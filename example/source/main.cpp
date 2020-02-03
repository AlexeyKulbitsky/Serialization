#include "ObjectFactory.h"
#include "Serializer.h"
#include "TypeInfo.h"

#include <iostream>

#include <json/json.h>


class JsonSerializer : public Serializer
{
public:
	JsonSerializer(const std::string& filePath) : m_filePath(filePath) 
	{
		m_currentValue = &m_root;
	}

	void Clear()
	{
		Json::StreamWriterBuilder builder;
		const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
		writer->write(m_root, &std::cout);
	}

private:
	std::string m_filePath;
	Json::Value m_root;
	Json::Value* m_currentValue;

public:
	void SerializePointers() override
	{
		Json::Value pointersValue;
		for (auto& it : m_pointersToSerialize)
		{
			Json::Value objectValue;
			auto valueAddress = it.first;
			auto typeInfo = it.second;

			objectValue["address"] = reinterpret_cast<uintptr_t>(valueAddress);
			objectValue["type"] = typeInfo->GetName();
			auto temp = m_currentValue;

			Json::Value value;
			m_currentValue = &value;

			SerializeByType(*typeInfo, valueAddress);
			objectValue["value"] = value;

			pointersValue.append(objectValue);

			m_currentValue = temp;
		}
		m_pointersToSerialize.clear();

		m_root["pointers"] = pointersValue;
	}

	void DeserializePointers() override
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
				const uintptr_t v = (*it)["address"].asUInt64();
				const auto typeName = (*it)["type"].asString();
				Json::Value value = (*it)["value"];

				auto findIt = m_pointersToDeserialize.find(v);
				if (findIt != m_pointersToDeserialize.end())
				{
					auto typeInfo = typeInfoCollection.GetTypeInfo(typeName);
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

					typeInfo->deleteValue(valueBuffer);
				}
			}
		}
	}

protected:
	void SerializeInternal(const ObjectDesc& objectDesc, void* object) override final
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

	void DeserializeInternal(const ObjectDesc& objectDesc, void* object) override final
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

	void SerializeByType(const TypeInfo& typeInfo, void* data)
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
				if (isNotSerializedYet)
				{
					auto underlyingTypeInfo = typeInfo.underlyingType;
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

	void DeserializeByType(const TypeInfo& typeInfo, void* data)
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
			uintptr_t v = m_currentValue->asUInt64();
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
};

struct Vec2
{
	float x = 0.0f;
	float y = 0.0f;
};

struct Vec3
{
    float x = 0.0f;
    float y = 1.0f;
    float z = 2.3f;
};

enum class MyEnum
{
	First,
	Second,
	Third
};

class BaseClass
{
public:
	int a = 0;
	float b = 0.0f;
};

class DerivedClass : public BaseClass
{
public:
	float c = 0.0f;
};

struct TestStruct
{
	int intValue = 5;

	int GetValue() const { return intValue; }
	void SetValue(const int val) { intValue = val; }
	float floatValue = 7.0f;
	int someArray[5] = { 1, 4, 5, 8, 9 };
	int someMatrix[3][3] = { {1, 2, 3}, {3, 3, 3}, {6, 6, 6} };
	std::vector<int> someVector;
	std::array<int, 3> someStaticArray;
	std::map<std::string, float> someMap;
	Vec3 vec3;
	Vec2* vec2 = nullptr;
	MyEnum someEnum;
	std::string someString;

	Vec3 privateVec3;
	const Vec3 GetVec3() const { return privateVec3; }
	void SetVec3(const Vec3& value) { privateVec3 = value; }
};

struct TestStruct2
{
	int intVal = 1;
	Vec2* vec2 = nullptr;
};

struct TestStruct3
{
	float floatVal = 9.0f;
	Vec2* vec2 = nullptr;
};

int main()
{
	class_<Vec2>("Vec2")
		.AddProperty("x", &Vec2::x)
		.AddProperty("y", &Vec2::y);

	class_<Vec3>("Vec3")
		.AddProperty("x", &Vec3::x)
		.AddProperty("y", &Vec3::y)
		.AddProperty("z", &Vec3::z);

	class_<TestStruct>("TestStruct")
		.AddProperty("intValue", &TestStruct::GetValue, &TestStruct::SetValue)
		.AddProperty("floatValue", &TestStruct::floatValue)
		.AddProperty("someArray", &TestStruct::someArray)
		.AddProperty("someStaticArray", &TestStruct::someStaticArray)
		.AddProperty("someMatrix", &TestStruct::someMatrix)
		.AddProperty("someVector", &TestStruct::someVector)
		.AddProperty("vec3", &TestStruct::vec3)
		.AddProperty("someMap", &TestStruct::someMap)
		.AddProperty("vec2", &TestStruct::vec2)
		.AddProperty("someEnum", &TestStruct::someEnum)
		.AddProperty("someString", &TestStruct::someString)
		.AddProperty("Vec3Accessor", &TestStruct::GetVec3, &TestStruct::SetVec3)
		;

	class_<TestStruct2>("TestStruct2")
		.AddProperty("intValue", &TestStruct2::intVal)
		.AddProperty("vec2", &TestStruct2::vec2)
		;

	class_<TestStruct3>("TestStruct3")
		.AddProperty("floatValue", &TestStruct3::floatVal)
		.AddProperty("vec2", &TestStruct3::vec2)
		;

	class_<BaseClass>("Base")
		.AddProperty("a", &BaseClass::a)
		.AddProperty("b", &BaseClass::b)
		;

	class_<DerivedClass, BaseClass>("Derived")
		.AddProperty("c", &DerivedClass::c)
		;

	Vec2* vec2 = new Vec2();
	vec2->x = 555.7f;
	vec2->y = 6.2f;

	TestStruct objectToSerialize_1, objectToSerialize_2;
	objectToSerialize_1.intValue = 7;
	objectToSerialize_1.floatValue = 23.5f;
	objectToSerialize_1.someStaticArray[1] = 100;
	objectToSerialize_1.someArray[3] = 900;
	objectToSerialize_1.someArray[4] = 7788;
	objectToSerialize_1.someMatrix[1][1] = 333333;
	objectToSerialize_1.someMap["One"] = 3.0f;
	objectToSerialize_1.someMap["BBBB"] = 444.4f;
	objectToSerialize_1.someEnum = MyEnum::Second;
	objectToSerialize_1.someString = "AAA";
	objectToSerialize_1.vec3.z = 111.555f;
	objectToSerialize_1.vec2 = vec2;

	objectToSerialize_2.vec2 = vec2;

	TestStruct2 objectOfTestStruct2, objectDesOfTestStruct2;
	objectOfTestStruct2.vec2 = vec2;
	objectOfTestStruct2.intVal = 999;

	TestStruct3 objectOfTestStruct3, objectDesOfTestStruct3;
	objectOfTestStruct3.vec2 = vec2;
	objectOfTestStruct3.floatVal = 444.3f;

	JsonSerializer serializer("test.json");
 	serializer.Serialize(objectToSerialize_1);
	serializer.Serialize(objectOfTestStruct2);
	serializer.Serialize(objectOfTestStruct3);
	serializer.SerializePointers();

	TestStruct objectToDeserialize;
	serializer.Deserialize(objectToDeserialize);
	serializer.Deserialize(objectDesOfTestStruct2);
	serializer.Deserialize(objectDesOfTestStruct3);
	serializer.DeserializePointers();

	serializer.Clear();

    return 0;
}