namespace spall
{
	template <typename T>
	inline Resource<T>::Resource(
		T* resource)
		: m_Resource(resource)
	{
		if (m_Resource != nullptr)
		{
			m_Resource->addRef();
		}
	}

	template <typename T>
	inline Resource<T>::Resource(
		const Resource<T>& other)
		: m_Resource(other.m_Resource)
	{
		if (m_Resource != nullptr)
		{
			m_Resource->addRef();
		}
	}

	template <typename T>
	inline Resource<T>::Resource(
		Resource<T>&& other) noexcept
		: m_Resource(other.m_Resource)
	{
		other.m_Resource = nullptr;
	}

	template <typename T>
	inline Resource<T>::~Resource(
		void)
	{
		if (m_Resource != nullptr)
		{
			m_Resource->release();
		}
	}

	template <typename T>
	inline Resource<T>& Resource<T>::operator=(
		const Resource<T>& other)
	{
		if (this != &other)
		{
			if (other.m_Resource != nullptr)
			{
				other.m_Resource->addRef();
			}

			if (m_Resource != nullptr)
			{
				m_Resource->release();
			}

			m_Resource = other.m_Resource;
		}

		return *this;
	}

	template <typename T>
	inline Resource<T>& Resource<T>::operator=(
		Resource<T>&& other) noexcept
	{
		if (this != &other)
		{
			if (m_Resource != nullptr)
			{
				m_Resource->release();
			}

			m_Resource = other.m_Resource;
			other.m_Resource = nullptr;
		}

		return *this;
	}

	template <typename T>
	inline T& Resource<T>::operator*(
		void) const
	{
		SPALL_ASSERT(m_Resource != nullptr);
		return *m_Resource;
	}

	template <typename T>
	inline T* Resource<T>::operator->(
		void) const
	{
		SPALL_ASSERT(m_Resource != nullptr);
		return m_Resource;
	}

	template <typename T>
	inline Resource<T>::operator bool(
		void) const
	{
		return m_Resource != nullptr;
	}

	template <typename T>
	inline T* Resource<T>::get(
		void) const
	{
		return m_Resource;
	}

	template <typename T>
	inline void Resource<T>::reset(
		void)
	{
		reset(nullptr);
	}

	template <typename T>
	inline void Resource<T>::reset(
		T* resource)
	{
		if (m_Resource != nullptr)
		{
			if (resource != nullptr)
			{
				resource->addRef();
			}

			m_Resource->release();
			m_Resource = resource;
		}
		else
		{
			m_Resource = resource;

			if (m_Resource != nullptr)
			{
				m_Resource->addRef();
			}
		}
	}
} // namespace spall
