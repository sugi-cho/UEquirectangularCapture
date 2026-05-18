#include "EquirectangularCaptureActor.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EquirectangularCaptureRenderer.h"

namespace
{
	constexpr int32 FaceCount = 6;

	struct FFaceSetup
	{
		const TCHAR* Name;
		FRotator Rotation;
	};

	const FFaceSetup FaceSetups[FaceCount] =
	{
		{ TEXT("FaceCapture_PositiveX"), FRotator(0.0f, 0.0f, 0.0f) },
		{ TEXT("FaceCapture_NegativeX"), FRotator(0.0f, 180.0f, 0.0f) },
		{ TEXT("FaceCapture_PositiveY"), FRotator(0.0f, 90.0f, 0.0f) },
		{ TEXT("FaceCapture_NegativeY"), FRotator(0.0f, -90.0f, 0.0f) },
		{ TEXT("FaceCapture_PositiveZ"), FRotator(90.0f, 0.0f, 0.0f) },
		{ TEXT("FaceCapture_NegativeZ"), FRotator(-90.0f, 0.0f, 0.0f) }
	};
}

AEquirectangularCaptureActor::AEquirectangularCaptureActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InitializeCaptureComponents();
}

void AEquirectangularCaptureActor::InitializeCaptureComponents()
{
	FaceCaptures.Reset();
	FaceCaptures.Reserve(FaceCount);

	for (int32 Index = 0; Index < FaceCount; ++Index)
	{
		USceneCaptureComponent2D* Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(FaceSetups[Index].Name);
		Capture->SetupAttachment(SceneRoot);
		Capture->SetRelativeLocation(FVector::ZeroVector);
		Capture->SetRelativeRotation(FaceSetups[Index].Rotation);
		Capture->bCaptureEveryFrame = false;
		Capture->bCaptureOnMovement = false;
		Capture->bAlwaysPersistRenderingState = true;
		Capture->FOVAngle = 90.0f;
		Capture->CaptureSource = ESceneCaptureSource::SCS_FinalToneCurveHDR;
		FaceCaptures.Add(Capture);
	}
}

void AEquirectangularCaptureActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshResources();
	if (bAutoCapture)
	{
#if WITH_EDITOR
		if (!GetWorld() || !GetWorld()->IsGameWorld())
		{
			RequestEditorCapture();
			return;
		}
#endif
		CaptureNow();
	}
}

void AEquirectangularCaptureActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshResources();
}

void AEquirectangularCaptureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if WITH_EDITOR
	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		EditorCaptureElapsed += DeltaSeconds;
		const bool bIntervalElapsed = bAutoCapture && EditorCaptureElapsed >= FMath::Max(0.1f, EditorCaptureInterval);
		if (bPendingEditorCapture || bIntervalElapsed)
		{
			bPendingEditorCapture = false;
			EditorCaptureElapsed = 0.0f;
			CaptureNow();
		}
		return;
	}
#endif

	if (bAutoCapture)
	{
		CaptureNow();
	}
}

#if WITH_EDITOR
void AEquirectangularCaptureActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshResources();
	if (bAutoCapture)
	{
		RequestEditorCapture();
	}
}

void AEquirectangularCaptureActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (bFinished && bAutoCapture)
	{
		RequestEditorCapture();
	}
}

bool AEquirectangularCaptureActor::ShouldTickIfViewportsOnly() const
{
	return bAutoCapture || bPendingEditorCapture;
}

void AEquirectangularCaptureActor::RequestEditorCapture()
{
	bPendingEditorCapture = true;
	EditorCaptureElapsed = 0.0f;
}
#endif

void AEquirectangularCaptureActor::InitializeRenderTargets()
{
	const int32 SafeFaceResolution = FMath::Max(64, FaceResolution);
	const int32 SafePreviewWidth = FMath::Max(64, OutputRenderTarget ? OutputRenderTarget->SizeX : PreviewWidth);
	const int32 SafePreviewHeight = FMath::Max(32, OutputRenderTarget ? OutputRenderTarget->SizeY : PreviewHeight);

	if (FaceRenderTargets.Num() != FaceCount)
	{
		FaceRenderTargets.SetNum(FaceCount);
	}

	for (int32 Index = 0; Index < FaceCount; ++Index)
	{
		if (!FaceRenderTargets[Index])
		{
			FaceRenderTargets[Index] = NewObject<UTextureRenderTarget2D>(this, *FString::Printf(TEXT("FaceRenderTarget_%d"), Index));
		}

		UTextureRenderTarget2D* FaceRT = FaceRenderTargets[Index];
		if (FaceRT->SizeX != SafeFaceResolution || FaceRT->SizeY != SafeFaceResolution)
		{
			FaceRT->ClearColor = FLinearColor::Black;
			FaceRT->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
			FaceRT->InitCustomFormat(SafeFaceResolution, SafeFaceResolution, PF_FloatRGBA, false);
			FaceRT->TargetGamma = 1.0f;
			FaceRT->UpdateResourceImmediate(true);
		}

		FaceCaptures[Index]->TextureTarget = FaceRT;
	}

	if (!PreviewRenderTarget)
	{
		PreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("PreviewRenderTarget"));
	}

	const bool bNeedsPreviewRecreate =
		!PreviewRenderTarget->GetResource() ||
		!PreviewRenderTarget->bSupportsUAV ||
		PreviewRenderTarget->SizeX != SafePreviewWidth ||
		PreviewRenderTarget->SizeY != SafePreviewHeight;

	PreviewRenderTarget->bSupportsUAV = true;

	if (bNeedsPreviewRecreate)
	{
		PreviewRenderTarget->ClearColor = FLinearColor::Black;
		PreviewRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
		PreviewRenderTarget->InitCustomFormat(SafePreviewWidth, SafePreviewHeight, PF_FloatRGBA, false);
		PreviewRenderTarget->TargetGamma = 1.0f;
		PreviewRenderTarget->UpdateResourceImmediate(true);
	}

	if (OutputRenderTarget && !OutputRenderTarget->GetResource())
	{
		OutputRenderTarget->UpdateResourceImmediate(true);
	}
}

