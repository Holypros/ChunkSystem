#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "MGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatchCompleteDelegate, bool, Succeeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FChunkMountedDelegate, int32, ChunkID, bool, Succeeded);

UCLASS()
class CHUNKSYSTEM_API UMGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "Patching")
		bool DownloadSingleChunk(int32 ChunkID);


	void OnSingleChunkDownloadComplete(bool bSuccess);

	UFUNCTION(BlueprintPure, Category = "Patching")
		bool IsChunkLoaded(int32 ChunkID);


	UPROPERTY(BlueprintAssignable, Category = "Patching");
	FPatchCompleteDelegate OnPatchReady;

	UPROPERTY(BlueprintAssignable, Category = "Patching");
	FPatchCompleteDelegate OnSingleChunkPatchComplete;

	TArray<int32> SingleChunkDownloadList;

	bool bIsDownloadingSingleChunks = false;

protected:

	FString PatchPlatform;

	UPROPERTY(EditDefaultsOnly, Category = "Patching")
		FString PatchVersionURL;

	bool bIsPatchingGame = false;

	bool bIsDownloadManifestUpToDate = false;
	
};
