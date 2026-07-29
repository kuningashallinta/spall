struct VertexOutput
{
	float4 Position : SV_Position;
	float2 UV : TEXCOORD0;
};

VertexOutput vsMain(uint vertexId : SV_VertexID)
{
	VertexOutput output;
	output.UV = float2((vertexId << 1) & 2, vertexId & 2);
	output.Position = float4(output.UV * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

	return output;
}
