#ifndef TYPE_INFO_INCLUDE
#define TYPE_INFO_INCLUDE

#include "TypeTraits.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <functional>

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
		Map,
		String,

		Undefined
	};

	enum class ArrayType
	{
		CStyleArray,
		StaticArray,
		Vector,

		Undefined
	};

	struct VectorParams
	{
		using SizeGetter = std::function<size_t(void*)>;
		using ItemGetter = std::function<void*(void*, const size_t)>;
		using SizeSetter = std::function<void(void*, size_t)>;

		SizeGetter getSize;
		ItemGetter getItem;
		SizeSetter setSize;

	} vectorParams;

	struct MapParams
	{
		std::unique_ptr<TypeInfo> keyTypeInfo;
		std::unique_ptr<TypeInfo> valueTypeInfo;
	} mapParams;

	const Type GetType() const { return m_type; }
	const uintptr_t GetObjectDescId() const { return m_objectDescId; }
	const std::unique_ptr<TypeInfo>& GetUnderlyingType() const { return m_underlyingType; }
	const ArrayType GetArrayType() const { return m_arrayType; }
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
			m_arrayType = ArrayType::CStyleArray;

			// Get the size of array and the type of element
			m_elementsCount = deducer.elementsCount;

			auto typeInfo = std::make_unique<TypeInfo>();
			m_underlyingType = std::move(typeInfo);
			m_underlyingType->m_type = deducer.underlyingType;
			m_underlyingType->m_elementSize = deducer.elementSize;

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
			m_type = String;
		}
		else if (is_vector<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			m_type = Array;
			m_arrayType = ArrayType::Vector;

			m_elementsCount = 0U;
			auto typeInfo = std::make_unique<TypeInfo>();
			m_underlyingType = std::move(typeInfo);
			m_underlyingType->m_type = deducer.underlyingType;
			m_underlyingType->m_elementSize = deducer.elementSize;

			using ElemetType = remove_vector_extent<ObjectType>::type;
			m_underlyingType->Init<ElemetType>();

			vectorParams.getSize = VectorSizeGetter<ElemetType>;
			vectorParams.getItem = VectorItemGetter<ElemetType>;
			vectorParams.setSize = VectorSizeSetter<ElemetType>;
		}
		else if (is_map<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			m_type = Map;
			mapParams.keyTypeInfo = std::make_unique<TypeInfo>();
			mapParams.valueTypeInfo = std::make_unique<TypeInfo>();

			mapParams.keyTypeInfo->m_type = deducer.keyType;
			mapParams.valueTypeInfo->m_type = deducer.valueType;
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
	Type m_type = Type::Undefined;
	ArrayType m_arrayType = ArrayType::Undefined;
	uintptr_t m_objectDescId = 0U;
	size_t m_elementsCount = 0U;
	size_t m_elementSize = 0U;
	std::unique_ptr<TypeInfo> m_underlyingType;
};

template<typename ObjectType>
struct TypeDeducer
{
	TypeDeducer()
	{
		if (std::is_integral<ObjectType>::value)
		{
			type = TypeInfo::Integral;
		}
		else if (std::is_floating_point<ObjectType>::value)
		{
			type = TypeInfo::Floating;
		}
		else if (std::is_class<ObjectType>::value)
		{
			type = TypeInfo::Class;
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			type = TypeInfo::Pointer;
		}
	}

	TypeInfo::Type type = TypeInfo::Undefined;
	TypeInfo::Type underlyingType = TypeInfo::Undefined;
	size_t elementsCount = 0U;
	size_t elementSize = 0U;
};

template<typename ObjectType, size_t N>
struct TypeDeducer<ObjectType[N]>
{
	TypeDeducer()
	{
		elementsCount = N;
		elementSize = sizeof(ObjectType);
		if (std::is_integral<ObjectType>::value)
		{
			underlyingType = TypeInfo::Integral;
		}
		else if (std::is_floating_point<ObjectType>::value)
		{
			underlyingType = TypeInfo::Floating;
		}
		else if (std::is_class<ObjectType>::value)
		{
			underlyingType = TypeInfo::Class;
		}
		else if (std::is_array<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			underlyingType = TypeInfo::Array;
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			underlyingType = TypeInfo::Pointer;
		}
	}

	TypeInfo::Type type = TypeInfo::Array;
	TypeInfo::Type underlyingType = TypeInfo::Undefined;
	size_t elementsCount = 0U;
	size_t elementSize = 0U;
};


template<typename ObjectType>
struct TypeDeducer<std::vector<ObjectType>>
{
	TypeDeducer()
	{
		elementsCount = 0;
		elementSize = sizeof(ObjectType);
		if (std::is_integral<ObjectType>::value)
		{
			underlyingType = TypeInfo::Integral;
		}
		else if (std::is_floating_point<ObjectType>::value)
		{
			underlyingType = TypeInfo::Floating;
		}
		else if (std::is_class<ObjectType>::value)
		{
			underlyingType = TypeInfo::Class;
		}
		else if (std::is_array<ObjectType>::value)
		{
			TypeDeducer<ObjectType> deducer;
			underlyingType = TypeInfo::Array;
		}
		else if (std::is_pointer<ObjectType>::value)
		{
			underlyingType = TypeInfo::Pointer;
		}
	}

	TypeInfo::Type type = TypeInfo::Array;
	TypeInfo::Type underlyingType = TypeInfo::Undefined;
	size_t elementsCount = 0U;
	size_t elementSize = 0U;
};


template<typename KeyType, typename ValueType>
struct TypeDeducer<std::map<KeyType, ValueType>>
{
	TypeDeducer()
	{
		TypeDeducer<KeyType> keyTypeDeducer;
		TypeDeducer<ValueType> valueTypeDeducer;

		keyType = keyTypeDeducer.type;
		valueType = valueTypeDeducer.type;
	}

	TypeInfo::Type type = TypeInfo::Map;
	
	TypeInfo::Type keyType = TypeInfo::Undefined;
	TypeInfo::Type valueType = TypeInfo::Undefined;

	TypeInfo::Type underlyingType = TypeInfo::Undefined;
	size_t elementsCount = 0U;
	size_t elementSize = 0U;
};

#endif // !TYPE_INFO_INCLUDE

