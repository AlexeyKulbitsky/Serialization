#ifndef TYPE_INFO_INCLUDE
#define TYPE_INFO_INCLUDE

#include "TypeTraits.h"

#include <cstdint>
#include <iostream>
#include <memory>

class TypeInfo
{
public:
	enum Type
	{
		Integral,
		Floating,
		Class,
		Array,
		Pointer,
		Enum,

		Undefined
	};

	Type GetType() const { return m_type; }
	const uintptr_t GetObjectDescId() const { return m_objectDescId; }
	const std::unique_ptr<TypeInfo>& GetUnderlyingType() const { return m_underlyingType; }
	const size_t GetElementsCount() const { return m_elementsCount; }
	const size_t GetElementSize() const { return m_elementSize; }

	template<typename ObjectType>
	void Init()
	{
		if (std::is_integral<ObjectType>::value)
		{
			m_type = Integral;
		}
		else if (std::is_floating_point<ObjectType>::value)
		{
			m_type = Floating;
		}
		else if (std::is_enum<ObjectType>::value)
		{
			m_type = Enum;
		}
		else if (std::is_array<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			m_type = Array;
			// Get the size of array and the type of element
			m_elementsCount = deducer.m_elementsCount;

			auto typeInfo = std::make_unique<TypeInfo>();
			m_underlyingType = std::move(typeInfo);
			m_underlyingType->m_type = deducer.m_underlyingType;
			m_underlyingType->m_elementSize = deducer.m_elementSize;

			using ElementType = std::remove_extent<ObjectType>::type;
			m_underlyingType->Init<ElementType>();
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			m_type = Pointer;
			using RawObjectType = std::remove_pointer<ObjectType>::type;

			auto typeInfo = std::make_unique<TypeInfo>();
			m_underlyingType = std::move(typeInfo);
			m_underlyingType->Init<RawObjectType>();
		}
		
		else if (is_string<ObjectType>::value)
		{
			int a = 0;
			a++;
		}
		else if (is_vector<ObjectType>::value)
		{
			int a = 0;
			a++;
		}
		else if (std::is_class<ObjectType>::value)
		{
			const auto& objectFactory = ObjectFactory::GetInstance();
			if (objectFactory.IsObjectRegistered<ObjectType>())
			{
				m_type = Class;
				m_objectDescId = objectFactory.GetObjectId<ObjectType>();
			}
		}
	}

	

private:
	Type m_type = Undefined;
	uintptr_t m_objectDescId = 0U;
	size_t m_elementsCount = 0U;
	size_t m_elementSize = 0U;

	//////////////////////////////////////////////////////////////////////

	std::unique_ptr<TypeInfo> m_underlyingType;
};

template<typename ObjectType>
struct TypeDeducer
{
	TypeDeducer()
	{
		if (std::is_integral<ObjectType>::value)
		{
			m_type = TypeInfo::Integral;
		}
		else if (std::is_floating_point<ObjectType>::value)
		{
			m_type = TypeInfo::Floating;
		}
		else if (std::is_class<ObjectType>::value)
		{
			m_type = TypeInfo::Class;
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			m_type = TypeInfo::Pointer;
		}
	}

	TypeInfo::Type m_type = TypeInfo::Undefined;
	TypeInfo::Type m_underlyingType = TypeInfo::Undefined;
	size_t m_elementsCount = 0U;
	size_t m_elementSize = 0U;
};

template<typename ObjectType, size_t N>
struct TypeDeducer<ObjectType[N]>
{
	TypeDeducer()
	{
		m_elementsCount = N;
		m_elementSize = sizeof(ObjectType);
		if (std::is_integral<ObjectType>::value)
		{
			m_underlyingType = TypeInfo::Integral;
		}
		else if (std::is_floating_point<ObjectType>::value)
		{
			m_underlyingType = TypeInfo::Floating;
		}
		else if (std::is_class<ObjectType>::value)
		{
			m_underlyingType = TypeInfo::Class;
		}
		else if (std::is_array<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			m_underlyingType = TypeInfo::Array;
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			m_underlyingType = TypeInfo::Pointer;
		}
	}

	TypeInfo::Type m_type = TypeInfo::Array;
	TypeInfo::Type m_underlyingType = TypeInfo::Undefined;
	size_t m_elementsCount = 0U;
	size_t m_elementSize = 0U;
};

#endif // !TYPE_INFO_INCLUDE

