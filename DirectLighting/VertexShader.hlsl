
struct VS_INPUT
{
  float3 pos : POSITION;
  float4 color: COLOR;
};

struct VS_OUTPUT
{
  float4 pos: SV_POSITION;
  float4 color: COLOR;
};
VS_OUTPUT main(VS_INPUT input)
{
  VS_OUTPUT output;
  output.pos = float4(input.pos, 1.0f);
  output.color = float4(1.0, 0.0, 0.0, 1.0);//input.color;
	return output;
}