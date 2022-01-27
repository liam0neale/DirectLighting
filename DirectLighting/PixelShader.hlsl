Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

struct VS_OUTPUT
{
	float4 pos: SV_POSITION;
	float2 texCoord: TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	// return interpolated color
	return float4(1.0, 1.0, 0.0, 1.0);//t1.Sample(s1, input.texCoord);
}