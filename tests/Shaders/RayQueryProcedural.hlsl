// Traces one ray per thread against a procedural bounding box, intersecting it
// in the shader and committing the entry distance.

RaytracingAccelerationStructure Scene : register(t0);
RWStructuredBuffer<uint4> Results : register(u1);

static const float3 Origins[2] = {
	float3(0.25f, 0.25f, -1.0f),
	float3(5.0f, 5.0f, -1.0f)};

static const float3 BoxMin = float3(0.0f, 0.0f, 0.5f);
static const float3 BoxMax = float3(1.0f, 1.0f, 1.5f);

bool intersectBox(float3 origin, float3 direction, out float entry)
{
	const float3 inverseDirection = 1.0f / direction;
	const float3 first = (BoxMin - origin) * inverseDirection;
	const float3 second = (BoxMax - origin) * inverseDirection;

	const float3 nearest = min(first, second);
	const float3 farthest = max(first, second);

	entry = max(max(nearest.x, nearest.y), nearest.z);

	const float exit = min(min(farthest.x, farthest.y), farthest.z);

	return (exit >= max(entry, 0.0f));
}

[numthreads(2, 1, 1)]
void csMain(uint3 threadId : SV_DispatchThreadID)
{
	const uint index = threadId.x;

	RayDesc ray;
	ray.Origin = Origins[index];
	ray.Direction = float3(0.0f, 0.0f, 1.0f);
	ray.TMin = 0.0f;
	ray.TMax = 100.0f;

	RayQuery<RAY_FLAG_NONE> query;
	query.TraceRayInline(Scene, RAY_FLAG_NONE, 0xFF, ray);

	while (query.Proceed())
	{
		if (query.CandidateType() == CANDIDATE_PROCEDURAL_PRIMITIVE)
		{
			float entry = 0.0f;

			if (intersectBox(ray.Origin, ray.Direction, entry))
			{
				query.CommitProceduralPrimitiveHit(entry);
			}
		}
	}

	uint hit = 0;
	uint distanceBits = 0;
	uint instanceId = 0;
	uint primitiveIndex = 0;

	if (query.CommittedStatus() == COMMITTED_PROCEDURAL_PRIMITIVE_HIT)
	{
		hit = 1;
		distanceBits = asuint(query.CommittedRayT());
		instanceId = query.CommittedInstanceID();
		primitiveIndex = query.CommittedPrimitiveIndex();
	}

	Results[index] = uint4(hit, distanceBits, instanceId, primitiveIndex);
}
