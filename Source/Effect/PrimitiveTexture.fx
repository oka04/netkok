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

// スポットライトの照射角（cos値）
float gSpotLightAngle[MAX_SPOT_LIGHTS];

// スポットライトの照射距離
float gSpotLightRange[MAX_SPOT_LIGHTS];

float gSpotLightOuterAngle[MAX_SPOT_LIGHTS];
float gSpotLightInnerAngle[MAX_SPOT_LIGHTS];
float gSpotLightFalloff[MAX_SPOT_LIGHTS];   
											
float gSpotLightAttn0[MAX_SPOT_LIGHTS];
float gSpotLightAttn1[MAX_SPOT_LIGHTS];
float gSpotLightAttn2[MAX_SPOT_LIGHTS];

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
struct VS_OUTPUT {
	float4 position : POSITION;
	float3 worldPos : TEXCOORD1;   
	float3 worldNormal : TEXCOORD2;
	float2 texCoord : TEXCOORD0;
};

// ピクセルシェーダー入力用
struct PS_INPUT {
	float4 position : POSITION;
	float3 worldPos : TEXCOORD1;   
	float3 worldNormal : TEXCOORD2;
	float2 texCoord : TEXCOORD0;
};

// ピクセルシェーダー出力用
struct PS_OUTPUT {
	float4 color : COLOR0;
};

// ライティング計算（ディレクショナルライト＋スポットライト）
float4 CalculateLighting(float3 worldPos, float3 normal)
{
	// 最終的なライトの結果
	float4 lightResult = float4(0.0f, 0.0f, 0.0f, 1.0f);

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

		// 純粋なスポットライトのディフューズ（拡散光）成分を計算
		// ここでモデルの法線（normal）とライト方向（toLight）の内積を取ることで、光が面に当たる角度によって明るさが変わるようにする
		// saturate() で0.0～1.0の範囲にクランプさせる
		float diffuseFactor = saturate(dot(normal, toLight));

		// スポットライトの色と減衰、拡散光成分を掛け合わせる
		lightResult += gSpotLightColor[i] * attenuation * spotFactor * diffuseFactor;
	}

	// 環境光を加算
	lightResult += gAmbientColor;

	// ディレクショナルライトを加算 
	float diffuseDirFactor = saturate(dot(normal, -gLightDir));
	lightResult += gLightColor * diffuseDirFactor;

	return saturate(lightResult); // 最終的なライトの色を0-1の範囲にクランプして返す
}

// 頂点シェーダ
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

	return Out;
}

// ピクセルシェーダ
PS_OUTPUT TexturePS(PS_INPUT In)
{
	PS_OUTPUT Out;

	// テクスチャーの色を取得
	float4 texColor = tex2D(texSampler, In.texCoord);

	// ライティング結果を計算（ピクセル単位で実行）
	float4 lightingColor = CalculateLighting(In.worldPos, In.worldNormal);

	Out.color = texColor * gMaterialDiffuse * lightingColor;

	return Out;
}

// 頂点シェーダ（テクスチャなし）
VS_OUTPUT NonTextureVS(VS_INPUT In)
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

	return Out;
}

// ピクセルシェーダ（テクスチャなし）
PS_OUTPUT NonTexturePS(PS_INPUT In)
{
	PS_OUTPUT Out;

	// ライティング結果を計算（ピクセル単位で実行）
	float4 lightingColor = CalculateLighting(In.worldPos, In.worldNormal);

	Out.color = gMaterialDiffuse * lightingColor;

	return Out;
}

// テクニック
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