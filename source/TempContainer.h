#ifndef TEMP_CONTAINER_INCLUDE
#define TEMP_CONTAINER_INCLUDE

class TempContainer
{
public:
	TempContainer()
	{
		Realloc(m_currentSize);
	}

	~TempContainer()
	{
		if (m_data)
		{
			delete[] m_data;
			m_data = nullptr;
		}
	}


	template<typename T>
	void SetValue(const T& value)
	{
		constexpr size_t valueSize = sizeof(T);
		if (valueSize > m_currentSize)
		{
			Realloc(valueSize);
		}
		std::memcpy(m_data, &value, valueSize);
	}

	void* GetData() { return m_data; }
private:
	void Realloc(const size_t newSize)
	{
		if (m_data)
		{
			delete[] m_data;
		}
		m_data = new char[newSize];
		m_currentSize = newSize;
	}

private:
	char* m_data = nullptr;
	size_t m_currentSize = 256U;
};

#endif