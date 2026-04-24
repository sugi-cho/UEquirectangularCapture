#include "EquirectangularCaptureRenderer.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTargetCube.h"
#include "GlobalShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"

class FEquirectangularCaptureCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FEquirectangularCaptureCS);
	SHADER_USE_PARAMETER_STRUCT(FEquirectangularCaptureCS, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector2f, OutputSize)
		SHADER_PARAMETER(uint32, FaceEnabledMask)
		SHADER_PARAMETER_SAMPLER(SamplerState, FaceSampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FaceTexture0)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FaceTexture1)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FaceTexture2)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FaceTexture3)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FaceTexture4)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FaceTexture5)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&)
	{
		return true;
	}
};

IMPLEMENT_GLOBAL_SHADER(FEquirectangularCaptureCS, "/UEquirectangularCapture/EquirectangularCapture.usf", "MainCS", SF_Compute);

class FEquirectangularCubeCaptureCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FEquirectangularCubeCaptureCS);
	SHADER_USE_PARAMETER_STRUCT(FEquirectangularCubeCaptureCS, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector2f, OutputSize)
		SHADER_PARAMETER_SAMPLER(SamplerState, CubeSampler)
		SHADER_PARAMETER_RDG_TEXTURE(TextureCube, CubeTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters&)
	{
		return true;
	}
};

IMPLEMENT_GLOBAL_SHADER(FEquirectangularCubeCaptureCS, "/UEquirectangularCapture/EquirectangularCapture.usf", "MainCubeCS", SF_Compute);

void UEquirectangularCaptureRenderer::Render(UTextureRenderTarget2D* OutputRenderTarget, const TArray<TObjectPtr<UTextureRenderTarget2D>>& FaceRenderTargets, uint32 EnabledFaceMask)
{
	if (!OutputRenderTarget || FaceRenderTargets.Num() < 6)
	{
		return;
	}

	FTextureRenderTargetResource* OutputResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
	if (!OutputResource)
	{
		return;
	}

	TArray<FTextureRenderTargetResource*, TInlineAllocator<6>> FaceResources;
	FaceResources.Reserve(6);
	for (int32 Index = 0; Index < 6; ++Index)
	{
		if (!FaceRenderTargets.IsValidIndex(Index) || !FaceRenderTargets[Index])
		{
			return;
		}

		FTextureRenderTargetResource* Resource = FaceRenderTargets[Index]->GameThread_GetRenderTargetResource();
		if (!Resource)
		{
			return;
		}

		FaceResources.Add(Resource);
	}

	const FIntPoint OutputSize(OutputRenderTarget->SizeX, OutputRenderTarget->SizeY);

	ENQUEUE_RENDER_COMMAND(RenderEquirectangularCapture)(
		[OutputResource, FaceResources, EnabledFaceMask, OutputSize](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRDGTextureRef OutputTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(OutputResource->GetRenderTargetTexture(), TEXT("UEquirectangularCapture_Output")));
			FRDGTextureRef FaceTexture0 = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(FaceResources[0]->GetRenderTargetTexture(), TEXT("UEquirectangularCapture_Face0")));
			FRDGTextureRef FaceTexture1 = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(FaceResources[1]->GetRenderTargetTexture(), TEXT("UEquirectangularCapture_Face1")));
			FRDGTextureRef FaceTexture2 = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(FaceResources[2]->GetRenderTargetTexture(), TEXT("UEquirectangularCapture_Face2")));
			FRDGTextureRef FaceTexture3 = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(FaceResources[3]->GetRenderTargetTexture(), TEXT("UEquirectangularCapture_Face3")));
			FRDGTextureRef FaceTexture4 = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(FaceResources[4]->GetRenderTargetTexture(), TEXT("UEquirectangularCapture_Face4")));
			FRDGTextureRef FaceTexture5 = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(FaceResources[5]->GetRenderTargetTexture(), TEXT("UEquirectangularCapture_Face5")));

			FEquirectangularCaptureCS::FParameters* Parameters = GraphBuilder.AllocParameters<FEquirectangularCaptureCS::FParameters>();
			Parameters->OutputSize = FVector2f((float)OutputSize.X, (float)OutputSize.Y);
			Parameters->FaceEnabledMask = EnabledFaceMask;
			Parameters->FaceSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
			Parameters->FaceTexture0 = FaceTexture0;
			Parameters->FaceTexture1 = FaceTexture1;
			Parameters->FaceTexture2 = FaceTexture2;
			Parameters->FaceTexture3 = FaceTexture3;
			Parameters->FaceTexture4 = FaceTexture4;
			Parameters->FaceTexture5 = FaceTexture5;
			Parameters->OutputTexture = GraphBuilder.CreateUAV(OutputTexture);

			TShaderMapRef<FEquirectangularCaptureCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			const FIntVector GroupCount(
				FMath::DivideAndRoundUp(OutputSize.X, 8),
				FMath::DivideAndRoundUp(OutputSize.Y, 8),
				1);

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("UEquirectangularCapture"),
				ComputeShader,
				Parameters,
				GroupCount);

			GraphBuilder.Execute();
		});
}

