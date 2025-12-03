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

// スポットライトの最大数（アプリ側でこれに合わせる）
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

// スポットライトのパラメータ（Outer, Inner, Falloff)
float gSpotLightOuterAngle[MAX_SPOT_LIGHTS];
float gSpotLightInnerAngle[MAX_SPOT_LIGHTS];
float gSpotLightFalloff[MAX_SPOT_LIGHTS];

// スポットライトの減衰パラメータ
float gSpotLightAttn0[MAX_SPOT_LIGHTS];
float gSpotLightAttn1[MAX_SPOT_LIGHTS];
float gSpotLightAttn2[MAX_SPOT_LIGHTS];

// --- ライト視点のビュー射影行列（配列は OK） ---
float4x4 gLightViewProj[MAX_SPOT_LIGHTS];

// --- シャドウマップ（個別に宣言） ---
texture gShadowMap0;
sampler shadowSampler0 = sampler_state
{
	Texture = <gShadowMap0>;
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = NONE;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

texture gShadowMap1;
sampler shadowSampler1 = sampler_state
{
	Texture = <gShadowMap1>;
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = NONE;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

texture gShadowMap2;
sampler shadowSampler2 = sampler_state
{
	Texture = <gShadowMap2>;
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = NONE;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

texture gShadowMap3;
sampler shadowSampler3 = sampler_state
{
	Texture = <gShadowMap3>;
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = NONE;
	AddressU = CLAMP;
	AddressV = CLAMP;
};

// シャドウバイアス（影のアーティファクト防止）
float gShadowBias = 0.001f;

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

// 頂点シェーダー出力用（lightSpacePos を個別 TEXCOORD に分解）
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

// ピクセルシェーダー入力用
struct PS_INPUT
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
	float depth : TEXCOORD0;
};

// 深度パス用のピクセルシェーダー出力
struct PS_DEPTH_OUTPUT
{
	float4 color : COLOR0;
};

//=============================================================================
// シャドウマップから影を判定（ライトインデックスでサンプラを選択）
//=============================================================================
float CalculateShadow(int lightIndex, float4 lightSpacePos)
{
	// ライト空間座標を正規化（-1～1 -> 0～1）
	float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
	projCoords.x = projCoords.x * 0.5f + 0.5f;
	projCoords.y = -projCoords.y * 0.5f + 0.5f;

	// テクスチャ範囲外なら影なし
	if (projCoords.x < 0.0f || projCoords.x > 1.0f ||
		projCoords.y < 0.0f || projCoords.y > 1.0f ||
		projCoords.z < 0.0f || projCoords.z > 1.0f)
	{
		return 1.0f;
	}

	// シャドウマップから深度値を取得（ライト毎にサンプラを切り替え）
	float shadowDepth = 1.0f;

	[branch]
	if (lightIndex == 0)
	{
		shadowDepth = tex2D(shadowSampler0, projCoords.xy).r;
	}
	[branch]
	if (lightIndex == 1)
	{
		shadowDepth = tex2D(shadowSampler1, projCoords.xy).r;
	}
	[branch]
	if (lightIndex == 2)
	{
		shadowDepth = tex2D(shadowSampler2, projCoords.xy).r;
	}
	[branch]
	if (lightIndex == 3)
	{
		shadowDepth = tex2D(shadowSampler3, projCoords.xy).r;
	}

	float currentDepth = projCoords.z;

	// 影判定（バイアスを加える）
	float shadow = (currentDepth - gShadowBias) > shadowDepth ? 0.0f : 1.0f;
	return shadow;
}

//=============================================================================
// ライティング計算（ディレクショナルライト＋スポットライト + 影）
//=============================================================================
float4 CalculateLighting(float3 worldPos, float3 normal,
	float4 ls0, float4 ls1, float4 ls2, float4 ls3)
{
	// 最終的なライトの結果
	float4 lightResult = float4(0.0f, 0.0f, 0.0f, 1.0f);

	// スポットライトの計算
	for (int i = 0; i < gSpotLightCount; i++)
	{
		float3 lightVec = gSpotLightPos[i] - worldPos;
		float dist = length(lightVec);
		float3 toLight = normalize(lightVec);
		float3 spotDir = normalize(gSpotLightDir[i]);

		// スポットライトの最大範囲外なら計算しない
		if (dist > gSpotLightRange[i]) continue;

		// 距離減衰の計算
		float clampedDist = max(dist, 0.5f);
		float attenuation = 1.0f / (gSpotLightAttn0[i] + gSpotLightAttn1[i] * clampedDist + gSpotLightAttn2[i] * clampedDist * clampedDist);

		// ライトの方向と逆方向ベクトルの内積 (cos(角度))
		float spotCos = dot(toLight, -spotDir);

		float spotFactor = 0.0f;

		// スポットライトの角度減衰の計算
		if (spotCos >= gSpotLightOuterAngle[i]) // Outer Coneの内側
		{
			if (spotCos >= gSpotLightInnerAngle[i]) // Inner Coneの内側
			{
				spotFactor = 1.0f;
			}
			else // Inner ConeとOuter Coneの間
			{
				spotFactor = saturate(pow((spotCos - gSpotLightOuterAngle[i]) / (gSpotLightInnerAngle[i] - gSpotLightOuterAngle[i]), gSpotLightFalloff[i]));
			}
		}

		// ディフューズ（拡散光）成分を計算
		float diffuseFactor = saturate(dot(normal, toLight));

		// ★★ 影判定：ライトに応じて対応する lightSpacePos を渡す
		float shadow = 1.0f;
		if (i == 0) shadow = CalculateShadow(0, ls0);
		else if (i == 1) shadow = CalculateShadow(1, ls1);
		else if (i == 2) shadow = CalculateShadow(2, ls2);
		else if (i == 3) shadow = CalculateShadow(3, ls3);

		// スポットライトの色と減衰、拡散光成分、影を掛け合わせる
		lightResult += gSpotLightColor[i] * attenuation * spotFactor * diffuseFactor * shadow;
	}

	// 環境光を加算
	lightResult += gAmbientColor;

	// ディレクショナルライトを加算
	float diffuseDirFactor = saturate(dot(normal, -gLightDir));
	lightResult += gLightColor * diffuseDirFactor;

	// 最終的なライトの色を0-1の範囲にクランプして返す
	return saturate(lightResult);
}

//=============================================================================
// 頂点シェーダ（テクスチャあり）
//=============================================================================
VS_OUTPUT TextureVS(VS_INPUT In)
{
	VS_OUTPUT Out;

	// 頂点に変換行列を乗算する
	Out.position = mul(float4(In.position, 1.0f), gMatWVP);

	// ワールド空間の頂点座標をピクセルシェーダーに渡す
	Out.worldPos = mul(float4(In.position, 1.0f), gMatW).xyz;

	// ワールド空間の法線ベクトルをピクセルシェーダーに渡す
	Out.worldNormal = normalize(mul(float4(In.normal, 0.0f), gMatW).xyz);

	// UV座標の設定
	Out.texCoord = In.texCoord;

	// ★★★ 修正: スポットライトがある場合のみライト空間座標を計算 ★★★
	float4 worldPos4 = float4(Out.worldPos, 1.0f);

	if (gSpotLightCount > 0)
	{
		Out.lightSpacePos0 = mul(worldPos4, gLightViewProj[0]);
		if (gSpotLightCount > 1)
			Out.lightSpacePos1 = mul(worldPos4, gLightViewProj[1]);
		else
			Out.lightSpacePos1 = float4(0, 0, 0, 1);

		if (gSpotLightCount > 2)
			Out.lightSpacePos2 = mul(worldPos4, gLightViewProj[2]);
		else
			Out.lightSpacePos2 = float4(0, 0, 0, 1);

		if (gSpotLightCount > 3)
			Out.lightSpacePos3 = mul(worldPos4, gLightViewProj[3]);
		else
			Out.lightSpacePos3 = float4(0, 0, 0, 1);
	}
	else
	{
		// スポットライトが無い場合はダミー値
		Out.lightSpacePos0 = float4(0, 0, 0, 1);
		Out.lightSpacePos1 = float4(0, 0, 0, 1);
		Out.lightSpacePos2 = float4(0, 0, 0, 1);
		Out.lightSpacePos3 = float4(0, 0, 0, 1);
	}

	return Out;
}

//=============================================================================
// ピクセルシェーダ（テクスチャあり）
//=============================================================================
PS_OUTPUT TexturePS(PS_INPUT In)
{
	PS_OUTPUT Out;

	// テクスチャーの色を取得
	float4 texColor = tex2D(texSampler, In.texCoord);

	// ライティング結果を計算（影を含む）
	float4 lightingColor = CalculateLighting(In.worldPos, In.worldNormal,
		In.lightSpacePos0, In.lightSpacePos1, In.lightSpacePos2, In.lightSpacePos3);

	// 最終的な色を計算
	Out.color = texColor * gMaterialDiffuse * lightingColor;

	return Out;
}

//=============================================================================
// 頂点シェーダ（テクスチャなし）
//=============================================================================
VS_OUTPUT NonTextureVS(VS_INPUT In)
{
	VS_OUTPUT Out;

	// 頂点に変換行列を乗算する
	Out.position = mul(float4(In.position, 1.0f), gMatWVP);

	// ワールド空間の頂点座標をピクセルシェーダーに渡す
	Out.worldPos = mul(float4(In.position, 1.0f), gMatW).xyz;

	// ワールド空間の法線ベクトルをピクセルシェーダーに渡す
	Out.worldNormal = normalize(mul(float4(In.normal, 0.0f), gMatW).xyz);

	// UV座標の設定（未使用でも埋めておく）
	Out.texCoord = In.texCoord;

	// ★★★ 修正: スポットライトがある場合のみライト空間座標を計算 ★★★
	float4 worldPos4 = float4(Out.worldPos, 1.0f);

	if (gSpotLightCount > 0)
	{
		Out.lightSpacePos0 = mul(worldPos4, gLightViewProj[0]);
		if (gSpotLightCount > 1)
			Out.lightSpacePos1 = mul(worldPos4, gLightViewProj[1]);
		else
			Out.lightSpacePos1 = float4(0, 0, 0, 1);

		if (gSpotLightCount > 2)
			Out.lightSpacePos2 = mul(worldPos4, gLightViewProj[2]);
		else
			Out.lightSpacePos2 = float4(0, 0, 0, 1);

		if (gSpotLightCount > 3)
			Out.lightSpacePos3 = mul(worldPos4, gLightViewProj[3]);
		else
			Out.lightSpacePos3 = float4(0, 0, 0, 1);
	}
	else
	{
		// スポットライトが無い場合はダミー値
		Out.lightSpacePos0 = float4(0, 0, 0, 1);
		Out.lightSpacePos1 = float4(0, 0, 0, 1);
		Out.lightSpacePos2 = float4(0, 0, 0, 1);
		Out.lightSpacePos3 = float4(0, 0, 0, 1);
	}

	return Out;
}

//=============================================================================
// ピクセルシェーダ（テクスチャなし）
//=============================================================================
PS_OUTPUT NonTexturePS(PS_INPUT In)
{
	PS_OUTPUT Out;

	// ライティング結果を計算（影を含む）
	float4 lightingColor = CalculateLighting(In.worldPos, In.worldNormal,
		In.lightSpacePos0, In.lightSpacePos1, In.lightSpacePos2, In.lightSpacePos3);

	// 最終的な色を計算
	Out.color = gMaterialDiffuse * lightingColor;

	return Out;
}

//=============================================================================
// 深度パス用の頂点シェーダ
//=============================================================================
VS_DEPTH_OUTPUT DepthVS(VS_INPUT In)
{
	VS_DEPTH_OUTPUT Out;

	// 頂点に変換行列を乗算する（深度レンダー用）
	Out.position = mul(float4(In.position, 1.0f), gMatWVP);

	// 深度値を計算（0.0～1.0の範囲）
	Out.depth = Out.position.z / Out.position.w;

	return Out;
}

//=============================================================================
// 深度パス用のピクセルシェーダ
//=============================================================================
PS_DEPTH_OUTPUT DepthPS(VS_DEPTH_OUTPUT In)
{
	PS_DEPTH_OUTPUT Out;

	// 深度値をカラーとして出力
	Out.color = float4(In.depth, In.depth, In.depth, 1.0f);

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