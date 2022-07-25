

struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color         : COLOR;
	float2 uv            : TEXCOORD;
};

struct v2p_t
{
	float4 position : SV_Position;
	float4 color    : COLOR;
	float2 uv       : TEXCOORD;
};

struct vs_input_t_lit
{
	float3 localPosition : POSITION;
	float3 localNormal   : NORMAL;
	float4 color         : COLOR;
	float2 uv            : TEXCOORD;
};

struct v2p_t_lit
{
	float4 position : SV_Position;
	float4 normal   : NORMAL;
	float4 color    : COLOR;
	float2 uv       : TEXCOORD;
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

cbuffer LightingConstants : register(b1)
{
    float  AmbientIntensity;
    float  SunIntensity;
    float3 SunDirection;
}

v2p_t VertexMain(vs_input_t input)
{
    float4 localPos = float4(input.localPosition, 1);
    float4 worldPos = mul(ModelMatrix, localPos);

	v2p_t v2p;
	v2p.position = mul(ProjectionMatrix, mul(ViewMatrix, worldPos));
	v2p.color = input.color;
	v2p.uv = input.uv;
	
	return v2p;
}

float4 PixelMain(v2p_t input) : SV_Target0
{
	float4 diffuse = diffuseTexture.Sample(diffuseSampler, input.uv);
	
	float4 color = TintColor * input.color * diffuse;
    // color.xyz = float3(1.0f, 1.0f, 1.0f) - color.xyz;

    if (color.a <= 0)
        discard;
    return color;
}

v2p_t_lit VertexMainLit(vs_input_t_lit input)
{
    float4 localPos = float4(input.localPosition, 1);
    float4 worldPos = mul(ModelMatrix, localPos);
    float4 localNrm = float4(input.localNormal, 1);
    float4 worldNrm = mul(ModelMatrix, localNrm);

	v2p_t_lit v2p;
	v2p.position = mul(ProjectionMatrix, mul(ViewMatrix, worldPos));
	v2p.normal   = mul(ProjectionMatrix, mul(ViewMatrix, worldNrm));
    v2p.color = input.color;
	v2p.uv = input.uv;
	
	return v2p;
}

float4 PixelMainLit(v2p_t_lit input) : SV_Target0
{
    float  ambient     = AmbientIntensity;
    float  directional = SunIntensity * saturate(dot(normalize(input.normal), -SunDirection));
    float4 light       = float4((ambient + directional).xxx, 1);

	float4 diffuse = diffuseTexture.Sample(diffuseSampler, input.uv);
	
	float4 color = TintColor * input.color * diffuse * light;
    // color.xyz = float3(1.0f, 1.0f, 1.0f) - color.xyz;

    if (color.a <= 0)
        discard;
    return color;
}

