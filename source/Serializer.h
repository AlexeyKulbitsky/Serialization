#ifndef SERIALIZER_INCLUDE
#define SERIALIZER_INCLUDE

#include "ObjectFactory.h"
#include <set>

class Serializer
{
public:
    virtual ~Serializer() = default;

	template<typename ObjectType>
	void Serialize(ObjectType* object)
	{
		auto& factoryInstance = ObjectFactory::GetInstance();

		const auto& objectDesc = factoryInstance.GetObjectDesc<ObjectType>();
		void* objectRawPtr = reinterpret_cast<void*>(object);

		const bool isAlreadySerialized = m_serializedPointers.find(objectRawPtr) != m_serializedPointers.end();
		const bool isMarkedForSerialization = m_pointersToSerialize.find(objectRawPtr) != m_pointersToSerialize.end();

		if (!isAlreadySerialized && !isMarkedForSerialization)
		{
			SerializeInternal(objectDesc, objectRawPtr);
			m_serializedPointers.insert(objectRawPtr);
		}
	}

	template<typename ObjectType>
	void Serialize(const ObjectType& object)
	{
		auto& factoryInstance = ObjectFactory::GetInstance();

		const auto& objectDesc = factoryInstance.GetObjectDesc<ObjectType>();
		ObjectType* objectPtr = const_cast<ObjectType*>(&object);
		void* objectRawPtr = reinterpret_cast<void*>(objectPtr);

		SerializeInternal(objectDesc, objectRawPtr);
	}

	template<typename ObjectType>
	void Deserialize(ObjectType* object)
	{
		auto& factoryInstance = ObjectFactory::GetInstance();

		const auto& objectDesc = factoryInstance.GetObjectDesc<ObjectType>();
		void* objectRawPtr = reinterpret_cast<void*>(object);

		DeserializeInternal(objectDesc, objectRawPtr);
	}

	template<typename ObjectType>
	void Deserialize(ObjectType& object)
	{
		auto& factoryInstance = ObjectFactory::GetInstance();

		const auto& objectDesc = factoryInstance.GetObjectDesc<ObjectType>();
		ObjectType* objectPtr = const_cast<ObjectType*>(&object);
		void* objectRawPtr = reinterpret_cast<void*>(objectPtr);

		DeserializeInternal(objectDesc, objectRawPtr);
	}

	void SerializePointers()
	{
		for (auto& it : m_pointersToSerialize)
		{
			auto addr = it.first;
			auto typeInfo = it.second;

			auto& factoryInstance = ObjectFactory::GetInstance();
			const auto id = typeInfo->GetObjectDescId();

			const auto& childObjectDesc = factoryInstance.GetObjectDesc(id);
			SerializeInternal(childObjectDesc, addr);
		}
		m_pointersToSerialize.clear();
	}

protected:
	virtual void SerializeInternal(const ObjectDesc& objectDesc, void* object) = 0;
	virtual void DeserializeInternal(const ObjectDesc& objectDesc, void* object) = 0;

	std::unordered_map<void*, TypeInfo*> m_pointersToSerialize;
	std::set<void*> m_serializedPointers;
};

#endif
