// ワールド座標変換行列
float4x4 gMatW;

// ワールドビュー射影変換行列宣言
float4x4 gMatWVP;

// アンビエントライトの色
float4 gAmbientColor;

// ディレクショナルライトの色
float4 gLightColor;

// ディレクショナルライトの方向
float3 gLightDir;

// マテリアルの色
float4 gMaterialDiffuse;

// マテリアルのアンビエント
float4 gMaterialAmbient;

// スポットライトの最大数
#define MAX_SPOT_LIGHTS 4

// スポットライトの数
int gSpotLightCount;

// スポットライトの位置
float3 gSpotLightPos[MAX_SPOT_LIGHTS];

// スポットライトの方向
float3 gSpotLightDir[MAX_SPOT_LIGHTS];

// スポットライトの色
float4 gSpotLightColor[MAX_SPOT_LIGHTS];

// スポットライトの照射距離
float gSpotLightRange[MAX_SPOT_LIGHTS];

// スポットライトのパラメータ
float gSpotLightOuterAngle[MAX_SPOT_LIGHTS];
float gSpotLightInnerAngle[MAX_SPOT_LIGHTS];
float gSpotLightFalloff[MAX_SPOT_LIGHTS];

// スポットライトの減衰パラメータ
float gSpotLightAttn0[MAX_SPOT_LIGHTS];
float gSpotLightAttn1[MAX_SPOT_LIGHTS];
float gSpotLightAttn2[MAX_SPOT_LIGHTS];

// ライト視点のビュー射影行列
float4x4 gLightViewProj[MAX_SPOT_LIGHTS];