void UEquirectangularCaptureRenderer::RenderFromCube(UTextureRenderTarget2D* OutputRenderTarget, UTextureRenderTargetCube* CubeRenderTarget)
{
	if (!OutputRenderTarget || !CubeRenderTarget)
	{
		return;
	}

	FTextureRenderTargetResource* OutputResource = OutputRenderTarget->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* CubeResource = CubeRenderTarget->GameThread_GetRenderTargetResource();
	if (!OutputResource || !CubeResource)
	{
		return;
	}

	const FIntPoint OutputSize(OutputRenderTarget->SizeX, OutputRenderTarget->SizeY);

	ENQUEUE_RENDER_COMMAND(RenderEquirectangularCubeCapture)(
		[OutputResource, CubeResource, OutputSize](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRDGTextureRef OutputTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(OutputResource->GetRenderTargetTexture(), TEXT("UEquirectangularCubeCapture_Output")));
			FRDGTextureRef CubeTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(CubeResource->GetRenderTargetTexture(), TEXT("UEquirectangularCubeCapture_Cube")));

			FEquirectangularCubeCaptureCS::FParameters* Parameters = GraphBuilder.AllocParameters<FEquirectangularCubeCaptureCS::FParameters>();
			Parameters->OutputSize = FVector2f((float)OutputSize.X, (float)OutputSize.Y);
			Parameters->CubeSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
			Parameters->CubeTexture = CubeTexture;
			Parameters->OutputTexture = GraphBuilder.CreateUAV(OutputTexture);

			TShaderMapRef<FEquirectangularCubeCaptureCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			const FIntVector GroupCount(
				FMath::DivideAndRoundUp(OutputSize.X, 8),
				FMath::DivideAndRoundUp(OutputSize.Y, 8),
				1);

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("UEquirectangularCubeCapture"),
				ComputeShader,
				Parameters,
				GroupCount);

			GraphBuilder.Execute();
		});
}

void UEquirectangularCaptureRenderer::CopyToRenderTarget(UTextureRenderTarget2D* SourceRenderTarget, UTextureRenderTarget2D* DestinationRenderTarget)
{
	if (!SourceRenderTarget || !DestinationRenderTarget || SourceRenderTarget == DestinationRenderTarget)
	{
		return;
	}

	FTextureRenderTargetResource* SourceResource = SourceRenderTarget->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* DestinationResource = DestinationRenderTarget->GameThread_GetRenderTargetResource();
	if (!SourceResource || !DestinationResource)
	{
		return;
	}

	const FIntPoint CopySize(
		FMath::Min(SourceRenderTarget->SizeX, DestinationRenderTarget->SizeX),
		FMath::Min(SourceRenderTarget->SizeY, DestinationRenderTarget->SizeY));

	ENQUEUE_RENDER_COMMAND(CopyEquirectangularCaptureOutput)(
		[SourceResource, DestinationResource, CopySize](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(RHICmdList);

			FRDGTextureRef SourceTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(SourceResource->GetRenderTargetTexture(), TEXT("UEquirectangularCapture_CopySource")));
			FRDGTextureRef DestinationTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DestinationResource->GetRenderTargetTexture(), TEXT("UEquirectangularCapture_CopyDestination")));

			FRHICopyTextureInfo CopyInfo;
			CopyInfo.Size = FIntVector(CopySize.X, CopySize.Y, 1);
			AddCopyTexturePass(GraphBuilder, SourceTexture, DestinationTexture, CopyInfo);

			GraphBuilder.Execute();
		});
}
