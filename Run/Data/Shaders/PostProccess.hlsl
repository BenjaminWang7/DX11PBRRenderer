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

Texture2D tex : register(t0);
SamplerState splr : register(s0);

v2p_t VertexMain( vs_input_t input )
{
	v2p_t v2p;
    v2p.position = float4(input.position, 0.0, 1.0);
    v2p.uv = input.uv;
    return v2p;
}

#define INTENSITY 0.4f
float4 PixelMain(v2p_t input) : SV_Target0
{
    uint width, height;
    tex.GetDimensions(width, height);
    float3 color = tex.Sample(splr, input.uv);
    
    //Pixels towards the edge get darker
    float dist = 1.0f - distance(float2((input.position.x - width / 2) / width, (input.position.y - height / 2) / height) * INTENSITY, float2(0.0f, 0.0f));
    color *= dist;
    
    /* Gamma correction */
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, float3(1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0));


    return float4(color, 1.0f);
}