// シャドウマップ
texture gShadowMap0;
sampler shadowSampler0 = sampler_state
{
	Texture = <gShadowMap0>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

texture gShadowMap1;
sampler shadowSampler1 = sampler_state
{
	Texture = <gShadowMap1>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

texture gShadowMap2;
sampler shadowSampler2 = sampler_state
{
	Texture = <gShadowMap2>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

texture gShadowMap3;
sampler shadowSampler3 = sampler_state
{
	Texture = <gShadowMap3>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

// スケールバイアス行列（射影空間→テクスチャ空間）
float4x4 gMatScaleBias[MAX_SPOT_LIGHTS];

// テクスチャー
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

// 頂点シェーダー入力用
struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
	float2 texCoord : TEXCOORD0;
};

// 頂点シェーダー出力用
struct VS_OUTPUT
{
	float4 position : POSITION;
	float3 worldPos : TEXCOORD1;
	float3 worldNormal : TEXCOORD2;
	float2 texCoord : TEXCOORD0;

	float4 lightSpacePos0 : TEXCOORD3;
	float4 lightSpacePos1 : TEXCOORD4;
	float4 lightSpacePos2 : TEXCOORD5;
	float4 lightSpacePos3 : TEXCOORD6;
};

// ピクセルシェーダー出力用
struct PS_OUTPUT
{
	float4 color : COLOR0;
};

// 深度パス用の頂点シェーダー出力
struct VS_DEPTH_OUTPUT
{
	float4 position : POSITION;
	float4 lightSpacePos : TEXCOORD0;
};

// 深度パス用のピクセルシェーダー出力
struct PS_DEPTH_OUTPUT
{
	float4 color : COLOR0;
};
float gShadowBias = 0.002f;
//=============================================================================
// シャドウマップから影を判定
//=============================================================================
float CalculateShadow(int lightIndex, float4 lightSpacePos, float4x4 scaleBias, float3 normal, float3 lightDir)
{
	// ライト空間座標をテクスチャ座標に変換
	float4 texCoord = mul(lightSpacePos, scaleBias);
	texCoord.xyz /= texCoord.w;
	// テクスチャ範囲外なら影なし
	if (texCoord.x < 0.0f || texCoord.x > 1.0f ||
		texCoord.y < 0.0f || texCoord.y > 1.0f ||
		texCoord.z < 0.0f || texCoord.z > 1.0f)
	{
		return 1.0f;
	}
	// 法線とライト方向の内積でバイアスを調整
	float cosTheta = saturate(dot(normal, -lightDir));

	// 傾斜バイアス: 表面が光源に対して傾いているほど大きくする
	float slopeBias = gShadowBias * tan(acos(cosTheta));
	slopeBias = clamp(slopeBias, 0.0f, 0.02f);

	// 最終バイアス = 基本バイアス + 傾斜バイアス
	float bias = gShadowBias + slopeBias;
	// シャドウマップから深度値を取得（4サンプルPCFでソフトシャドウ）
	float shadowDepth = 0.0f;
	float2 texelSize = float2(1.0f / 1024.0f, 1.0f / 1024.0f);

	// 2x2 PCFサンプリング
	float samples = 0.0f;
	for (int x = -1; x <= 0; x++)
	{
		for (int y = -1; y <= 0; y++)
		{
			float2 offset = float2(x, y) * texelSize;
			float depth = 0.0f;

			if (lightIndex == 0)
			{
				depth = tex2D(shadowSampler0, texCoord.xy + offset).r;
			}
			else if (lightIndex == 1)
			{
				depth = tex2D(shadowSampler1, texCoord.xy + offset).r;
			}
			else if (lightIndex == 2)
			{
				depth = tex2D(shadowSampler2, texCoord.xy + offset).r;
			}
			else if (lightIndex == 3)
			{
				depth = tex2D(shadowSampler3, texCoord.xy + offset).r;
			}

			samples += ((texCoord.z - bias) > depth) ? 0.0f : 1.0f;
		}
	}

	// 平均を取る
	float shadow = samples / 4.0f;
	return shadow;
}
//=============================================================================
// ライティング計算
//=============================================================================
float4 CalculateLighting(float3 worldPos, float3 normal,
	float4 ls0, float4 ls1, float4 ls2, float4 ls3)
{
	float4 lightResult = float4(0.0f, 0.0f, 0.0f, 1.0f);

	// スポットライトの計算
	for (int i = 0; i < gSpotLightCount; i++)
	{
		float3 lightVec = gSpotLightPos[i] - worldPos;
		float dist = length(lightVec);
		float3 toLight = normalize(lightVec);
		float3 spotDir = normalize(gSpotLightDir[i]);

		if (dist > gSpotLightRange[i]) continue;

		// 距離減衰
		float clampedDist = max(dist, 0.5f);
		float attenuation = 1.0f / (gSpotLightAttn0[i] + gSpotLightAttn1[i] * clampedDist + gSpotLightAttn2[i] * clampedDist * clampedDist);

		// スポットライトの角度減衰
		float spotCos = dot(toLight, -spotDir);
		float spotFactor = 0.0f;

		if (spotCos >= gSpotLightOuterAngle[i])
		{
			if (spotCos >= gSpotLightInnerAngle[i])
			{
				spotFactor = 1.0f;
			}
			else
			{
				spotFactor = saturate(pow((spotCos - gSpotLightOuterAngle[i]) / (gSpotLightInnerAngle[i] - gSpotLightOuterAngle[i]), gSpotLightFalloff[i]));
			}
		}

		// スポットライトの範囲外なら影の計算をスキップ
		if (spotFactor <= 0.0f)
		{
			continue;
		}

		// ディフューズ成分
		float diffuseFactor = saturate(dot(normal, toLight));

		// 影判定
		float shadow = 1.0f;
		if (i == 0) shadow = CalculateShadow(0, ls0, gMatScaleBias[0], normal, spotDir);
		else if (i == 1) shadow = CalculateShadow(1, ls1, gMatScaleBias[1], normal, spotDir);
		else if (i == 2) shadow = CalculateShadow(2, ls2, gMatScaleBias[2], normal, spotDir);
		else if (i == 3) shadow = CalculateShadow(3, ls3, gMatScaleBias[3], normal, spotDir);

		// スポットライトの色と減衰、拡散光成分、影を掛け合わせる
		lightResult += gSpotLightColor[i] * attenuation * spotFactor * diffuseFactor * shadow;
	}

	// 環境光を加算
	lightResult += gAmbientColor;

	// ディレクショナルライトを加算
	float diffuseDirFactor = saturate(dot(normal, -gLightDir));
	lightResult += gLightColor * diffuseDirFactor;

	return saturate(lightResult);
}
//=============================================================================
// 頂点シェーダ（テクスチャあり）
//=============================================================================
VS_OUTPUT TextureVS(VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(float4(In.position, 1.0f), gMatWVP);
	Out.worldPos = mul(float4(In.position, 1.0f), gMatW).xyz;
	Out.worldNormal = normalize(mul(float4(In.normal, 0.0f), gMatW).xyz);
	Out.texCoord = In.texCoord;

	float4 worldPos4 = float4(Out.worldPos, 1.0f);

	Out.lightSpacePos0 = float4(0, 0, 0, 1);
	Out.lightSpacePos1 = float4(0, 0, 0, 1);
	Out.lightSpacePos2 = float4(0, 0, 0, 1);
	Out.lightSpacePos3 = float4(0, 0, 0, 1);

	if (gSpotLightCount > 0)
		Out.lightSpacePos0 = mul(worldPos4, gLightViewProj[0]);
	if (gSpotLightCount > 1)
		Out.lightSpacePos1 = mul(worldPos4, gLightViewProj[1]);
	if (gSpotLightCount > 2)
		Out.lightSpacePos2 = mul(worldPos4, gLightViewProj[2]);
	if (gSpotLightCount > 3)
		Out.lightSpacePos3 = mul(worldPos4, gLightViewProj[3]);

	return Out;
}

//=============================================================================
// ピクセルシェーダ（テクスチャあり）
//=============================================================================
PS_OUTPUT TexturePS(VS_OUTPUT In)
{
	PS_OUTPUT Out;

	float4 texColor = tex2D(texSampler, In.texCoord);
	float4 lightingColor = CalculateLighting(In.worldPos, In.worldNormal,
		In.lightSpacePos0, In.lightSpacePos1, In.lightSpacePos2, In.lightSpacePos3);

	Out.color = texColor * gMaterialDiffuse * lightingColor;

	return Out;
}

//=============================================================================
// 頂点シェーダ（テクスチャなし）
//=============================================================================
VS_OUTPUT NonTextureVS(VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(float4(In.position, 1.0f), gMatWVP);
	Out.worldPos = mul(float4(In.position, 1.0f), gMatW).xyz;
	Out.worldNormal = normalize(mul(float4(In.normal, 0.0f), gMatW).xyz);
	Out.texCoord = In.texCoord;

	float4 worldPos4 = float4(Out.worldPos, 1.0f);

	Out.lightSpacePos0 = float4(0, 0, 0, 1);
	Out.lightSpacePos1 = float4(0, 0, 0, 1);
	Out.lightSpacePos2 = float4(0, 0, 0, 1);
	Out.lightSpacePos3 = float4(0, 0, 0, 1);

	if (gSpotLightCount > 0)
		Out.lightSpacePos0 = mul(worldPos4, gLightViewProj[0]);
	if (gSpotLightCount > 1)
		Out.lightSpacePos1 = mul(worldPos4, gLightViewProj[1]);
	if (gSpotLightCount > 2)
		Out.lightSpacePos2 = mul(worldPos4, gLightViewProj[2]);
	if (gSpotLightCount > 3)
		Out.lightSpacePos3 = mul(worldPos4, gLightViewProj[3]);

	return Out;
}

//=============================================================================
// ピクセルシェーダ（テクスチャなし）
//=============================================================================
PS_OUTPUT NonTexturePS(VS_OUTPUT In)
{
	PS_OUTPUT Out;

	float4 lightingColor = CalculateLighting(In.worldPos, In.worldNormal,
		In.lightSpacePos0, In.lightSpacePos1, In.lightSpacePos2, In.lightSpacePos3);

	Out.color = gMaterialDiffuse * lightingColor;

	return Out;
}

//=============================================================================
// 深度パス用の頂点シェーダ
//=============================================================================
VS_DEPTH_OUTPUT DepthVS(VS_INPUT In)
{
	VS_DEPTH_OUTPUT Out;

	Out.position = mul(float4(In.position, 1.0f), gMatWVP);
	Out.lightSpacePos = Out.position;

	return Out;
}

//=============================================================================
// 深度パス用のピクセルシェーダ
//=============================================================================
PS_DEPTH_OUTPUT DepthPS(VS_DEPTH_OUTPUT In)
{
	PS_DEPTH_OUTPUT Out;

	// Z値を正規化して出力
	float depth = In.lightSpacePos.z / In.lightSpacePos.w;
	Out.color = float4(depth, depth, depth, 1.0f);

	return Out;
}

//=============================================================================
// テクニック：通常の描画
//=============================================================================
technique PrimitiveTextureTec
{
	pass P0
	{
		VertexShader = compile vs_3_0 TextureVS();
		PixelShader = compile ps_3_0 TexturePS();
	}

	pass P1
	{
		VertexShader = compile vs_3_0 NonTextureVS();
		PixelShader = compile ps_3_0 NonTexturePS();
	}
}

//=============================================================================
// テクニック：深度パス（影生成用）
//=============================================================================
technique DepthPass
{
	pass P0
	{
		VertexShader = compile vs_3_0 DepthVS();
		PixelShader = compile ps_3_0 DepthPS();
	}
}