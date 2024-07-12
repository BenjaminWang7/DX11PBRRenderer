#define NUM_LIGHTS 4
#define PI 3.14159265359
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
	float4 worldPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 tangent : TANGENT;
	float4 bitangent : BITANGENT;
	float4 normal : NORMAL;
};


//------------------------------------------------------------------------------------------------
cbuffer LightConstants : register(b1)
{
	float3 SunDirection;
	float SunIntensity;
	float AmbientIntensity;
	float SpecularIntensity;
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

struct PointLight
{
	float3 LightPosition;
	float PointLightConstant;

	float PointLightAmbientIntensity;
	float PointLightDiffuseIntensity;
	float PointLightSpecularIntensity;

	float PointLightQuadratic;

	float3 PointLightColor;
	float PointLightLinear;
};

cbuffer PointLightConstants : register(b4)
{
	PointLight PointLights[NUM_LIGHTS];
};

cbuffer SpotLightConstants : register(b5)
{
	float3 SpotLightPosition;
	float SpotLightCutOff;
	float3 SpotLightDirection;
	float SpotLightOuterCutOff;

	float SpotLightAmbientIntensity;
	float SpotLightDiffuseIntensity;
	float SpotLightSpecularIntensity;

    float SpotLightConstant;
    float SpotLightLinear;
    float SpotLightQuadratic;
};

cbuffer MaterialConstants: register(b6)
{
	int	MatType;
	float MatAmbient;
	float MatDiffuse;
	float MatSpecular;
	float MatShininess;
	float MatRoughness;
	float MatMetallic;
};

//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D albedoTexture : register(t2);
Texture2D aoTexture : register(t3);
Texture2D metallicTexture : register(t4);
Texture2D roughnessTexture : register(t5);
Texture2D BRDF : register(t6);
TextureCube cubeMap : register(t7);
TextureCube irradianceMap : register(t8);
TextureCube prefilteredMap : register(t9);

//------------------------------------------------------------------------------------------------
SamplerState diffuseSampler : register(s0);

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
	v2p.worldPosition = worldPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.tangent = float4(input.localTangent.xyz, 0);
	v2p.bitangent = float4(input.localBitangent.xyz, 0);
	v2p.normal = worldNormal;
	return v2p;
}

