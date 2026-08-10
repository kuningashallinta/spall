namespace spall::d3d12
{
	inline RegionLayout regionLayout(
		Format format,
		const TextureRegion& region)
	{
		const std::uint32_t blockWidth = formatBlockWidth(format);
		const std::uint32_t blockHeight = formatBlockHeight(format);

		RegionLayout layout = {};
		layout.RowBytes = formatBlockCount(region.Width, blockWidth) * formatBytesPerBlock(format);
		layout.RowCount = formatBlockCount(region.Height, blockHeight);
		layout.FootprintWidth = formatBlockCount(region.Width, blockWidth) * blockWidth;
		layout.FootprintHeight = formatBlockCount(region.Height, blockHeight) * blockHeight;

		return layout;
	}
} // namespace spall::d3d12
