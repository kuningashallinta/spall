// Traces a procedural bounding box through a ray-tracing pipeline, exercising
// an intersection shader and an any-hit shader alongside closest hit.

RaytracingAccelerationStructure Scene : register(t0);
RWStructuredBuffer<uint4> Results : register(u1);

struct Payload
{
	uint Hit;
	uint DistanceBits;
	uint InstanceId;
	uint PrimitiveIndex;
};

struct BoxAttributes
{
	float3 Normal;
};

static const float3 Origins[2] = {
	float3(0.25f, 0.25f, -1.0f),
	float3(5.0f, 5.0f, -1.0f)};

static const float3 BoxMin = float3(0.0f, 0.0f, 0.5f);
static const float3 BoxMax = float3(1.0f, 1.0f, 1.5f);

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

[shader("intersection")]
void intersectionMain()
{
	const float3 origin = ObjectRayOrigin();
	const float3 direction = ObjectRayDirection();

	const float3 inverseDirection = 1.0f / direction;
	const float3 first = (BoxMin - origin) * inverseDirection;
	const float3 second = (BoxMax - origin) * inverseDirection;

	const float3 nearest = min(first, second);
	const float3 farthest = max(first, second);

	const float entry = max(max(nearest.x, nearest.y), nearest.z);
	const float exit = min(min(farthest.x, farthest.y), farthest.z);

	if (exit >= max(entry, 0.0f))
	{
		BoxAttributes attributes;
		attributes.Normal = float3(0.0f, 0.0f, -1.0f);

		ReportHit(entry, 0, attributes);
	}
}

[shader("anyhit")]
void anyHitMain(inout Payload payload, in BoxAttributes attributes)
{
	AcceptHitAndEndSearch();
}

[shader("closesthit")]
void closestHitMain(inout Payload payload, in BoxAttributes attributes)
{
	payload.Hit = 1;
	payload.DistanceBits = asuint(RayTCurrent());
	payload.InstanceId = InstanceID();
	payload.PrimitiveIndex = PrimitiveIndex();
}