float4 CalcDirLight( float3 normal, float3 viewDir )
{
	//Directional Light-------------------------------------------------------------------
	float3 halfwayDir = normalize(-SunDirection + viewDir);
    // specular shading
    float directionLightSpec = pow(max(dot(normal, halfwayDir), 0.f), MatShininess);
	//Directional Light
	float ambient = AmbientIntensity * MatAmbient;
	float directional = SunIntensity * 	max((dot(normal, -SunDirection)* 0.5f + 0.5f), 0.f) * MatDiffuse;
    float specular = SpecularIntensity * directionLightSpec * MatSpecular;
	//Direction result
	float4 lightColor = float4((ambient + directional + specular).xxx, 1);
	return lightColor;
}
float4 CalcPointLight( float3 normal, float3 fragPos, float3 viewDir)
{
	float4 pointLightColor = float4(0.f, 0.f, 0.f, 0.f);
	for(int i = 0; i < NUM_LIGHTS; i++)
	{
		float3 pointLightDirection = normalize(PointLights[i].LightPosition - fragPos);
		float distanceToPointLight = distance(PointLights[i].LightPosition, fragPos);
		float pointLightAmbient = PointLights[i].PointLightAmbientIntensity * MatAmbient;
		float dotValue = dot(normal, pointLightDirection);
		// specular shading
		float3 halfwayDir = normalize(pointLightDirection + viewDir);
		float pointLightSpec = pow(max(dot(normal, halfwayDir), 0.f), MatShininess);
		float pointLightSpecular = PointLights[i].PointLightSpecularIntensity * pointLightSpec * MatSpecular;
		//Attenuation
		float pointLightDiffuse = PointLights[i].PointLightDiffuseIntensity * saturate(dotValue) * MatDiffuse;
		float attenuation = 1.0 / (PointLights[i].PointLightConstant + PointLights[i].PointLightLinear * distanceToPointLight + PointLights[i].PointLightQuadratic * (distanceToPointLight * distanceToPointLight));
		pointLightAmbient *= attenuation;
		pointLightDiffuse *= attenuation;
		pointLightSpecular *= attenuation;
		pointLightColor += float4((pointLightDiffuse + pointLightAmbient + pointLightSpecular ).xxx, 1);
	}
	return pointLightColor;
}
float4 CalcSpotLight( float3 normal, float3 fragPos, float3 viewDir)
{
	//SpotLight----------------------------------------------------------------------------
	float3 spotLightDir = normalize(SpotLightPosition - fragPos);
    // diffuse shading
    float spotLightDiff = max(dot(normal, spotLightDir), 0.f);
    // attenuation
    float distanceToSpotLight = distance(SpotLightPosition, fragPos);
    float spotLightAttenuation = 1.0 / (SpotLightConstant + SpotLightLinear * distanceToSpotLight + SpotLightQuadratic * (distanceToSpotLight * distanceToSpotLight));    
    // spotlight intensity
    float theta = dot(spotLightDir, normalize(-SpotLightDirection)); 
    float epsilon = SpotLightCutOff - SpotLightOuterCutOff;
    float spotLightIntensity = clamp((theta - SpotLightOuterCutOff) / epsilon, 0.f, 1.f);
	//Specular
	float3 halfwayDir = normalize(spotLightDir + viewDir);
	float spotLightSpec = pow(max(dot(normal, halfwayDir), 0.f), MatShininess);
    // combine results
    float spotLightAmbient = SpotLightAmbientIntensity * MatAmbient;
    float spotLightDiffuse = SpotLightDiffuseIntensity * spotLightDiff * MatDiffuse;
    float spotLightSpecular = SpotLightSpecularIntensity * spotLightSpec * MatSpecular;

    spotLightAmbient *= spotLightAttenuation * spotLightIntensity;
    spotLightDiffuse *= spotLightAttenuation * spotLightIntensity;
    spotLightSpecular *= spotLightAttenuation* spotLightIntensity ;
	float4 spotLightColor = float4((spotLightDiffuse + spotLightAmbient + spotLightSpecular).xxx, 1);
	return spotLightColor;
}

float4 CalcLambertDirLight( float3 normal, float3 viewDir )
{
	//Directional Light-------------------------------------------------------------------
    //specular shading
    float3 directionLightReflectDir = reflect(SunDirection, normal);
	//Directional Light
	float ambient = AmbientIntensity * MatAmbient;
	float directional = SunIntensity * saturate(dot(normal, -SunDirection)) * MatDiffuse;
	//Direction result
	float4 lightColor = float4((ambient + directional).xxx, 1);
	return lightColor;
}
float4 CalcLambertPointLight( float3 normal, float3 fragPos, float3 viewDir)
{
	float4 pointLightColor = float4(0, 0, 0, 0);
	for(int i = 0; i < NUM_LIGHTS; i++)
	{
		float3 pointLightDirection = normalize(PointLights[i].LightPosition - fragPos);
		float distanceToPointLight = distance(PointLights[i].LightPosition, fragPos);
		float pointLightAmbient = PointLights[i].PointLightAmbientIntensity * MatAmbient;
		float dotValue = dot(normal, pointLightDirection);
		//Attenuation
		float pointLightDiffuse = PointLights[i].PointLightDiffuseIntensity * saturate(dotValue) * MatDiffuse;
		float attenuation = 1.0 / (PointLights[i].PointLightConstant + PointLights[i].PointLightLinear * distanceToPointLight + PointLights[i].PointLightQuadratic * (distanceToPointLight * distanceToPointLight));
		pointLightAmbient *= attenuation;
		pointLightDiffuse *= attenuation;
		pointLightColor += float4((pointLightDiffuse + pointLightAmbient ).xxx, 1);
	}
	return pointLightColor;
}
float4 CalcLambertSpotLight( float3 normal, float3 fragPos, float3 viewDir)
{
	//SpotLight----------------------------------------------------------------------------
	float3 spotLightDir = normalize(SpotLightPosition - fragPos);
    // diffuse shading
    float spotLightDiff = max(dot(normal, spotLightDir), 0.f);
    // attenuation
    float distanceToSpotLight = distance(SpotLightPosition, fragPos);
    float spotLightAttenuation = 1.0 / (SpotLightConstant + SpotLightLinear * distanceToSpotLight + SpotLightQuadratic * (distanceToSpotLight * distanceToSpotLight));    
    // spotlight intensity
    float theta = dot(spotLightDir, normalize(-SpotLightDirection)); 
    float epsilon = SpotLightCutOff - SpotLightOuterCutOff;
    float spotLightIntensity = clamp((theta - SpotLightOuterCutOff) / epsilon, 0.f, 1.f);

    // combine results
    float spotLightAmbient = SpotLightAmbientIntensity * MatAmbient;
    float spotLightDiffuse = SpotLightDiffuseIntensity * spotLightDiff * MatDiffuse;

    spotLightAmbient *= spotLightAttenuation * spotLightIntensity;
    spotLightDiffuse *= spotLightAttenuation * spotLightIntensity;
	float4 spotLightColor = float4((spotLightDiffuse + spotLightAmbient).xxx, 1);
	return spotLightColor;
}

