

//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 localTangent : TANGENT;
	float3 localBitangent : BITANGENT;
	float3 localNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 position : SV_Position;
	float4 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 tangent : TANGENT;
	float4 bitangent : BITANGENT;
	float4 normal : NORMAL;
};

//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrix;
	float3 CameraPosition;
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};

cbuffer SkyBoxTexConstants : register(b9)
{
	float4 TexDimensions;
};

//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
Texture2D HDRTexture : register(t6);

//------------------------------------------------------------------------------------------------
SamplerState diffuseSampler : register(s0);


const float2 invAtan = float2(0.1591f, 0.3183f);
float2 SampleSphericalMap(float3 v)
{
    float2 uv = float2(atan2(-v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5f;
    return uv;
}

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	float4 localPosition = float4(input.localPosition, 1);
	float4 worldPosition = mul(ModelMatrix, localPosition);
	float4 viewPosition = mul(ViewMatrix, worldPosition);
	float4 clipPosition = mul(ProjectionMatrix, viewPosition);
	float4 localNormal = float4(input.localNormal, 0);
	float4 worldNormal = mul(ModelMatrix, localNormal);

	v2p_t v2p;
	v2p.position = clipPosition;
	v2p.localPosition = localPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.tangent = float4(input.localTangent.xyz, 0);
	v2p.bitangent = float4(input.localBitangent.xyz, 0);
	v2p.normal = worldNormal;
	return v2p;
}



//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
	float kernel = 25.f;
	float averageValue = 1.f/kernel;
	float2 texOffset = 1.0f/TexDimensions.xy;
	float4 convolutionColor = float4(0.0, 0.0, 0.0, 0.0);
    for (int y = -2; y <= 2; y++) 
	{
        for (int x = -2; x <= 2; x++) 
		{
            float2 sampleCoord = input.uv + float2(x, y) * texOffset;
            convolutionColor += diffuseTexture.Sample(diffuseSampler, sampleCoord) * averageValue;
        }
    }
	
	float4 textureColor =  diffuseTexture.Sample(diffuseSampler, input.uv);;
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;
	float4 color = convolutionColor * vertexColor * modelColor;
	clip(color.a - 0.01f);
	return color;
}