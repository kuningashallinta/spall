namespace spall::d3d12
{
	inline Status mapStatus(
		HRESULT hr)
	{
		if (SUCCEEDED(hr))
		{
			return SUCCESS;
		}

		switch (hr)
		{
			case E_INVALIDARG:
			{
				return ERR_INVALID_ARGUMENT;
			}

			case E_OUTOFMEMORY:
			{
				return ERR_OUT_OF_MEMORY;
			}

			case DXGI_ERROR_DEVICE_REMOVED:
			case DXGI_ERROR_DEVICE_RESET:
			case DXGI_ERROR_DEVICE_HUNG:
			case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
			{
				return ERR_DEVICE_LOST;
			}

			default:
			{
				return ERR_BACKEND_FAILURE;
			}
		}
	}
} // namespace spall::d3d12
