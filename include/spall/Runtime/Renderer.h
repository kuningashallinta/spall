#pragma once

#include <spall/Common/Enums/RenderBackendType.h>
#include <spall/Common/Status/Status.h>
#include <spall/Runtime/Frame.h>
#include <spall/Runtime/FrameBeginInfo.h>
#include <spall/Runtime/RendererCreateInfo.h>

#include <cstdint>
#include <memory>

namespace spall
{
	class IDevice;
	class ISwapChain;
	class RendererImpl;

	/// Owns the common backend, device, swap-chain, and presentation lifecycle.
	///
	/// Applications may use the managed frame workflow or access the underlying
	/// primitives directly for more control.
	class Renderer
	{
	public:
		Renderer(void);
		Renderer(Renderer&& other) noexcept;

		~Renderer(void);

		Renderer& operator=(Renderer&& other) noexcept;

		explicit operator bool(void) const noexcept;

		/// Creates a renderer and preserves detailed failure information.
		static Status create(
			const RendererCreateInfo& info,
			Renderer* renderer);

		/// Creates a renderer, returning an invalid renderer on failure.
		static Renderer create(
			const RendererCreateInfo& info);

		RenderBackendType backendType(void) const;
		std::uint32_t width(void) const;
		std::uint32_t height(void) const;
		Format colorFormat(void) const;
		Format depthStencilFormat(void) const;

		IDevice& device(void) const;
		ISwapChain& swapChain(void) const;

		/// Acquires and begins a managed frame.
		Status beginFrame(
			const FrameBeginInfo& info,
			Frame* frame);

		/// Acquires and begins a managed frame, returning an invalid frame on failure.
		Frame beginFrame(
			const FrameBeginInfo& info = {});

		/// Resizes presentation resources. Zero dimensions suspend frame acquisition.
		Status resize(
			std::uint32_t width,
			std::uint32_t height);

	private:
		std::unique_ptr<RendererImpl> m_Impl;
	};
} // namespace spall
