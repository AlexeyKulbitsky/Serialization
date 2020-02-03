#ifndef PROPERTY_INCLUDE
#define PROPERTY_INCLUDE

#include "TypeInfo.h"

#include <string>
#include <type_traits>

class Property
{
public:
    Property(const std::string& name) : m_name(name) {}
    const std::string& GetName() const;

	virtual void SetValue(void* object, void* data) = 0;
	virtual void* GetValue(void* object) = 0;

	const TypeInfo::Type GetType() const
	{
		return m_typeInfo->type;
	}

	const TypeInfo& GetTypeInfo() const
	{
		return *m_typeInfo;
	}

protected:
	TypeInfo* m_typeInfo = nullptr;

private:
    std::string m_name = "";
};


template<typename ObjectType, typename FieldType>
class FieldProperty : public Property
{
public:
    FieldProperty(const std::string& name, FieldType ObjectType::* fieldPtr)
        : Property(name)
		, m_assigner(fieldPtr)
    {
		auto& typeInfoCollection = TypeInfoCollection::GetInstance();
		if (!typeInfoCollection.IsTypeInfoRegistered<FieldType>())
		{
			typeInfoCollection.RegisterTypeInfo<FieldType>();
		}
		m_typeInfo = typeInfoCollection.GetTypeInfo<FieldType>();
    }

    void SetValue(void* object, void* data) override final
    {
		m_assigner(object, data);
    }

    void* GetValue(void* object) override final
    {
        ObjectType* concreteObject = reinterpret_cast<ObjectType*>(object);
		return reinterpret_cast<void*>(&(concreteObject->*(m_assigner.fieldPtr)));
    }

private:
	MemberAssigner<ClassField<ObjectType, FieldType>> m_assigner;
};


template<typename ObjectType, typename ReturnType, typename SetType>
class AccessorProperty : public Property
{
public:
    AccessorProperty(const std::string& name, ReturnType(ObjectType::* getter)(), void (ObjectType::* setter)(SetType))
        : Property(name)
		, m_getter(getter)
		, m_setter(setter)
    {
		using NonReferenceType = std::remove_reference<SetType>::type;
		using BaseType = std::remove_cv<NonReferenceType>::type;

		auto& typeInfoCollection = TypeInfoCollection::GetInstance();
		if (!typeInfoCollection.IsTypeInfoRegistered<BaseType>())
		{
			typeInfoCollection.RegisterTypeInfo<BaseType>();
		}
		m_typeInfo = typeInfoCollection.GetTypeInfo<BaseType>();
    }

    AccessorProperty(const std::string& name, ReturnType(ObjectType::* getter)() const, void (ObjectType::* setter)(SetType))
        : Property(name)
		, m_constGetter(getter)
		, m_setter(setter)
    {
		using NonReferenceType = std::remove_reference<SetType>::type;
		using BaseType = std::remove_cv<NonReferenceType>::type;

		auto& typeInfoCollection = TypeInfoCollection::GetInstance();
		if (!typeInfoCollection.IsTypeInfoRegistered<BaseType>())
		{
			typeInfoCollection.RegisterTypeInfo<BaseType>();
		}
		m_typeInfo = typeInfoCollection.GetTypeInfo<BaseType>();
    }

    void SetValue(void* object, void* data) override final
    {
		using NonReferenceType = std::remove_reference<SetType>::type;
		using BaseType = std::remove_cv<NonReferenceType>::type;
		BaseType* actualDataPtr = reinterpret_cast<BaseType*>(data);

		ObjectType* concreteObject = reinterpret_cast<ObjectType*>(object);
		(concreteObject->*m_setter)(*actualDataPtr);
    }

    void* GetValue(void* object) override final
    {
		auto& tempContainer = ObjectFactory::GetInstance().GetTempContainer();
		ObjectType* concreteObject = reinterpret_cast<ObjectType*>(object);
		void* data = nullptr;
		if (m_getter)
		{
			auto value = (concreteObject->*m_getter)();
			tempContainer.SetValue(value);
		}
		else if (m_constGetter)
		{
			auto value = (concreteObject->*m_constGetter)();
			tempContainer.SetValue(value);
		}
		return tempContainer.GetData();
    }

private:
	ReturnType(ObjectType::* m_getter)()  = nullptr;
	ReturnType(ObjectType::* m_constGetter)() const  = nullptr;
	void (ObjectType::* m_setter)(SetType) = nullptr;
};

#endif
