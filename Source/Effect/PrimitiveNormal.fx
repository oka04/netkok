//ワールド座標変換行列
float4x4 gMatW;

//ワールドビュー射影変換行列宣言
float4x4 gMatWVP;

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

//頂点シェーダー入力用
struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
};

//頂点シェーダー出力用
struct VS_OUTPUT {
	float4 position : POSITION;
	float4 color    : COLOR0;
};

//ピクセルシェーダー入力用
struct PS_INPUT {
	float4 position : POSITION;
	float4 color    : COLOR0;
};

//ピクセルシェーダー出力用
struct PS_OUTPUT {
	float4 color : COLOR0;
};

// 頂点シェーダ
VS_OUTPUT NormalVS(VS_INPUT In)
{
	VS_OUTPUT Out;

	//頂点に変換行列を乗算する
	Out.position = mul(float4(In.position, 1.0f), gMatWVP);

	//float4へ変換
	float4 normal = mul(float4(In.normal, 0.0f), gMatW);

	//ディレクショナルライトの方向を逆にする（次の計算のため）
	float4 revLightDir = -float4(gLightDir, 0.0f);

	//法線とディレクショナルライトの方向の内積にディレクショナルライトの色を乗算しアンビエントライトの色と加算
	//saturate　→　結果を0.0から1.0までの範囲に収める
	Out.color = saturate(gAmbientColor * gMaterialAmbient + gLightColor * gMaterialDiffuse * max(0, dot(revLightDir, normal)));

	return Out;
}

// ピクセルシェーダ
PS_OUTPUT NormalPS(PS_INPUT In)
{
	PS_OUTPUT Out;

	Out.color = In.color;

	return Out;
}

//テクニック
technique PrimitiveNormalTec
{
   pass P0
   {
      VertexShader = compile vs_3_0 NormalVS();
      PixelShader = compile ps_3_0 NormalPS();
   }
}