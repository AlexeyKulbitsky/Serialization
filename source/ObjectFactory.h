#ifndef SHADOW_OBJECT_FACTORY
#define SHADOW_OBJECT_FACTORY

#include "ObjectDesc.h"
#include "Property.h"
#include "TempContainer.h"

#include <string>
#include <unordered_map>
#include <cassert>
#include <memory>
#include <algorithm>


class ObjectFactory
{
public:
    static ObjectFactory& GetInstance()
    {
        static ObjectFactory factory;
        return factory;
    }

    template<typename ObjectType>
    ObjectDesc& RegisterObject(const std::string& objectName)
    {
		const auto objectId = GetObjectId<ObjectType>();
		auto findResult = m_descs.find(objectId);
		assert(findResult == m_descs.end());

		m_descs[objectId] = ObjectDesc(objectName);
		auto& objectDesc = m_descs.at(objectId);
		objectDesc.CreateFactory<ObjectType>();
        return objectDesc;
    }

	template<typename ObjectType>
	const ObjectDesc& GetObjectDesc()
	{
		const auto objectId = GetObjectId<ObjectType>();
		auto findResult = m_descs.find(objectId);
		assert(findResult != m_descs.end());
		
		return findResult->second;
	}

	const ObjectDesc& GetObjectDesc(const uintptr_t objectId)
	{
		auto findResult = m_descs.find(objectId);
		assert(findResult != m_descs.end());

		return findResult->second;
	}

	const ObjectDesc& GetObjectDesc(const std::string& objectName)
	{
		using map_value = std::unordered_map<uintptr_t, ObjectDesc>::value_type;
		auto findResult = std::find_if(m_descs.begin(), m_descs.end(), [&objectName](const map_value& value)
		{
			return value.second.GetName() == objectName;
		});
		assert(findResult != m_descs.end());

		return findResult->second;
	}

	template<typename ObjectType>
	static uintptr_t GetObjectId()
	{
		static ObjectType s_inst;
		return reinterpret_cast<uintptr_t>(&s_inst);
	}

	template<typename ObjectType>
	bool IsObjectRegistered() const
	{
		const auto objectId = GetObjectId<ObjectType>();
		auto findResult = m_descs.find(objectId);
		return findResult != m_descs.end();
	}

	TempContainer& GetTempContainer() { return m_tempContainer; }

private:
    std::unordered_map<uintptr_t, ObjectDesc> m_descs;
	TempContainer m_tempContainer;
};

template <typename ObjectType>
ObjectDesc& class_(const std::string& objectName)
{
    auto& instance = ObjectFactory::GetInstance();
    return instance.RegisterObject<ObjectType>(objectName);
}

#endif