float4 CalcWrapDirLight( float3 normal, float3 viewDir )
{
	//Directional Light-------------------------------------------------------------------
    // specular shading
    float3 directionLightReflectDir = reflect(SunDirection, normal);
	//Directional Light
	float ambient = AmbientIntensity * MatAmbient;
	float diffuseReflection = dot(normal, -SunDirection);
	float dotValue = max((diffuseReflection* 0.5f + 0.5f), 0.f);
	float diffuse = SunIntensity * dotValue * MatDiffuse;
	//Direction result
	float4 lightColor = float4((ambient + diffuse).xxx, 1);
	return lightColor;
}

float4 CalcMinnaertDirLight( float3 normal, float3 viewDir )
{
	//Directional Light-------------------------------------------------------------------
    // specular shading
    float3 directionLightReflectDir = reflect(SunDirection, normal);
	//Directional Light
	float ambient = AmbientIntensity * MatAmbient;
	float dotLight = pow(max(dot(normal, -SunDirection), 0.0), 2.f);
	float dotView = max(dot(normal, viewDir), 0.0);
	float dotValue = saturate(dotLight * pow(dotLight * dotView, MatShininess));
	float diffuse = SunIntensity * dotValue * MatDiffuse;
	//Direction result
	float4 lightColor = float4((ambient + diffuse).xxx, 1);
	return lightColor;
}

float4 CalcBandedDirLight( float3 normal, float3 viewDir )
{
	//Directional Light-------------------------------------------------------------------
    // specular shading
    float3 directionLightReflectDir = reflect(SunDirection, normal);
	//Directional Light
	float ambient = AmbientIntensity * MatAmbient;
	float DiffuseReflection = (dot(normal, -SunDirection) + 1.f) * 0.5f;
	float Layered = 4.f;
	float dotValue = floor(DiffuseReflection * Layered) / Layered;
	float diffuse = SunIntensity * dotValue * MatDiffuse;
	
	//Direction result
	float4 lightColor = float4((ambient + diffuse).xxx, 1);
	return lightColor;
}

float FresnelSchlickMethod(float InF0,float3 InObjectPointNormal,float3 InDirection,int InPowM)
{
	return InF0 + (1.f - InF0) * pow(1.f - saturate(dot(InObjectPointNormal, InDirection)),InPowM);
}