void AEquirectangularCaptureActor::RefreshResources()
{
	InitializeRenderTargets();
}

UTextureRenderTarget2D* AEquirectangularCaptureActor::GetPreviewRenderTarget() const
{
	return PreviewRenderTarget;
}

UTextureRenderTarget2D* AEquirectangularCaptureActor::GetOutputRenderTarget() const
{
	return GetRenderTargetForOutput();
}

void AEquirectangularCaptureActor::CaptureNow()
{
	RefreshResources();

	for (int32 Index = 0; Index < FaceCount; ++Index)
	{
		if (!FaceCaptures.IsValidIndex(Index) || !FaceCaptures[Index] || !FaceRenderTargets.IsValidIndex(Index) || !FaceRenderTargets[Index])
		{
			continue;
		}

		FaceCaptures[Index]->TextureTarget = FaceRenderTargets[Index];
		if (IsFaceEnabled(static_cast<EEquirectangularCaptureFace>(Index)))
		{
			FaceCaptures[Index]->CaptureScene();
		}
	}

	RenderPreview();
}

void AEquirectangularCaptureActor::RenderPreview()
{
	UEquirectangularCaptureRenderer::Render(PreviewRenderTarget, FaceRenderTargets, GetEnabledFaceMask());
	UEquirectangularCaptureRenderer::CopyToRenderTarget(PreviewRenderTarget, OutputRenderTarget);
}

UTextureRenderTarget2D* AEquirectangularCaptureActor::GetRenderTargetForOutput() const
{
	return OutputRenderTarget ? OutputRenderTarget : PreviewRenderTarget;
}

void AEquirectangularCaptureActor::SetFaceEnabled(EEquirectangularCaptureFace Face, bool bEnabled)
{
	switch (Face)
	{
	case EEquirectangularCaptureFace::PositiveX: bPositiveX = bEnabled; break;
	case EEquirectangularCaptureFace::NegativeX: bNegativeX = bEnabled; break;
	case EEquirectangularCaptureFace::PositiveY: bPositiveY = bEnabled; break;
	case EEquirectangularCaptureFace::NegativeY: bNegativeY = bEnabled; break;
	case EEquirectangularCaptureFace::PositiveZ: bPositiveZ = bEnabled; break;
	case EEquirectangularCaptureFace::NegativeZ: bNegativeZ = bEnabled; break;
	default: break;
	}
}

bool AEquirectangularCaptureActor::IsFaceEnabled(EEquirectangularCaptureFace Face) const
{
	switch (Face)
	{
	case EEquirectangularCaptureFace::PositiveX: return bPositiveX;
	case EEquirectangularCaptureFace::NegativeX: return bNegativeX;
	case EEquirectangularCaptureFace::PositiveY: return bPositiveY;
	case EEquirectangularCaptureFace::NegativeY: return bNegativeY;
	case EEquirectangularCaptureFace::PositiveZ: return bPositiveZ;
	case EEquirectangularCaptureFace::NegativeZ: return bNegativeZ;
	default: return false;
	}
}

void AEquirectangularCaptureActor::SetAllFacesEnabled(bool bEnabled)
{
	bPositiveX = bEnabled;
	bNegativeX = bEnabled;
	bPositiveY = bEnabled;
	bNegativeY = bEnabled;
	bPositiveZ = bEnabled;
	bNegativeZ = bEnabled;
}

uint32 AEquirectangularCaptureActor::GetEnabledFaceMask() const
{
	uint32 Mask = 0;
	Mask |= bPositiveX ? (1u << 0) : 0u;
	Mask |= bNegativeX ? (1u << 1) : 0u;
	Mask |= bPositiveY ? (1u << 2) : 0u;
	Mask |= bNegativeY ? (1u << 3) : 0u;
	Mask |= bPositiveZ ? (1u << 4) : 0u;
	Mask |= bNegativeZ ? (1u << 5) : 0u;
	return Mask;
}
