#pragma once

#include "CoreMinimal.h"

class UTextureRenderTarget2D;
class UTextureRenderTargetCube;

class UEquirectangularCaptureRenderer
{
public:
	static void Render(UTextureRenderTarget2D* OutputRenderTarget, const TArray<TObjectPtr<UTextureRenderTarget2D>>& FaceRenderTargets, uint32 EnabledFaceMask);
	static void RenderFromCube(UTextureRenderTarget2D* OutputRenderTarget, UTextureRenderTargetCube* CubeRenderTarget);
	static void CopyToRenderTarget(UTextureRenderTarget2D* SourceRenderTarget, UTextureRenderTarget2D* DestinationRenderTarget);
};
