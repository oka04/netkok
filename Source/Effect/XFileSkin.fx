//ビュー射影変換行列宣言
float4x4 gMatVP;

//合成行列
float4x4 gMatW[4];

//頂点に影響を与えるボーンの数
int gNumMaxInfle;

//アンビエントライトの色
float4 gAmbientColor;

//ディレクショナルライトの方向
float3 gLightDir;

//ディレクショナルライトの色
float4 gLightColor;

//マテリアルの色
float4 gMaterialDiffuse;

//マテリアルのアンビエント
float4 gMaterialAmbient;

//テクスチャー
texture gTexture;
sampler texSampler = sampler_state
{
	Texture = <gTexture>;

	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;

	AddressU = WRAP;
	AddressV = WRAP;
};

//頂点シェーダー入力用
struct VS_INPUT_PB1NT
{
	float3 position : POSITION;
	float  blend    : BLENDWEIGHT;
	float3 normal   : NORMAL;
	float2 texCoord : TEXCOORD0;
};

struct VS_INPUT_PB3NT
{
	float3 position : POSITION;
	float3 blend    : BLENDWEIGHT;
	float3 normal   : NORMAL;
	float2 texCoord : TEXCOORD0;
};

struct VS_INPUT_PB1NCT
{
	float3 position : POSITION;
	float  blend : BLENDWEIGHT;
	float3 normal   : NORMAL;
	float4 color    : COLOR0;
	float2 texCoord : TEXCOORD0;
};

struct VS_INPUT_PB2NCT
{
	float3 position : POSITION;
	float2 blend    : BLENDWEIGHT;
	float3 normal   : NORMAL;
	float4 color    : COLOR0;
	float2 texCoord : TEXCOORD0;
};