float4 CalcFresnelBandedDirLight( float3 normal, float3 viewDir )
{
	//Directional Light-------------------------------------------------------------------
    // specular shading
    float3 directionLightReflectDir = reflect(SunDirection, normal);
	float directionLightSpec = pow(max(dot(viewDir, directionLightReflectDir), 0.0), MatShininess)/0.032f; 
	//Directional Light
	float ambient = AmbientIntensity * MatAmbient;
	float DiffuseReflection = (dot(normal, -SunDirection) + 1.f) * 0.5f;
	float Layered = 4.f;
	float dotValue = floor(DiffuseReflection * Layered) / Layered;
	float diffuse = SunIntensity * dotValue * MatDiffuse;
	//Fresnel
	float F0 = 0.05f;
	float specular = 0.f;
	specular += FresnelSchlickMethod(F0, normal, viewDir, 3);
    specular += SpecularIntensity * directionLightSpec * MatSpecular;
	//Direction result
	float4 lightColor = float4((ambient + diffuse + specular).xxx, 1);
	return lightColor;
}
float4 CalcBackLightDirLight( float3 normal, float3 viewDir )
{
	//Directional Light-------------------------------------------------------------------
    // specular shading
    float3 directionLightReflectDir = reflect(SunDirection, normal);
	float directionLightSpec = saturate(pow(max(dot(viewDir, directionLightReflectDir), 0.f), MatShininess));

	float ambient = AmbientIntensity * MatAmbient;
	//wrap
	float WrapValue = 1.2f;
	float DiffuseReflection = dot(normal, -SunDirection);
	float dotValue = max((DiffuseReflection + WrapValue) / (1.f + WrapValue), 0.0);//[-1,1] => [0,1]
	//SSS simulation
	float SSSValue = 1.3f;
	float TransmissionIntensity = 2.f;
	float TransmissionScope = 3.f;
	float3 LightNormalizeValue = -normalize(normal * SSSValue + SunDirection);
	dotValue = dotValue + pow(saturate(dot(LightNormalizeValue, viewDir)), TransmissionScope) * TransmissionIntensity; 
	float diffuse = SunIntensity * dotValue * MatDiffuse;
	float specular = 0.f;
	float F0 = 0.05f;
	specular += FresnelSchlickMethod(F0, normal, viewDir, 3);
    specular += SpecularIntensity * directionLightSpec * MatSpecular;
	//Direction result
	float4 lightColor = float4((ambient + diffuse + specular).xxx, 1);
	return lightColor;
}

float4 CalcOrenNayarDirLight( float3 normal, float3 viewDir )
{
	//Directional Light
	float ambient = AmbientIntensity * MatAmbient;
	float NormalLight = saturate(pow(max(dot(normal, -SunDirection), 0.0), 2.f));
	float NormalView = saturate(dot(normal, viewDir));

	float Phiri = length(viewDir - normal * NormalView) + length(-SunDirection - normal * NormalLight);

	float ACosNormalView = acos(NormalView);//[0,1]
	float ACosNormalLight = acos(NormalLight);

	float Alpha = max(ACosNormalView, ACosNormalLight);
	float Beta = min(ACosNormalView, ACosNormalLight);

	float MatRoughness = 1.f -  saturate(MatShininess/100.f);
	float MyRoughness = pow(MatRoughness, 2);

	float A = 1 - 0.5f * (MyRoughness / (MyRoughness + 0.33f));
	float B = 0.45f * (MyRoughness / (MyRoughness + 0.09f));

	float dotValue = NormalLight * (A + B * max(0, Phiri) * sin(Alpha) * tan(Beta));
	float diffuse = SunIntensity * dotValue * MatDiffuse;
	
	//Direction result
	float4 lightColor = float4((ambient + diffuse).xxx, 1);
	return lightColor;
}

//PBR---------------------------------------------------------------------------------

// ----------------------------------------------------------------------------
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    //return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float4 CalcPBRPointLight( float3 normal, float3 fragPos, float3 viewDir, v2p_t input )
{
	float4 pointLightColor = float4(0.f, 0.f, 0.f, 0.f);
	//float3 albedo = float3(0.5f, 0.0f, 0.0f);
	//float ao = 1.f;
    float3 albedo = pow(albedoTexture.Sample(diffuseSampler, input.uv).xyz, float3(2.2f, 2.2f, 2.2f));
	float ao = aoTexture.Sample(diffuseSampler, input.uv).x;
	
	//float metallic = metallicTexture.Sample(diffuseSampler, input.uv).x;
	//float roughness = roughnessTexture.Sample(diffuseSampler, input.uv).x;
	float metallic = MatMetallic;	
	float roughness = MatRoughness;

	float3 N = normal;
	float3 V = viewDir;
	float3 F0 = float3(0.04f, 0.04f, 0.04f );
	F0 = lerp(F0, albedo, metallic);
	float3 Lo = float3(0.f, 0.f, 0.f);
	for(int i = 0; i < NUM_LIGHTS; i++)
	{
		float3 L = normalize(PointLights[i].LightPosition - fragPos);
		float3 H = normalize(L + V);
		float distanceToPointLight = distance(PointLights[i].LightPosition, fragPos);
		float attenuation = 1.f / ( distanceToPointLight * distanceToPointLight );
		float3 radiance = PointLights[i].PointLightColor * attenuation;
		
		float NDF = DistributionGGX(N, H, roughness);   
		float G = GeometrySmith(N, V, L, roughness);      
		float3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

		float3 numerator = NDF * G * F; 
		float denominator = 4.0 * max(dot(N, V), 0.f) * max(dot(N, L), 0.f) + 0.0001f;
		float3 specular = numerator / denominator;
		float3 kS = F;
		float3 kD =  float3(1.f, 1.f, 1.f) - kS;
		kD *= 1.0 - metallic;
		float NdotL = max(dot(N, L), 0.f);      

		Lo += (kD * albedo / 3.14159265f + specular) * radiance * NdotL;
	}
	float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao;
	float3 color = ambient + Lo;

	color = color / (color + float3(1.f, 1.f, 1.f));

	color = pow(color, float3(1.f/2.2f, 1.f/2.2f, 1.f/2.2f)); 

	pointLightColor = float4(color.xyz, 1);
	return pointLightColor;
}

