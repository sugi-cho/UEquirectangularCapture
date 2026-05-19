#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EquirectangularCaptureActor.generated.h"

class USceneCaptureComponent2D;
class USceneCaptureComponentCube;
class USceneComponent;
class UTextureRenderTarget2D;
class UTextureRenderTargetCube;

UENUM(BlueprintType)
enum class EEquirectangularCaptureFace : uint8
{
	PositiveX UMETA(DisplayName = "Positive X"),
	NegativeX UMETA(DisplayName = "Negative X"),
	PositiveY UMETA(DisplayName = "Positive Y"),
	NegativeY UMETA(DisplayName = "Negative Y"),
	PositiveZ UMETA(DisplayName = "Positive Z"),
	NegativeZ UMETA(DisplayName = "Negative Z")
};

UCLASS(BlueprintType)
class UEQUIRECTANGULARCAPTURE_API AEquirectangularCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	AEquirectangularCaptureActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Equirectangular Capture")
	void RefreshResources();

	UFUNCTION(BlueprintCallable, Category = "Equirectangular Capture")
	void CaptureNow();

	UFUNCTION(BlueprintPure, Category = "Equirectangular Capture")
	UTextureRenderTarget2D* GetPreviewRenderTarget() const;

	UFUNCTION(BlueprintPure, Category = "Equirectangular Capture")
	UTextureRenderTarget2D* GetOutputRenderTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Equirectangular Capture")
	void SetFaceEnabled(EEquirectangularCaptureFace Face, bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Equirectangular Capture")
	bool IsFaceEnabled(EEquirectangularCaptureFace Face) const;

	UFUNCTION(BlueprintCallable, Category = "Equirectangular Capture")
	void SetAllFacesEnabled(bool bEnabled);

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

private:
	void InitializeCaptureComponents();
	void InitializeRenderTargets();
	uint32 GetEnabledFaceMask() const;
	UTextureRenderTarget2D* GetRenderTargetForOutput() const;
	void RenderPreview();
	void RenderPreviewFromFaces();
	void RenderPreviewFromCube();

#if WITH_EDITOR
	void RequestEditorCapture();
#endif

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Transient, Category = "Components")
	TArray<TObjectPtr<USceneCaptureComponent2D>> FaceCaptures;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneCaptureComponentCube> CubeCapture;

	UPROPERTY(VisibleAnywhere, Transient)
	TArray<TObjectPtr<UTextureRenderTarget2D>> FaceRenderTargets;

	UPROPERTY(VisibleAnywhere, Transient)
	TObjectPtr<UTextureRenderTargetCube> CubeRenderTarget;

	UPROPERTY(VisibleAnywhere, Transient)
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextureRenderTarget2D> OutputRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true", ClampMin = "64", UIMin = "64"))
	int32 FaceResolution = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true", ClampMin = "64", UIMin = "64"))
	int32 PreviewWidth = 2048;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true", ClampMin = "32", UIMin = "32"))
	int32 PreviewHeight = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true"))
	bool bAutoCapture = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true", DisplayName = "Use Synchronized Cube Capture", ToolTip = "Capture all directions in one cube capture and convert it to equirectangular output. Disable to use the legacy 6x SceneCapture2D path."))
	bool bUseSynchronizedCubeCapture = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", UIMin = "0.1"))
	float EditorCaptureInterval = 1.0f;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	bool bPendingEditorCapture = false;

	UPROPERTY(Transient)
	float EditorCaptureElapsed = 0.0f;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faces", meta = (AllowPrivateAccess = "true"))
	bool bPositiveX = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faces", meta = (AllowPrivateAccess = "true"))
	bool bNegativeX = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faces", meta = (AllowPrivateAccess = "true"))
	bool bPositiveY = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faces", meta = (AllowPrivateAccess = "true"))
	bool bNegativeY = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faces", meta = (AllowPrivateAccess = "true"))
	bool bPositiveZ = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faces", meta = (AllowPrivateAccess = "true"))
	bool bNegativeZ = true;
};
