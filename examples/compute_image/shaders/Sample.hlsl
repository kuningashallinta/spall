Texture2D uTexture : register(t0);
SamplerState uSampler : register(s0);

float4 psMain(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target
{
	return uTexture.Sample(uSampler, uv);
}
