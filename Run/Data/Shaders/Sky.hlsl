struct vs_input_t
{
	float3 position : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct v2p_t
{
	float4 position : SV_Position;
	float3 worldPos : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrix;
	float3 CameraPosition;
};

v2p_t VertexMain( vs_input_t input )
{
	v2p_t v2p;

    float4x4 v = ViewMatrix;
    v[3][0] = 0;
    v[3][1] = 0;
    v[3][2] = 0;

    v2p.worldPos = input.position;
    v2p.position = mul(float4(input.position, 1.0), mul(v, ProjectionMatrix));
    v2p.position.z = v2p.position.w;
    return v2p;
}

TextureCube tex : register(t7);
//Texture2D tex : register(t0);
SamplerState splr : register(s0);

float2 SampleSphericalMap(float3 v)
{
    float2 invAtan = float2(0.1591, 0.3183);
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    float2 uv = SampleSphericalMap(normalize(input.worldPos)); // make sure to normalize localPos
    float3 color = tex.Sample(splr, normalize(input.worldPos)).rgb;
    //color = float3(1.0,0.0,0.0);
    //return tex.Sample(splr, input.worldPos);
    return float4(color, 1.0f);
}