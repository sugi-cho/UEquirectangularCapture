#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EquirectangularCubeCaptureActor.generated.h"

class USceneCaptureComponentCube;
class USceneComponent;
class UTextureRenderTarget2D;
class UTextureRenderTargetCube;

UCLASS(BlueprintType)
class UEQUIRECTANGULARCAPTURE_API AEquirectangularCubeCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	AEquirectangularCubeCaptureActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Equirectangular Cube Capture")
	void RefreshResources();

	UFUNCTION(BlueprintCallable, Category = "Equirectangular Cube Capture")
	void CaptureNow();

	UFUNCTION(BlueprintPure, Category = "Equirectangular Cube Capture")
	UTextureRenderTarget2D* GetPreviewRenderTarget() const;

	UFUNCTION(BlueprintPure, Category = "Equirectangular Cube Capture")
	UTextureRenderTarget2D* GetOutputRenderTarget() const;

	UFUNCTION(BlueprintPure, Category = "Equirectangular Cube Capture")
	UTextureRenderTargetCube* GetCubeRenderTarget() const;

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

private:
	void InitializeRenderTargets();
	UTextureRenderTarget2D* GetRenderTargetForOutput() const;
	void RenderPreview();

#if WITH_EDITOR
	void RequestEditorCapture();
#endif

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneCaptureComponentCube> CubeCapture;

	UPROPERTY(VisibleAnywhere, Transient)
	TObjectPtr<UTextureRenderTargetCube> CubeRenderTarget;

	UPROPERTY(VisibleAnywhere, Transient)
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextureRenderTarget2D> OutputRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true", ClampMin = "64", UIMin = "64"))
	int32 CubeResolution = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true", ClampMin = "64", UIMin = "64"))
	int32 PreviewWidth = 2048;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true", ClampMin = "32", UIMin = "32"))
	int32 PreviewHeight = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true"))
	bool bAutoCapture = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", UIMin = "0.1"))
	float EditorCaptureInterval = 1.0f;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Transient)
	bool bPendingEditorCapture = false;

	UPROPERTY(Transient)
	float EditorCaptureElapsed = 0.0f;
#endif
};
