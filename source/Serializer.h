#ifndef SERIALIZER_INCLUDE
#define SERIALIZER_INCLUDE

#include "ObjectFactory.h"

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
		
		SerializeInternal(objectDesc, objectRawPtr);
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

protected:
	virtual void SerializeInternal(const ObjectDesc& objectDesc, void* object) = 0;
	virtual void DeserializeInternal(const ObjectDesc& objectDesc, void* object) = 0;
};

#endif
