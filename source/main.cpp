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
		switch (typeInfo.GetType())
		{
		case TypeInfo::Integral:
		{
			int value = *reinterpret_cast<int*>(data);
			std::cout << " Type: [Integral] Value: " << value << std::endl;
			ss.write(reinterpret_cast<char*>(data), sizeof(int));
		}
			break;
		case TypeInfo::Floating:
		{
			float value = *reinterpret_cast<float*>(data);
			std::cout << " Type: [Floating] Value: " << value << std::endl;
			ss.write(reinterpret_cast<char*>(data), sizeof(float));
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
			const auto& underlyingTypeInfo = typeInfo.GetUnderlyingType();
			char* dataPtr = reinterpret_cast<char*>(data);

			switch (typeInfo.GetArrayType())
			{
			case TypeInfo::ArrayType::CStyleArray:
			{
				elementsCount = typeInfo.GetElementsCount();
				for (size_t i = 0U; i < elementsCount; ++i)
				{
					char* currentDataAddress = dataPtr + i * underlyingTypeInfo->GetElementSize();
					SerializeByType(*underlyingTypeInfo, reinterpret_cast<void*>(currentDataAddress));
				}
			}
				break;
			case TypeInfo::ArrayType::StaticArray:
				break;
			case TypeInfo::ArrayType::Vector:
			{
				elementsCount = typeInfo.vectorParams.getSize(data);
				ss.write(reinterpret_cast<char*>(&elementsCount), sizeof(size_t));
				for (size_t i = 0U; i < elementsCount; ++i)
				{
					void* currentDataAddress = typeInfo.vectorParams.getItem(data, i);
					SerializeByType(*underlyingTypeInfo, currentDataAddress);
				}
			}
				break;
			default:
				break;
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
		switch (typeInfo.GetType())
		{
		case TypeInfo::Integral:
		{
			int value = 0;
			ss.read(reinterpret_cast<char*>(&value), sizeof(int));
			*reinterpret_cast<int*>(data) = value;
		}
		break;
		case TypeInfo::Floating:
		{
			float value = 0;
			ss.read(reinterpret_cast<char*>(&value), sizeof(float));
			*reinterpret_cast<float*>(data) = value;
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
			const auto& underlyingTypeInfo = typeInfo.GetUnderlyingType();
			switch (typeInfo.GetArrayType())
			{
			case TypeInfo::ArrayType::CStyleArray:
			{
				const size_t elementsCount = typeInfo.GetElementsCount();
				size_t offset = 0U;
				char* dataPtr = reinterpret_cast<char*>(data);
				for (size_t i = 0U; i < elementsCount; ++i)
				{
					char* currentDataAddress = dataPtr + i * underlyingTypeInfo->GetElementSize();
					DeserializeByType(*underlyingTypeInfo, reinterpret_cast<void*>(currentDataAddress));
				}
			}
				break;
			case TypeInfo::ArrayType::StaticArray:
				break;
			case TypeInfo::ArrayType::Vector:
			{
				size_t elementsCount = 0;
				ss.read(reinterpret_cast<char*>(&elementsCount), sizeof(size_t));
				typeInfo.vectorParams.setSize(data, elementsCount);
				for (size_t i = 0U; i < elementsCount; ++i)
				{
					void* currentDataAddress = typeInfo.vectorParams.getItem(data, i);
					DeserializeByType(*underlyingTypeInfo, reinterpret_cast<void*>(currentDataAddress));
				}
			}
				break;
			default:
				break;
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
			for (size_t i = 0U; i < elementsCount; ++i)
			{
				const size_t keySize = typeInfo.mapParams.keyTypeInfo->GetElementSize();

				int a = 0;
				a++;
			}
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
	int b = 100;
};

class DerivedClass : public BaseClass
{
public:
	int c = 222;
};

struct TestStruct
{
	int intValue = 5;
	float floatValue = 7.0f;
	int someArray[5] = { 1, 4, 5, 8, 9 };
	int someMatrix[3][3] = { {1, 2, 3}, {3, 3, 3}, {6, 6, 6} };
	std::vector<int> someVector;
	std::map<int, float> someMap;
	Vec3 vec3;
	Vec2* vec2 = nullptr;
	MyEnum someEnum;

	Vec3 privateVec3;
	const Vec3 GetVec3() const { return privateVec3; }
	void SetVec3(const Vec3& value) { privateVec3 = value; }
};

int main()
{
	/*class_<Vec2>("Vec2")
		.AddProperty("x", &Vec2::x)
		.AddProperty("y", &Vec2::y);

	class_<Vec3>("Vec3")
		.AddProperty("x", &Vec3::x)
		.AddProperty("y", &Vec3::y)
		.AddProperty("z", &Vec3::z);*/

	class_<TestStruct>("TestStruct")
		//.AddProperty("intValue", &TestStruct::intValue)
		//.AddProperty("floatValue", &TestStruct::floatValue)
		//.AddProperty("someArray", &TestStruct::someArray)
		//.AddProperty("someMatrix", &TestStruct::someMatrix)
		//.AddProperty("someVector", &TestStruct::someVector)
		//.AddProperty("vec3", &TestStruct::vec3)
		.AddProperty("someMap", &TestStruct::someMap);
		//.AddProperty("vec2", &TestStruct::vec2)
		//.AddProperty("someEnum", &TestStruct::someEnum)
		//.AddProperty("Vec3Accessor", &TestStruct::GetVec3, &TestStruct::SetVec3)
		;

	/*class_<BaseClass>("BaseClass")
		.AddProperty("a", &BaseClass::a)
		.AddProperty("b", &BaseClass::b);

	class_<DerivedClass>("DerivedClass")
		.AddProperty("c", &DerivedClass::c);*/

	TestStruct objectToSerialize;
	//objectToSerialize.vec3.x = 1000.0f;
	//objectToSerialize.vec3.y = 335.5f;
	//objectToSerialize.vec3.z = 700.1f;
	//objectToSerialize.someArray[3] = 900;
	//objectToSerialize.someArray[4] = 7788;
	//objectToSerialize.someMatrix[1][1] = 333333;
	//objectToSerialize.someEnum = MyEnum::Second;
	//objectToSerialize.SetVec3(Vec3{ 1.0f, 999.0f, 56.3f });
	//objectToSerialize.someVector.push_back(100);
	//objectToSerialize.someVector.push_back(134);

	objectToSerialize.someMap[16] = 3.0f;
	objectToSerialize.someMap[27] = 444.4f;
	auto it = objectToSerialize.someMap.begin();
	auto endIt = objectToSerialize.someMap.end();

    ConsoleSerializer serializer;
 	serializer.Serialize(objectToSerialize);

	TestStruct objectToDeserialize;
	serializer.Deserialize(objectToDeserialize);

    return 0;
}