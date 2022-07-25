struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color         : COLOR;
	float2 uv            : TEXCOORD;
};

struct v2p_t
{
	float4 position : SV_Position;
	float4 wpo      : WorldPos;
	float4 color    : COLOR;
	float2 uv       : TEXCOORD;
};

struct ps_output_t
{
	float4 color : SV_Target0;
	float4 depth : SV_Target1;
};

Texture2D       diffuseTexture  : register(t0);
SamplerState    diffuseSampler  : register(s0);

cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4   TintColor;
}

cbuffer CameraConstants : register(b2)
{
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrix;
}

cbuffer EnvironmentConstants : register(b5)
{
	float4 G_SkyColor;
	float4 G_SkyLight;
	float4 G_GlowLight;
	float3 G_CameraWorldPos;
	float  G_WorldTime;
	float  G_Lightning;
	float  G_Flicker;
	float  G_FogFar;
	float  G_FogNear;
}

v2p_t VertexMain(vs_input_t input)
{
    float4 localPos = float4(input.localPosition, 1);
    float4 worldPos = mul(ModelMatrix, localPos);

	v2p_t v2p;
	v2p.position = mul(ProjectionMatrix, mul(ViewMatrix, worldPos));
	v2p.wpo      = worldPos;
    v2p.color    = input.color;
	v2p.uv       = input.uv;
	
	return v2p;
}

float3 SoftBlend(float3 a, float3 b)
{
	return 1.0f - (1.0f - a) * (1.0f - b);
}

ps_output_t PixelMain(v2p_t input)
{
	// diffuse
	float4 diffuse = diffuseTexture.Sample(diffuseSampler, input.uv);

	// Compute lit pixel color
	float  iLightLevel = input.color.r; // indoor (block light)
	float  oLightLevel = input.color.g * 0.5f + 0.5f; // outdoor (sky light)
	float  aLightLevel = input.color.b; // ambient (block face light)

	float3 iLight = iLightLevel * G_GlowLight.rgb * G_Flicker;
	float3 oLight = oLightLevel * G_SkyLight.rgb;
	float3 light = SoftBlend(iLight, oLight) * aLightLevel;

	float4 color = TintColor * diffuse * float4(light, 1.0f);

	// Compute the fog
	float3 cameraOffset = input.wpo.xyz - G_CameraWorldPos;
	float  cameraDist = length(cameraOffset);
	float  fogDensity = saturate((cameraDist - G_FogNear) / (G_FogFar - G_FogNear));
	
	// apply fog
	float3 fogRGB = lerp(color.rgb, G_SkyColor.rgb, fogDensity);
	float  fogA   = saturate(color.a + fogDensity); // calc alpha sperately
	color = float4(fogRGB, fogA);

	ps_output_t output;
	output.color = color;
	output.depth = cameraDist / (G_FogFar + 16.0f);

    return output;
}

