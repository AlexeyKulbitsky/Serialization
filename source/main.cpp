#include "ObjectFactory.h"
#include "Serializer.h"
#include "TypeInfo.h"

#include <iostream>
#include <sstream>


class ConsoleSerializer : public Serializer
{
private:
	std::stringstream ss;

protected:
	void SerializeInternal(const ObjectDesc& objectDesc, void* object) override final
	{
		std::cout << "Serializing object fo type: [" << objectDesc.GetName() << "]" << std::endl;

		const auto& properties = objectDesc.GetProperties();

		for (const auto prop : properties)
		{
			std::cout << "\tProperty: [" << prop.first << "]";

			const auto& typeInfo = prop.second->GetTypeInfo();
			void* data = prop.second->GetValue(object);

			SerializeByType(typeInfo, data);
		}
	}

	void DeserializeInternal(const ObjectDesc& objectDesc, void* object) override final
	{
		std::cout << "Deserializing object fo type: [" << objectDesc.GetName() << "]" << std::endl;

		const auto& properties = objectDesc.GetProperties();

		for (const auto prop : properties)
		{
			std::cout << "\tProperty: [" << prop.first << "]";

			const auto& typeInfo = prop.second->GetTypeInfo();
			void* data = prop.second->GetValue(object);

			DeserializeByType(typeInfo, data);

			prop.second->SetValue(object, data);
		}
	}

