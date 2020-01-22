#include "ObjectFactory.h"
#include "Serializer.h"
#include "TypeInfo.h"

#include <iostream>

#include <json/json.h>


class JsonSerializer : public Serializer
{
public:
	JsonSerializer(const std::string& filePath) : m_filePath(filePath) {}

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

protected:
	void SerializeInternal(const ObjectDesc& objectDesc, void* object) override final
	{
		Json::Value child;
		const auto& properties = objectDesc.GetProperties();

		for (const auto prop : properties)
		{
			const auto& typeInfo = prop.second->GetTypeInfo();
			void* data = prop.second->GetValue(object);

			Json::Value propertyValue;
			m_currentValue = &propertyValue;

			SerializeByType(typeInfo, data);

			child[prop.first] = propertyValue;
		}

		m_root[objectDesc.GetName()] = child;
	}

	void DeserializeInternal(const ObjectDesc& objectDesc, void* object) override final
	{
		Json::Value child = m_root[objectDesc.GetName()];

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
			SerializeInternal(childObjectDesc, data);
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
			
			/*void* actualPtr = *reinterpret_cast<void**>(data);
			if (actualPtr != nullptr)
			{
				const bool isNotSerializedYet = m_serializedPointers.find(actualPtr) == m_serializedPointers.end();
				if (isNotSerializedYet)
				{
					auto& underlyingTypeInfo = typeInfo.underlyingType;
					m_pointersToSerialize[actualPtr] = underlyingTypeInfo.get();
				}
				else
				{

				}
			}*/
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
			DeserializeInternal(childObjectDesc, data);
		}
		break;
		case TypeInfo::String:
		{
			auto str = reinterpret_cast<std::string*>(data);
			(*str) = m_currentValue->asString();
		}
		case TypeInfo::Pointer:
		{
			/*std::cout << " Type: [Pointer] Value:" << std::endl;
			void* actualPtr = *reinterpret_cast<void**>(data);
			if (actualPtr != nullptr)
			{
				const auto& underlyingTypeInfo = typeInfo.GetUnderlyingType();
				SerializeByType(*underlyingTypeInfo, actualPtr);
			}*/
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
		.AddProperty("intValue", &TestStruct::intValue)
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

	JsonSerializer serializer("test.json");
 	serializer.Serialize(objectToSerialize_1);

	TestStruct objectToDeserialize;
	serializer.Deserialize(objectToDeserialize);

	serializer.Clear();

    return 0;
}