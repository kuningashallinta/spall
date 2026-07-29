#pragma once

#include <spall/Common/Resource/IResource.h>

#include <spall/CommandList/ICommandList.h>
#include <spall/Common/Enums/QueueType.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>
#include <spall/Device/DeviceLimits.h>
#include <spall/Device/FormatCapabilities.h>
#include <spall/Device/IPipelineFactory.h>
#include <spall/Device/IPresentationFactory.h>
#include <spall/Device/IResourceFactory.h>

namespace spall
{
	class IGraphicsQueue;
	class IQueue;

	/// Represents a logical graphics device.
	///
	/// Device and child-object operations require external synchronization,
	/// including creation, writes, command recording, queue operations, and
	/// presentation.
	class IDevice : public IResource
	{
	public:
		/// Gets immutable limits for the lifetime of this device.
		virtual const DeviceLimits& limits(void) const = 0;

		/// Queries effective portable support for one format.
		virtual Status queryFormatCapabilities(
			Format format,
			FormatCapabilities* capabilities) const = 0;

		virtual IGraphicsQueue& graphicsQueue(void) = 0;
		virtual IQueue& computeQueue(void) = 0;

		/// Creates a command list for the given queue, returning an empty resource on failure.
		Resource<ICommandList> createCommandList(
			QueueType type = QueueType::Graphics);

		/// Creates a graphics-queue command list into the provided resource.
		Status createCommandList(
			Resource<ICommandList>* commandList);

		virtual Status createCommandList(
			QueueType type,
			Resource<ICommandList>* commandList) = 0;

		virtual IResourceFactory& resources(void) = 0;
		virtual IPipelineFactory& pipelines(void) = 0;
		virtual IPresentationFactory& presentation(void) = 0;
	};
} // namespace spall

#include <spall/Device/IDevice.inl>
