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
	float  depth : SV_Depth;
};

Texture2D       diffuseTexture  : register(t0);
SamplerState    diffuseSampler  : register(s0);

Texture2D       worldPass       : register(t1);
Texture2D       fluidPass       : register(t2);
Texture2D       worldDist       : register(t3);
Texture2D       fluidDist       : register(t4);
Texture2D       noiseTex        : register(t5);

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
	float  G_Water; // Was Lighting in game code
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
	float3 noise = noiseTex.Sample(diffuseSampler, float2(input.uv.x, input.uv.y + G_WorldTime * 100.0f)).xyz * 2.0f - 1.0f;

	float2 uv = lerp(input.uv, saturate(input.uv + 0.01f * noise.xy), G_Water);

	// samples
	float4 world = worldPass.Sample(diffuseSampler, uv);
	float  worldDepth = worldDist.Sample(diffuseSampler, uv).r;
	float4 fluid = fluidPass.Sample(diffuseSampler, uv);
	float  fluidDepth = fluidDist.Sample(diffuseSampler, uv).r;

	// calc combine color
	float waterDepth = saturate((worldDepth - fluidDepth) * 4.0f);

	float4 waterColor;
	waterColor = lerp(world, fluid, lerp(0.3f, 1.0f, waterDepth));
	waterColor.b = saturate(waterColor.b * 1.25f);

	ps_output_t output;
	output.color = lerp(world, waterColor, step(fluidDepth, worldDepth));
	output.color.a = world.a;

	output.color *= TintColor;
	// output.color.r = saturate(waterDepth);
	output.depth = worldDepth;

	return output;
}

