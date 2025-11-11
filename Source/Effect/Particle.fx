//ワールドビュー射影変換行列宣言
float4x4 gMatWVP;

//色情報
float4 gColor;

//テクスチャー
texture gTexture;
sampler texSampler = sampler_state
{
	Texture = <gTexture>;

	minFilter = LINEAR;
	magFilter = LINEAR;
	mipFilter = LINEAR;

	AddressU = WRAP;
	AddressV = WRAP;
};

//頂点シェーダー入力用
struct VS_INPUT
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
};

//頂点シェーダー出力用
struct VS_OUTPUT {
	float4 position : POSITION;
	float4 color    : COLOR0;
	float2 texCoord : TEXCOORD0;
};

//ピクセルシェーダー入力用
struct PS_INPUT {
	float4 position : POSITION;
	float4 color    : COLOR0;
	float2 texCoord : TEXCOORD0;
};

//ピクセルシェーダー出力用
struct PS_OUTPUT {
	float4 color : COLOR0;
};

// 頂点シェーダ
VS_OUTPUT ParticleVS(VS_INPUT In)
{
	VS_OUTPUT Out;

	//頂点に変換行列を乗算する
	Out.position = mul(float4(In.position, 1.0f), gMatWVP);

	Out.color = gColor;

	//UV座標の設定
	Out.texCoord = In.texCoord;

	return Out;
}

// ピクセルシェーダ
PS_OUTPUT ParticlePS(PS_INPUT In)
{
	PS_OUTPUT Out;

	//テクスチャーの色を取得
	float4 texColor = tex2D(texSampler, In.texCoord);

	//テクスチャーの色と乗算する
	Out.color = texColor * In.color;

	//アルファは別途設定
	Out.color.a = (texColor.r + texColor.g + texColor.b) * In.color.a;

	return Out;
}

//テクニック
technique ParticleTec
{
   pass P0
   {
      VertexShader = compile vs_3_0 ParticleVS();
      PixelShader = compile ps_3_0 ParticlePS();

	  //アルファブレンド設定
	  AlphaBlendEnable = True;
	  SrcBlend = SRCALPHA;
	  DestBlend = ONE;

	  //カリングOFF
	  CullMode = NONE;
   }
}