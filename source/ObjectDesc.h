#ifndef OBJECT_DESC_INCLUDE
#define OBJECT_DESC_INCLUDE

#include "Property.h"

#include <string>
#include <unordered_map>
#include <cassert>

class ConcreteObjectFactory
{
public:
	virtual void* CreateObject() = 0;
};

template<typename ObjectType>
class ConcreteObjectFactoryImpl : public ConcreteObjectFactory
{
public:
	void* CreateObject() override final
	{
		ObjectType* instance = new ObjectType();
		return reinterpret_cast<void*>(instance);
	}
};

class ObjectDesc
{
public:
	ObjectDesc() = default;
	ObjectDesc(const std::string name) : m_name(name) {}

	~ObjectDesc()
	{
		if (m_factory)
		{
			delete m_factory;
			m_factory = nullptr;
		}
	}

	template<typename ObjectType>
	void CreateFactory()
	{
		m_factory = new ConcreteObjectFactoryImpl<ObjectType>();
	}

	template<typename ObjectType, typename FieldType>
	ObjectDesc& AddProperty(const std::string& name, FieldType ObjectType::* fieldPointer)
	{
		auto findResult = m_properties.find(name);
		assert(findResult == m_properties.end());

		Property* prop = new FieldProperty<ObjectType, FieldType>(name, fieldPointer);

		m_properties[name] = prop;
		return *this;
	}

	template<typename ObjectType, typename ReturnType, typename SetType>
	ObjectDesc& AddProperty(const std::string& name, ReturnType(ObjectType::* getter)(), void (ObjectType::* setter)(SetType) = nullptr)
	{
		auto findResult = m_properties.find(name);
		assert(findResult == m_properties.end());

		Property* prop = new AccessorProperty<ObjectType, ReturnType, SetType>(name, getter, setter);
		m_properties[name] = prop;
		return *this;
	}

	template<typename ObjectType, typename ReturnType, typename SetType>
	ObjectDesc& AddProperty(const std::string& name, ReturnType(ObjectType::* getter)() const, void (ObjectType::* setter)(SetType) = nullptr)
	{
		auto findResult = m_properties.find(name);
		assert(findResult == m_properties.end());

		Property* prop = new AccessorProperty<ObjectType, ReturnType, SetType>(name, getter, setter);
		m_properties[name] = prop;
		return *this;
	}

	const std::unordered_map<std::string, Property*>& GetProperties() const { return m_properties; }
	const std::string& GetName() const { return m_name; }

private:
	ConcreteObjectFactory* m_factory = nullptr;
	std::unordered_map<std::string, Property*> m_properties;
	std::string m_name;

	friend class ObjectFactory;
};

#endif
