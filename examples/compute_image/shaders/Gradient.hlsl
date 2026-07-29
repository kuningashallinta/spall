RWTexture2D<float4> uOutput : register(u0);

cbuffer Constants : register(b0)
{
	float uTime;
}

[numthreads(8, 8, 1)]
void csMain(uint3 id : SV_DispatchThreadID)
{
	uint width;
	uint height;
	uOutput.GetDimensions(width, height);

	if (id.x >= width || id.y >= height)
	{
		return;
	}

	float2 uv = float2(id.xy) / float2(width - 1, height - 1);
	float wave = 0.5 + 0.5 * sin(uTime + (uv.x + uv.y) * 6.2831853);
	uOutput[id.xy] = float4(uv, wave, 1.0);
}
