#include <catch2/catch_test_macros.hpp>

#include <spall/Common/Resource/Resource.h>

#include <cstdint>
#include <utility>

namespace
{
	class CountingResource final : public spall::IResource
	{
	public:
		std::uint32_t addRef() override
		{
			return ++m_ReferenceCount;
		}

		std::uint32_t release() override
		{
			return --m_ReferenceCount;
		}

		spall::RenderBackendType backendType() const override
		{
			return spall::RenderBackendType::Vulkan;
		}

		std::uint32_t referenceCount() const
		{
			return m_ReferenceCount;
		}

	private:
		std::uint32_t m_ReferenceCount = 0;
	};
} // namespace

TEST_CASE(
	"A default resource is empty",
	"[resource]")
{
	const spall::Resource<spall::IResource> resource;

	CHECK_FALSE(resource);
	CHECK(resource.get() == nullptr);
}

TEST_CASE(
	"A resource acquires and releases its pointer",
	"[resource]")
{
	CountingResource object;

	{
		const spall::Resource<spall::IResource> resource(&object);

		CHECK(resource);
		CHECK(resource.get() == &object);
		CHECK(&*resource == &object);
		CHECK(resource->backendType() == spall::RenderBackendType::Vulkan);
		CHECK(object.referenceCount() == 1);
	}

	CHECK(object.referenceCount() == 0);
}

TEST_CASE(
	"Copying a resource retains shared ownership",
	"[resource]")
{
	CountingResource object;

	{
		spall::Resource<spall::IResource> first(&object);

		{
			spall::Resource<spall::IResource> second(first);
			spall::Resource<spall::IResource> third;
			third = first;

			CHECK(first.get() == &object);
			CHECK(second.get() == &object);
			CHECK(third.get() == &object);
			CHECK(object.referenceCount() == 3);

			third = third;
			CHECK(object.referenceCount() == 3);
		}

		CHECK(object.referenceCount() == 1);
	}

	CHECK(object.referenceCount() == 0);
}

TEST_CASE(
	"Moving a resource transfers ownership",
	"[resource]")
{
	CountingResource firstObject;
	CountingResource secondObject;

	{
		spall::Resource<spall::IResource> first(&firstObject);
		spall::Resource<spall::IResource> second(&secondObject);
		spall::Resource<spall::IResource> moved(std::move(first));

		CHECK_FALSE(first);
		CHECK(moved.get() == &firstObject);
		CHECK(firstObject.referenceCount() == 1);

		moved = std::move(second);

		CHECK_FALSE(second);
		CHECK(moved.get() == &secondObject);
		CHECK(firstObject.referenceCount() == 0);
		CHECK(secondObject.referenceCount() == 1);
	}

	CHECK(secondObject.referenceCount() == 0);
}

TEST_CASE(
	"Resetting a resource replaces or clears ownership",
	"[resource]")
{
	CountingResource firstObject;
	CountingResource secondObject;
	spall::Resource<spall::IResource> resource(&firstObject);

	resource.reset(&firstObject);
	CHECK(firstObject.referenceCount() == 1);

	resource.reset(&secondObject);
	CHECK(resource.get() == &secondObject);
	CHECK(firstObject.referenceCount() == 0);
	CHECK(secondObject.referenceCount() == 1);

	resource.reset();
	CHECK_FALSE(resource);
	CHECK(secondObject.referenceCount() == 0);
}