float4 CalcTexturedPBRPointLight( float3 normal, float3 fragPos, float3 viewDir, v2p_t input )
{
	float4 pointLightColor = float4(0.f, 0.f, 0.f, 0.f);
	//float3 albedo = float3(0.5f, 0.0f, 0.0f);
	//float ao = 1.f;
    float3 albedo = pow(albedoTexture.Sample(diffuseSampler, input.uv).xyz, float3(2.2f, 2.2f, 2.2f));
	float ao = aoTexture.Sample(diffuseSampler, input.uv).x;
	float metallic = metallicTexture.Sample(diffuseSampler, input.uv).x;
	float roughness = roughnessTexture.Sample(diffuseSampler, input.uv).x;

	float3 N = normal;
	float3 V = viewDir;
	float3 F0 = float3(0.04f, 0.04f, 0.04f );
	F0 = lerp(F0, albedo, metallic);
	float3 Lo = float3(0.f, 0.f, 0.f);
	for(int i = 0; i < NUM_LIGHTS; i++)
	{
		float3 L = normalize(PointLights[i].LightPosition - fragPos);
		float3 H = normalize(L + V);
		float distanceToPointLight = distance(PointLights[i].LightPosition, fragPos);
		float attenuation = 1.f / ( distanceToPointLight * distanceToPointLight );
		float3 radiance = PointLights[i].PointLightColor * attenuation;
		
		float NDF = DistributionGGX(N, H, roughness);   
		float G = GeometrySmith(N, V, L, roughness);      
		float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);


		float3 kS = F;
		float3 kD =  float3(1.f, 1.f, 1.f) - kS;
		kD *= 1.0 - metallic;

		float3 numerator = NDF * G * F; 
		float denominator = 4.0 * max(dot(N, V), 0.f) * max(dot(N, L), 0.f) + 0.0001f;
		float3 specular = numerator / denominator;

		float NdotL = max(dot(N, L), 0.f);      

		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}
	float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * ao;
	float3 color = ambient + Lo;

	color = color / (color + float3(1.f, 1.f, 1.f));

	color = pow(color, float3(1.f/2.2f, 1.f/2.2f, 1.f/2.2f)); 

	pointLightColor = float4(color.xyz, 1.f);
	return pointLightColor;
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   