struct VS_INPUT_PB3NCT
{
	float3 position : POSITION;
	float3 blend    : BLENDWEIGHT;
	float3 normal   : NORMAL;
	float4 color    : COLOR0;
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
VS_OUTPUT TextureVS_PB1NT(VS_INPUT_PB1NT In)
{
	VS_OUTPUT Out;

	float4x4 comb = (float4x4)0;

	//合成行列にブレンド値を加えワールド座標変換行列を作成
	int i;
	for (i = 0; i < gNumMaxInfle - 1; i++) {
		comb += gMatW[i] * In.blend;
	}
	comb += gMatW[i] * (1.0f - In.blend);

	//頂点を調整
	Out.position = mul(float4(In.position, 1.0f), comb);
	Out.position = mul(Out.position, gMatVP);

	//法線を調整
	float4 normal = mul(float4(In.normal, 0.0f), comb);

	//ディレクショナルライトの方向を逆にする（次の計算のため）
	float4 revLightDir = -float4(gLightDir, 0.0f);

	//法線とディレクショナルライトの方向の内積にディレクショナルライトの色を乗算しアンビエントライトの色と加算
	//saturate　→　結果を0.0から1.0までの範囲に収める
	Out.color = saturate(gAmbientColor * gMaterialAmbient + gLightColor * gMaterialDiffuse * max(0, dot(revLightDir, normal)));

	//UV座標の設定
	Out.texCoord = In.texCoord;

	return Out;
}

// 頂点シェーダ
VS_OUTPUT TextureVS_PB3NT(VS_INPUT_PB3NT In)
{
	VS_OUTPUT Out;

	float blend[3] = (float[3])In.blend;

	float4x4 comb = (float4x4)0;

	//合成行列にブレンド値を加えワールド座標変換行列を作成
	int i;
	for (i = 0; i < gNumMaxInfle - 1; i++) {
		comb += gMatW[i] * blend[i];
	}
	comb += gMatW[i] * (1.0f - blend[0] - blend[1] - blend[2]);

	//頂点を調整
	Out.position = mul(float4(In.position, 1.0f), comb);
	Out.position = mul(Out.position, gMatVP);

	//法線を調整
	float4 normal = mul(float4(In.normal, 0.0f), comb);

	//ディレクショナルライトの方向を逆にする（次の計算のため）
	float4 revLightDir = -float4(gLightDir, 0.0f);

	//法線とディレクショナルライトの方向の内積にディレクショナルライトの色を乗算しアンビエントライトの色と加算
	//saturate　→　結果を0.0から1.0までの範囲に収める
	Out.color = saturate(gAmbientColor * gMaterialAmbient + gLightColor * gMaterialDiffuse * max(0, dot(revLightDir, normal)));

	//UV座標の設定
	Out.texCoord = In.texCoord;

	return Out;
}

VS_OUTPUT TextureVS_PB1NCT(VS_INPUT_PB1NCT In)
{
	VS_OUTPUT Out;

	float4x4 comb = (float4x4)0;

	//合成行列にブレンド値を加えワールド座標変換行列を作成
	int i;
	for (i = 0; i < gNumMaxInfle - 1; i++) {
		comb += gMatW[i] * In.blend;
	}
	comb += gMatW[i] * (1.0f - In.blend);
	
	//頂点を調整
	Out.position = mul(float4(In.position, 1.0f), comb);
	Out.position = mul(Out.position, gMatVP);

	//法線を調整
	float4 normal = mul(float4(In.normal, 0.0f), comb);

	//ディレクショナルライトの方向を逆にする（次の計算のため）
	float4 revLightDir = -float4(gLightDir, 0.0f);

	//法線とディレクショナルライトの方向の内積にディレクショナルライトの色を乗算しアンビエントライトの色と加算
	//saturate　→　結果を0.0から1.0までの範囲に収める
	Out.color = saturate(gAmbientColor * gMaterialAmbient + gLightColor * gMaterialDiffuse * max(0, dot(revLightDir, normal)) * In.color);

	//UV座標の設定
	Out.texCoord = In.texCoord;

	return Out;
}

VS_OUTPUT TextureVS_PB2NCT(VS_INPUT_PB2NCT In)
{
	VS_OUTPUT Out;

	float blend[2] = (float[2])In.blend;

	float4x4 comb = (float4x4)0;

	//合成行列にブレンド値を加えワールド座標変換行列を作成
	int i;
	for (i = 0; i < gNumMaxInfle - 1; i++) {
		comb += gMatW[i] * In.blend[i];
	}
	comb += gMatW[i] * (1.0f - In.blend[0] - In.blend[1]);

	//頂点を調整
	Out.position = mul(float4(In.position, 1.0f), comb);
	Out.position = mul(Out.position, gMatVP);

	//法線を調整
	float4 normal = mul(float4(In.normal, 0.0f), comb);

	//ディレクショナルライトの方向を逆にする（次の計算のため）
	float4 revLightDir = -float4(gLightDir, 0.0f);

	//法線とディレクショナルライトの方向の内積にディレクショナルライトの色を乗算しアンビエントライトの色と加算
	//saturate　→　結果を0.0から1.0までの範囲に収める
	Out.color = saturate(gAmbientColor * gMaterialAmbient + gLightColor * gMaterialDiffuse * max(0, dot(revLightDir, normal)) * In.color);

	//UV座標の設定
	Out.texCoord = In.texCoord;

	return Out;
}

VS_OUTPUT TextureVS_PB3NCT(VS_INPUT_PB3NCT In)
{
	VS_OUTPUT Out;

	float blend[3] = (float[3])In.blend;

	float4x4 comb = (float4x4)0;

	//合成行列にブレンド値を加えワールド座標変換行列を作成
	int i;
	for (i = 0; i < gNumMaxInfle - 1; i++) {
		comb += gMatW[i] * In.blend[i];
	}
	comb += gMatW[i] * (1.0f - In.blend[0] - In.blend[1] - In.blend[2]);

	//頂点を調整
	Out.position = mul(float4(In.position, 1.0f), comb);
	Out.position = mul(Out.position, gMatVP);

	//法線を調整
	float4 normal = mul(float4(In.normal, 0.0f), comb);

	//ディレクショナルライトの方向を逆にする（次の計算のため）
	float4 revLightDir = -float4(gLightDir, 0.0f);

	//法線とディレクショナルライトの方向の内積にディレクショナルライトの色を乗算しアンビエントライトの色と加算
	//saturate　→　結果を0.0から1.0までの範囲に収める
	Out.color = saturate(gAmbientColor * gMaterialAmbient + gLightColor * gMaterialDiffuse * max(0, dot(revLightDir, normal)) * In.color);

	//UV座標の設定
	Out.texCoord = In.texCoord;

	return Out;
}

// ピクセルシェーダ
PS_OUTPUT TexturePS(PS_INPUT In)
{
	PS_OUTPUT Out;

	//テクスチャーの色を取得
	float4 texColor = tex2D(texSampler, In.texCoord);

	//ライティング済みの色とテクスチャーの色を乗算する
	Out.color = texColor * In.color;

	return Out;
}

// ピクセルシェーダ（テクスチャーなし）
PS_OUTPUT NonTexturePS(PS_INPUT In)
{
	PS_OUTPUT Out;

	Out.color = In.color;

	return Out;
}

//テクニック
technique XFileSkinTec
{
	pass P0
	{
		VertexShader = compile vs_3_0 TextureVS_PB1NT();
		PixelShader = compile ps_3_0 TexturePS();

		//アルファテスト
		AlphaTestEnable = True;
		AlphaFunc = GREATEREQUAL;
		AlphaRef = 0xC8;

		//アルファブレンド設定
		AlphaBlendEnable = True;
		SrcBlend = SRCALPHA;
		DestBlend = INVSRCALPHA;
	}
	pass P1
	{
		VertexShader = compile vs_3_0 TextureVS_PB1NT();
		PixelShader = compile ps_3_0 NonTexturePS();
	}
	pass P2
	{
		VertexShader = compile vs_3_0 TextureVS_PB1NCT();
		PixelShader = compile ps_3_0 TexturePS();

		//アルファテスト
		AlphaTestEnable = True;
		AlphaFunc = GREATEREQUAL;
		AlphaRef = 0xC8;

		//アルファブレンド設定
		AlphaBlendEnable = True;
		SrcBlend = SRCALPHA;
		DestBlend = INVSRCALPHA;
	}
	pass P3
	{
		VertexShader = compile vs_3_0 TextureVS_PB1NCT();
		PixelShader = compile ps_3_0 NonTexturePS();
	}
	pass P4
	{
		VertexShader = compile vs_3_0 TextureVS_PB2NCT();
		PixelShader = compile ps_3_0 TexturePS();

		//アルファテスト
		AlphaTestEnable = True;
		AlphaFunc = GREATEREQUAL;
		AlphaRef = 0xC8;

		//アルファブレンド設定
		AlphaBlendEnable = True;
		SrcBlend = SRCALPHA;
		DestBlend = INVSRCALPHA;
	}
	pass P5
	{
		VertexShader = compile vs_3_0 TextureVS_PB2NCT();
		PixelShader = compile ps_3_0 NonTexturePS();
	}
	pass P6
	{
		VertexShader = compile vs_3_0 TextureVS_PB3NCT();
		PixelShader = compile ps_3_0 TexturePS();

		//アルファテスト
		AlphaTestEnable = True;
		AlphaFunc = GREATEREQUAL;
		AlphaRef = 0xC8;

		//アルファブレンド設定
		AlphaBlendEnable = True;
		SrcBlend = SRCALPHA;
		DestBlend = INVSRCALPHA;
	}
	pass P7
	{
		VertexShader = compile vs_3_0 TextureVS_PB3NCT();
		PixelShader = compile ps_3_0 NonTexturePS();
	}
	pass P8
	{
		VertexShader = compile vs_3_0 TextureVS_PB3NT();
		PixelShader = compile ps_3_0 TexturePS();

		//アルファテスト
		AlphaTestEnable = True;
		AlphaFunc = GREATEREQUAL;
		AlphaRef = 0xC8;

		//アルファブレンド設定
		AlphaBlendEnable = True;
		SrcBlend = SRCALPHA;
		DestBlend = INVSRCALPHA;
	}
	pass P9
	{
		VertexShader = compile vs_3_0 TextureVS_PB3NT();
		PixelShader = compile ps_3_0 NonTexturePS();
	}	
}