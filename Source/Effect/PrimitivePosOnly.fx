//ワールドビュー射影変換行列宣言
float4x4 gMatWVP;

//アンビエントライトの色
float4 gAmbientColor;

//マテリアルの色
float4 gMaterialDiffuse;

//マテリアルのアンビエント
float4 gMaterialAmbient;

//頂点シェーダー入力用
struct VS_INPUT
{
	float3 position : POSITION;
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
VS_OUTPUT PosOnlyVS(VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(float4(In.position, 1.0f), gMatWVP);

	Out.color = gMaterialDiffuse;

	return Out;
}

// ピクセルシェーダ
PS_OUTPUT PosOnlyPS(PS_INPUT In)
{
	PS_OUTPUT Out;

	Out.color = In.color;

	return Out;
}

//テクニック
technique PrimitivePosOnlyTec
{
   pass P0
   {
      VertexShader = compile vs_3_0 PosOnlyVS();
      PixelShader = compile ps_3_0 PosOnlyPS();
   }
}