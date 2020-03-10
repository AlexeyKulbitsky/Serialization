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

	virtual void SerializePointers()
	{
	}

	virtual void DeserializePointers()
	{
	}

	virtual void Clear()
	{
		m_serializedPointers.clear();
	}

protected:
	virtual void SerializeInternal(const ObjectDesc& objectDesc, void* object) = 0;
	virtual void DeserializeInternal(const ObjectDesc& objectDesc, void* object) = 0;

	std::unordered_map<void*, const TypeInfo*> m_pointersToSerialize;
	std::set<void*> m_serializedPointers;

	std::unordered_map<uintptr_t, std::vector<void*>> m_pointersToDeserialize;
};

#endif
