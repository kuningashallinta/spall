namespace spall
{
	inline Status validateQueryPoolCreateInfo(
		const QueryPoolCreateInfo& info)
	{
		if (info.TimestampCount == 0)
		{
			return ERR_INVALID_SIZE;
		}

		return {};
	}

	inline Status validateTimestampWrite(
		const QueryPoolInfo& info,
		std::uint32_t query)
	{
		if (query >= info.TimestampCount)
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}

	inline Status validateTimestampRead(
		const QueryPoolInfo& info,
		std::uint32_t firstQuery,
		std::size_t queryCount)
	{
		if (queryCount == 0)
		{
			return ERR_INVALID_SIZE;
		}

		const std::uint64_t lastQuery = static_cast<std::uint64_t>(firstQuery) + static_cast<std::uint64_t>(queryCount);

		if (lastQuery > info.TimestampCount)
		{
			return ERR_INVALID_RANGE;
		}

		return {};
	}
} // namespace spall
