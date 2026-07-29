#include <catch2/catch_test_macros.hpp>

#include <spall/Common/Resource/SharedObject.h>

namespace
{
	class TrackedObject final : public spall::SharedObject<spall::IResource>
	{
	public:
		explicit TrackedObject(
			bool& destroyed)
			: m_Destroyed(&destroyed)
		{
		}

		~TrackedObject() override
		{
			*m_Destroyed = true;
		}

		spall::RenderBackendType backendType() const override
		{
			return spall::RenderBackendType::Vulkan;
		}

	private:
		bool* m_Destroyed = nullptr;
	};
} // namespace

TEST_CASE(
	"A shared object deletes itself after its last release",
	"[sharedobject]")
{
	bool destroyed = false;
	TrackedObject* object = new TrackedObject(destroyed);

	CHECK(object->referenceCount() == 0);
	CHECK(object->addRef() == 1);
	CHECK(object->addRef() == 2);
	CHECK(object->referenceCount() == 2);

	CHECK(object->release() == 1);
	CHECK_FALSE(destroyed);

	CHECK(object->release() == 0);
	CHECK(destroyed);
}
