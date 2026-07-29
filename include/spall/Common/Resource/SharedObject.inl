#include <limits>

namespace spall
{
	template <typename T>
	inline std::uint32_t SharedObject<T>::addRef(
		void)
	{
		std::uint32_t count = m_RefCount.load(std::memory_order_relaxed);

		for (;;)
		{
			SPALL_VERIFY(count != (std::numeric_limits<std::uint32_t>::max)());

			if (count == (std::numeric_limits<std::uint32_t>::max)())
			{
				return count;
			}

			if (m_RefCount.compare_exchange_weak(
					count,
					count + 1,
					std::memory_order_relaxed,
					std::memory_order_relaxed))
			{
				return count + 1;
			}
		}
	}

	template <typename T>
	inline std::uint32_t SharedObject<T>::release(
		void)
	{
		std::uint32_t count = m_RefCount.load(std::memory_order_relaxed);

		for (;;)
		{
			SPALL_VERIFY(count != 0);

			if (count == 0)
			{
				return 0;
			}

			if (m_RefCount.compare_exchange_weak(
					count,
					count - 1,
					std::memory_order_acq_rel,
					std::memory_order_relaxed))
			{
				break;
			}
		}

		--count;

		if (count == 0)
		{
			delete this;
		}

		return count;
	}

	template <typename T>
	inline std::uint32_t SharedObject<T>::referenceCount(
		void) const
	{
		return m_RefCount.load(std::memory_order_relaxed);
	}
} // namespace spall
