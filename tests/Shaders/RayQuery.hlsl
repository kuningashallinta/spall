// Traces one ray per thread against the bound top-level structure and reports
// the committed hit, exercising the whole inline ray-tracing path.
//
// Compiled to DXIL with:
//   dxc -nologo -T cs_6_5 -E csMain RayQuery.hlsl -Fo RayQuery.cso

// Spall RHI flattens a resource set into one register index per binding, so
// binding 0 lands on t0 and binding 1 on u1.
RaytracingAccelerationStructure Scene : register(t0);
RWStructuredBuffer<uint4> Results : register(u1);

// Thread zero aims at the middle of the triangle, thread one aims away from it.
static const float3 Origins[2] = {
	float3(0.25f, 0.25f, -1.0f),
	float3(5.0f, 5.0f, -1.0f)};

[numthreads(2, 1, 1)]
void csMain(uint3 threadId : SV_DispatchThreadID)
{
	const uint index = threadId.x;

	RayDesc ray;
	ray.Origin = Origins[index];
	ray.Direction = float3(0.0f, 0.0f, 1.0f);
	ray.TMin = 0.0f;
	ray.TMax = 100.0f;

	RayQuery<RAY_FLAG_FORCE_OPAQUE> query;
	query.TraceRayInline(Scene, RAY_FLAG_NONE, 0xFF, ray);
	query.Proceed();

	uint hit = 0;
	uint distanceBits = 0;
	uint instanceId = 0;
	uint primitiveIndex = 0;

	if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		hit = 1;
		distanceBits = asuint(query.CommittedRayT());
		instanceId = query.CommittedInstanceID();
		primitiveIndex = query.CommittedPrimitiveIndex();
	}

	Results[index] = uint4(hit, distanceBits, instanceId, primitiveIndex);
}
