// Traces one ray per dispatch column through a ray-tracing pipeline and
// reports the committed hit, exercising raygen, miss, and closest-hit.

RaytracingAccelerationStructure Scene : register(t0);
RWStructuredBuffer<uint4> Results : register(u1);

struct Payload
{
	uint Hit;
	uint DistanceBits;
	uint InstanceId;
	uint PrimitiveIndex;
};

static const float3 Origins[2] = {
	float3(0.25f, 0.25f, -1.0f),
	float3(5.0f, 5.0f, -1.0f)};

[shader("raygeneration")]
void rayGenMain()
{
	const uint index = DispatchRaysIndex().x;

	RayDesc ray;
	ray.Origin = Origins[index];
	ray.Direction = float3(0.0f, 0.0f, 1.0f);
	ray.TMin = 0.0f;
	ray.TMax = 100.0f;

	Payload payload = {0, 0, 0, 0};
	TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

	Results[index] = uint4(payload.Hit, payload.DistanceBits, payload.InstanceId, payload.PrimitiveIndex);
}

[shader("miss")]
void missMain(inout Payload payload)
{
	payload.Hit = 0;
}

[shader("closesthit")]
void closestHitMain(inout Payload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
	payload.Hit = 1;
	payload.DistanceBits = asuint(RayTCurrent());
	payload.InstanceId = InstanceID();
	payload.PrimitiveIndex = PrimitiveIndex();
}

// A second hit group, selected through the instance contribution, so a trace
// can prove which shader-binding-table record it reached.
[shader("closesthit")]
void closestHitSecondMain(inout Payload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
	payload.Hit = 2;
	payload.DistanceBits = asuint(RayTCurrent());
	payload.InstanceId = InstanceID();
	payload.PrimitiveIndex = PrimitiveIndex();
}
