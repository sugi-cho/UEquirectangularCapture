#include "EquirectangularCubeCaptureActor.h"

#include "Components/SceneCaptureComponentCube.h"
#include "Components/SceneComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/TextureRenderTargetCube.h"
#include "EquirectangularCaptureRenderer.h"

AEquirectangularCubeCaptureActor::AEquirectangularCubeCaptureActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CubeCapture = CreateDefaultSubobject<USceneCaptureComponentCube>(TEXT("CubeCapture"));
	CubeCapture->SetupAttachment(SceneRoot);
	CubeCapture->SetRelativeLocation(FVector::ZeroVector);
	CubeCapture->bCaptureEveryFrame = false;
	CubeCapture->bCaptureOnMovement = false;
	CubeCapture->bAlwaysPersistRenderingState = true;
	CubeCapture->bCaptureRotation = true;
	CubeCapture->CaptureSource = ESceneCaptureSource::SCS_FinalToneCurveHDR;
}

void AEquirectangularCubeCaptureActor::OnConstruction(const FTransform& Transform)
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

void AEquirectangularCubeCaptureActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshResources();
}

void AEquirectangularCubeCaptureActor::Tick(float DeltaSeconds)
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
void AEquirectangularCubeCaptureActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshResources();
	if (bAutoCapture)
	{
		RequestEditorCapture();
	}
}

void AEquirectangularCubeCaptureActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (bFinished && bAutoCapture)
	{
		RequestEditorCapture();
	}
}

bool AEquirectangularCubeCaptureActor::ShouldTickIfViewportsOnly() const
{
	return bAutoCapture || bPendingEditorCapture;
}

void AEquirectangularCubeCaptureActor::RequestEditorCapture()
{
	bPendingEditorCapture = true;
	EditorCaptureElapsed = 0.0f;
}
#endif

void AEquirectangularCubeCaptureActor::InitializeRenderTargets()
{
	const int32 SafeCubeResolution = FMath::Max(64, CubeResolution);
	const int32 SafePreviewWidth = FMath::Max(64, OutputRenderTarget ? OutputRenderTarget->SizeX : PreviewWidth);
	const int32 SafePreviewHeight = FMath::Max(32, OutputRenderTarget ? OutputRenderTarget->SizeY : PreviewHeight);
	const TEnumAsByte<ETextureRenderTargetFormat> PreviewFormat =
		OutputRenderTarget ? OutputRenderTarget->RenderTargetFormat : TEnumAsByte<ETextureRenderTargetFormat>(ETextureRenderTargetFormat::RTF_RGBA8);
    const bool bPreviewSRGB = OutputRenderTarget ? OutputRenderTarget->SRGB : true;
    const bool bPreviewForceLinearGamma = OutputRenderTarget ? OutputRenderTarget->bForceLinearGamma : false;
	const float PreviewGamma = OutputRenderTarget ? OutputRenderTarget->TargetGamma : 0.0f;

	if (!CubeRenderTarget)
	{
		CubeRenderTarget = NewObject<UTextureRenderTargetCube>(this, TEXT("CubeRenderTarget"));
	}

	if (CubeRenderTarget->SizeX != SafeCubeResolution || !CubeRenderTarget->GetResource())
	{
		CubeRenderTarget->ClearColor = FLinearColor::Black;
		CubeRenderTarget->bHDR = false;
		CubeRenderTarget->Init(SafeCubeResolution, PF_B8G8R8A8);
		CubeRenderTarget->UpdateResourceImmediate(true);
	}

	CubeCapture->TextureTarget = CubeRenderTarget;

	if (!PreviewRenderTarget)
	{
		PreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("CubePreviewRenderTarget"));
	}

	const bool bNeedsPreviewRecreate =
		!PreviewRenderTarget->GetResource() ||
		!PreviewRenderTarget->bSupportsUAV ||
		PreviewRenderTarget->SizeX != SafePreviewWidth ||
		PreviewRenderTarget->SizeY != SafePreviewHeight ||
		PreviewRenderTarget->RenderTargetFormat != PreviewFormat ||
		PreviewRenderTarget->bForceLinearGamma != bPreviewForceLinearGamma ||
		!FMath::IsNearlyEqual(PreviewRenderTarget->TargetGamma, PreviewGamma);

	PreviewRenderTarget->bSupportsUAV = true;
	PreviewRenderTarget->bForceLinearGamma = bPreviewForceLinearGamma;
        PreviewRenderTarget->TargetGamma = PreviewGamma;
        PreviewRenderTarget->UpdateResourceImmediate(true);

	if (bNeedsPreviewRecreate)
	{
		PreviewRenderTarget->ClearColor = FLinearColor::Black;
        PreviewRenderTarget->RenderTargetFormat = PreviewFormat;
        PreviewRenderTarget->SRGB = bPreviewSRGB;
        PreviewRenderTarget->InitCustomFormat(SafePreviewWidth, SafePreviewHeight, PF_B8G8R8A8, bPreviewSRGB);
		PreviewRenderTarget->UpdateResourceImmediate(true);
	}

	if (OutputRenderTarget && !OutputRenderTarget->GetResource())
	{
		OutputRenderTarget->UpdateResourceImmediate(true);
	}
}

void AEquirectangularCubeCaptureActor::RefreshResources()
{
	InitializeRenderTargets();
}

void AEquirectangularCubeCaptureActor::CaptureNow()
{
	RefreshResources();

	if (CubeCapture && CubeRenderTarget)
	{
		CubeCapture->TextureTarget = CubeRenderTarget;
		CubeCapture->CaptureScene();
	}

	RenderPreview();
}

UTextureRenderTarget2D* AEquirectangularCubeCaptureActor::GetPreviewRenderTarget() const
{
	return PreviewRenderTarget;
}

UTextureRenderTarget2D* AEquirectangularCubeCaptureActor::GetOutputRenderTarget() const
{
	return GetRenderTargetForOutput();
}

UTextureRenderTargetCube* AEquirectangularCubeCaptureActor::GetCubeRenderTarget() const
{
	return CubeRenderTarget;
}

void AEquirectangularCubeCaptureActor::RenderPreview()
{
	UEquirectangularCaptureRenderer::RenderFromCube(PreviewRenderTarget, CubeRenderTarget);
	UEquirectangularCaptureRenderer::CopyToRenderTarget(PreviewRenderTarget, OutputRenderTarget);
}

UTextureRenderTarget2D* AEquirectangularCubeCaptureActor::GetRenderTargetForOutput() const
{
	return OutputRenderTarget ? OutputRenderTarget : PreviewRenderTarget;
}
