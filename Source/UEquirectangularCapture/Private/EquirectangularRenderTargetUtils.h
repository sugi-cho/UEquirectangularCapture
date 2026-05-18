#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"

namespace UEquirectangularCapture
{
	struct FRenderTargetSettings
	{
		int32 Width = 64;
		int32 Height = 32;
		TEnumAsByte<ETextureRenderTargetFormat> RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
		EPixelFormat PixelFormat = PF_B8G8R8A8;
		bool bIsSRGB = true;
		bool bForceLinearGamma = false;
		float TargetGamma = 0.0f;
	};

	inline FRenderTargetSettings MakePreviewSettingsFromOutput(UTextureRenderTarget2D* OutputRenderTarget, int32 FallbackWidth, int32 FallbackHeight)
	{
		FRenderTargetSettings Settings;
		Settings.Width = FMath::Max(64, OutputRenderTarget ? OutputRenderTarget->SizeX : FallbackWidth);
		Settings.Height = FMath::Max(32, OutputRenderTarget ? OutputRenderTarget->SizeY : FallbackHeight);
		Settings.RenderTargetFormat = OutputRenderTarget
			? OutputRenderTarget->RenderTargetFormat
			: TEnumAsByte<ETextureRenderTargetFormat>(ETextureRenderTargetFormat::RTF_RGBA8);
		Settings.PixelFormat = OutputRenderTarget ? OutputRenderTarget->GetFormat() : PF_B8G8R8A8;
		Settings.bIsSRGB = OutputRenderTarget ? OutputRenderTarget->IsSRGB() : true;
		Settings.bForceLinearGamma = !Settings.bIsSRGB;
		Settings.TargetGamma = OutputRenderTarget ? OutputRenderTarget->TargetGamma : 0.0f;
		return Settings;
	}

	inline bool NeedsRenderTargetRecreate(UTextureRenderTarget2D* RenderTarget, const FRenderTargetSettings& Settings)
	{
		return
			!RenderTarget->GetResource() ||
			!RenderTarget->bSupportsUAV ||
			RenderTarget->SizeX != Settings.Width ||
			RenderTarget->SizeY != Settings.Height ||
			RenderTarget->RenderTargetFormat != Settings.RenderTargetFormat ||
			RenderTarget->GetFormat() != Settings.PixelFormat ||
			RenderTarget->IsSRGB() != Settings.bIsSRGB ||
			RenderTarget->SRGB != Settings.bIsSRGB ||
			RenderTarget->bForceLinearGamma != Settings.bForceLinearGamma ||
			!FMath::IsNearlyEqual(RenderTarget->TargetGamma, Settings.TargetGamma);
	}

	inline void EnsureRenderTargetMatchesSettings(UTextureRenderTarget2D* RenderTarget, const FRenderTargetSettings& Settings)
	{
		if (!RenderTarget)
		{
			return;
		}

		if (!NeedsRenderTargetRecreate(RenderTarget, Settings))
		{
			return;
		}

		RenderTarget->ClearColor = FLinearColor::Black;
		RenderTarget->bSupportsUAV = true;
		RenderTarget->RenderTargetFormat = Settings.RenderTargetFormat;
		RenderTarget->SRGB = Settings.bIsSRGB;
		RenderTarget->TargetGamma = Settings.TargetGamma;
		RenderTarget->InitCustomFormat(Settings.Width, Settings.Height, Settings.PixelFormat, Settings.bForceLinearGamma);
		RenderTarget->UpdateResourceImmediate(true);
	}
}