float4 CalcIBLPBRPointLight( float3 texNormal, float3 fragPos, float3 viewDir, v2p_t input )
{
    float3 albedo = albedoTexture.Sample(diffuseSampler, input.uv);
    float metallic = metallicTexture.Sample(diffuseSampler, input.uv).r;
    float roughness = roughnessTexture.Sample(diffuseSampler, input.uv).r;
    float ao = aoTexture.Sample(diffuseSampler, input.uv).r;


    float3 N = texNormal;
    float3 V = viewDir;
    float3 R = reflect(-V, N);

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);

	float3 Lo = float3(0.f, 0.f, 0.f);
	for(int i = 0; i < NUM_LIGHTS; i++)
	{
		float3 L = normalize(PointLights[i].LightPosition - fragPos);
		float3 H = normalize(L + V);
		float distanceToPointLight = distance(PointLights[i].LightPosition, fragPos);
		float attenuation = 1.f / ( distanceToPointLight * distanceToPointLight );
		float3 radiance = PointLights[i].PointLightColor * attenuation;
		
		float NDF = DistributionGGX(N, H, roughness);   
		float G = GeometrySmith(N, V, L, roughness);      
		float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		float3 kS = F;
		float3 kD =  float3(1.f, 1.f, 1.f) - kS;
		kD *= 1.0 - metallic;

		float3 numerator = NDF * G * F; 
		float denominator = 4.0 * max(dot(N, V), 0.f) * max(dot(N, L), 0.f) + 0.0001f;
		float3 specular = numerator / denominator;

		float NdotL = max(dot(N, L), 0.f);      

		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}


    float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    float3 irradiance = irradianceMap.Sample(diffuseSampler, N).rgb;
    float3 diffuse = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    float3 prefilteredColor = prefilteredMap.SampleLevel(diffuseSampler, R, roughness * MAX_REFLECTION_LOD).rgb;
    float2 envBRDF = BRDF.Sample(diffuseSampler, float2(max(dot(N, V), 0.0), roughness)).rg;
    // Environment Bidirectional reflectance distribution function
    float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    float3 ambient = (kD * diffuse + specular) * ao;

    //float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;
    float3 color = ambient +Lo;

    return float4(color, 1.0f);
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
	float3 viewDir = normalize(CameraPosition - input.worldPosition);
	float3 normal = normalize(input.normal.xyz);

	float4 directionalLightColor = float4(0.f, 0.f, 0.f, 0.f);
	float4 pointLightColor = float4(0.f, 0.f, 0.f, 0.f);
	float4 spotLightColor = float4(0.f, 0.f, 0.f, 0.f);

	float4 bumpMap;
    float3 bumpNormal;
    // Sample the pixel from the normal map.
    bumpMap = normalTexture.Sample(diffuseSampler, input.uv);
	
    // Expand the range of the normal value from (0, +1) to (-1, +1).
    bumpMap = (bumpMap * 2.0f) - 1.0f;
	
    // Calculate the normal from the data in the normal map.
    bumpNormal = (bumpMap.x * input.tangent) + (bumpMap.y * input.bitangent) + (bumpMap.z * normal);
	
    // Normalize the resulting bump normal.
    bumpNormal = normalize(bumpNormal);

	normal = bumpNormal;

	if (MatType == 0)//Lambert
	{
		directionalLightColor = CalcLambertDirLight( normal, viewDir );
		spotLightColor = CalcLambertSpotLight( normal, input.worldPosition, viewDir );
		pointLightColor = CalcLambertPointLight( normal, input.worldPosition, viewDir );
	}
	else if (MatType == 1)//BlinnPhong
	{
		directionalLightColor = CalcDirLight( normal, viewDir );
		spotLightColor = CalcSpotLight( normal, input.worldPosition, viewDir );
		pointLightColor = CalcPointLight( normal, input.worldPosition, viewDir );
	}
	else if (MatType == 2)//WrapLight
	{
		directionalLightColor = CalcWrapDirLight(normal, viewDir);
	}
	else if (MatType == 3)//Minnaert
	{
		directionalLightColor = CalcMinnaertDirLight(normal, viewDir);
	}
	else if (MatType == 4)//Banded
	{
		directionalLightColor = CalcBandedDirLight(normal, viewDir);
	}
	else if (MatType == 5)//FresnelBanded
	{
		directionalLightColor = CalcFresnelBandedDirLight(normal, viewDir);
	}
	else if (MatType == 6)//BackLight
	{
		directionalLightColor = CalcBackLightDirLight(normal, viewDir);
	}
	else if (MatType == 7)//OrenNayer
	{
		directionalLightColor = CalcOrenNayarDirLight(normal, viewDir);
	}
	else if (MatType == 8)//PBR
	{
		pointLightColor = CalcPBRPointLight(normal, input.worldPosition, viewDir, input);
	}
	else if (MatType == 9)//TexturedPBR
	{
		pointLightColor = CalcTexturedPBRPointLight(normal, input.worldPosition, viewDir, input);
	}
	else if (MatType == 10)//IBLPBR
	{
		pointLightColor = CalcIBLPBRPointLight(normal, input.worldPosition, viewDir, input);
	}
	float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;
	float4 color = (directionalLightColor + pointLightColor + spotLightColor)* textureColor * vertexColor * modelColor;
	clip(color.a - 0.01f);
	return color;
}