	void SerializeByType(const TypeInfo& typeInfo, void* data)
	{
		switch (typeInfo.type)
		{
		case TypeInfo::Fundamental:
		{
			const auto typeId = typeInfo.fundamentalTypeParams.typeIndex;
			const auto typeSize = typeInfo.fundamentalTypeParams.typeSize;
			ss.write(reinterpret_cast<char*>(data), typeSize);
		}
			break;
		case TypeInfo::Class:
		{
			std::cout << " Type: [Class] Value:" << std::endl;
			auto& factoryInstance = ObjectFactory::GetInstance();
			const auto id = typeInfo.GetObjectDescId();

			const auto& childObjectDesc = factoryInstance.GetObjectDesc(id);
			SerializeInternal(childObjectDesc, data);
		}
			break;
		case TypeInfo::String:
		{
			auto str = reinterpret_cast<std::string*>(data);
			const auto elementsCount = (*str).size();
			ss.write(reinterpret_cast<const char*>(&elementsCount), sizeof(size_t));
			ss.write((*str).data(), (*str).size());
		}
			break;
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
			std::cout << " Type: [Array] Value:" << std::endl;
			size_t elementsCount = 0U;
			const auto& elementTypeInfo = typeInfo.arrayParams.elementTypeInfo;

			if (typeInfo.arrayParams.arrayType == TypeInfo::ArrayType::Vector)
			{
				elementsCount = typeInfo.arrayParams.getSize(data);
				ss.write(reinterpret_cast<char*>(&elementsCount), sizeof(size_t));
			}
			else
			{
				elementsCount = typeInfo.arrayParams.elementsCount;
			}

			elementsCount = typeInfo.arrayParams.elementsCount;
			for (size_t i = 0U; i < elementsCount; ++i)
			{
				void* currentDataAddress = typeInfo.arrayParams.getItem(data, i);
				SerializeByType(*elementTypeInfo, currentDataAddress);
			}

		}
			break;
		case TypeInfo::Enum:
		{
			std::cout << "Type [Enum]" << std::endl;
			int value = *reinterpret_cast<int*>(data);
			ss.write(reinterpret_cast<char*>(data), sizeof(int));
		}
			break;
		case TypeInfo::Map:
		{
			size_t elementsCount = typeInfo.mapParams.getSize(data);
			ss.write(reinterpret_cast<char*>(&elementsCount), sizeof(size_t));

			auto it = typeInfo.mapParams.getIterator(data);

			while (typeInfo.mapParams.isIteratorValid(it, data))
			{
				auto key = typeInfo.mapParams.getKey(it);
				auto value = typeInfo.mapParams.getValue(it);
				SerializeByType(*typeInfo.mapParams.keyTypeInfo, (void*)key);
				SerializeByType(*typeInfo.mapParams.valueTypeInfo, value);

				typeInfo.mapParams.incrementIterator(it);
			}
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
		{
			const auto typeSize = typeInfo.fundamentalTypeParams.typeSize;
			char *buffer = new char[typeSize];

			ss.read(buffer, typeSize);
			std::memcpy(data, buffer, typeSize);

			delete[] buffer;
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

			size_t elementsCount;
			ss.read(reinterpret_cast<char*>(&elementsCount), sizeof(size_t));
			(*str).resize(elementsCount);

			char* a = new char[elementsCount];
			ss.read(a, elementsCount);
			for (size_t i = 0U; i < elementsCount; ++i)
			{
				(*str)[i] = a[i];
			}
			
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
			size_t elementsCount = 0U;
			const auto& underlyingTypeInfo = typeInfo.arrayParams.elementTypeInfo;

			if (typeInfo.arrayParams.arrayType == TypeInfo::ArrayType::Vector)
			{
				ss.read(reinterpret_cast<char*>(&elementsCount), sizeof(size_t));
				typeInfo.arrayParams.setSize(data, elementsCount);
			}
			else
			{
				elementsCount = typeInfo.arrayParams.elementsCount;
			}

			for (size_t i = 0U; i < elementsCount; ++i)
			{
				void* currentDataAddress = typeInfo.arrayParams.getItem(data, i);
				DeserializeByType(*underlyingTypeInfo, reinterpret_cast<void*>(currentDataAddress));
			}
		}
		break;
		case TypeInfo::Enum:
		{
			int value = 0;
			ss.read(reinterpret_cast<char*>(&value), sizeof(int));
			*reinterpret_cast<int*>(data) = value;
		}
		break;
		case TypeInfo::Map:
		{
			size_t elementsCount = 0;
			ss.read(reinterpret_cast<char*>(&elementsCount), sizeof(size_t));
			
			char* keyBuffer = new char[256];
			char* valueBuffer = new char[256];

			std::string v;
			void* keyPtr = reinterpret_cast<void*>(&v);
			//void* keyPtr = reinterpret_cast<void*>(keyBuffer);
			void* valuePtr = reinterpret_cast<void*>(valueBuffer);
			for (size_t i = 0U; i < elementsCount; ++i)
			{
				DeserializeByType(*typeInfo.mapParams.keyTypeInfo, keyPtr);
				DeserializeByType(*typeInfo.mapParams.valueTypeInfo, valuePtr);

				typeInfo.mapParams.setKeyValue(data, keyPtr, valuePtr);
			}

			delete[] keyBuffer;
			delete[] valueBuffer;
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

	TestStruct objectToSerialize;
	objectToSerialize.intValue = 7;
	objectToSerialize.floatValue = 23.5f;
	objectToSerialize.someStaticArray[1] = 100;
	objectToSerialize.someArray[3] = 900;
	objectToSerialize.someArray[4] = 7788;
	objectToSerialize.someMatrix[1][1] = 333333;
	objectToSerialize.someMap["One"] = 3.0f;
	objectToSerialize.someMap["BBBB"] = 444.4f;
	objectToSerialize.someEnum = MyEnum::Second;
	objectToSerialize.someString = "AAA";
	objectToSerialize.vec3.z = 111.555f;

    ConsoleSerializer serializer;
 	serializer.Serialize(objectToSerialize);

	TestStruct objectToDeserialize;
	serializer.Deserialize(objectToDeserialize);

	/*DerivedClass derivedToSerialize, derivedToDeserialize;
	derivedToSerialize.a = 1111;
	serializer.Serialize(derivedToSerialize);
	serializer.Deserialize(derivedToDeserialize);*/


    return 0;
}