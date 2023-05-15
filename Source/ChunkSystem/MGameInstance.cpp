// Fill out your copyright notice in the Description page of Project Settings.


#include "MGameInstance.h"
#include "ChunkDownloader.h"
#include "Misc/CoreDelegates.h"
#include "AssetRegistryModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogPatch, Display, Display);

void UMGameInstance::Init()
{
	Super::Init();

	const FString DeploymentName = "Patcher-Live";
	const FString ContentBuildId = "webData";

	// initialize the chunk downloader
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetOrCreate();
	Downloader->Initialize("Android", 8);

	// load the cached build ID
	Downloader->LoadCachedBuild(DeploymentName);

	// update the build manifest file
	TFunction<void(bool bSuccess)> UpdateCompleteCallback = [&](bool bSuccess) {bIsDownloadManifestUpToDate = bSuccess; };
	Downloader->UpdateBuild(DeploymentName, ContentBuildId, UpdateCompleteCallback);
}

void UMGameInstance::Shutdown()
{
	Super::Shutdown();

	// Shutdown the chunk downloader
	FChunkDownloader::Shutdown();
}

bool UMGameInstance::DownloadSingleChunk(int32 ChunkID)
{

	if (!bIsDownloadManifestUpToDate)
	{
		// we couldn't contact the server to validate our manifest, so we can't patch
		UE_LOG(LogPatch, Display, TEXT("Manifest Update Failed. Can't patch the game"));

		return false;
	}

	// ignore individual downloads if we're still patching the game
	if (bIsPatchingGame)
	{
		UE_LOG(LogPatch, Display, TEXT("Main game patching underway. Ignoring single chunk downloads."));

		return false;
	}

	// make sure we're not trying to download the chunk multiple times
	if (SingleChunkDownloadList.Contains(ChunkID))
	{
		UE_LOG(LogPatch, Display, TEXT("Chunk: %i already downloading"), ChunkID);

		return false;
	}

	// raise the single chunk download flag
	bIsDownloadingSingleChunks = true;

	// add the chunk to our download list
	SingleChunkDownloadList.Add(ChunkID);

	UE_LOG(LogPatch, Display, TEXT("Downloading specific Chunk: %i"), ChunkID);

	// get the chunk downloader
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

	// prepare the pak mounting callback lambda function
	TFunction<void(bool)> MountCompleteCallback = [this](bool bSuccess)
	{
		if (bSuccess)
		{
			UE_LOG(LogPatch, Display, TEXT("Single Chunk Mount complete"));

			// call the delegate
			OnSingleChunkPatchComplete.Broadcast(true);
		}
		else {
			UE_LOG(LogPatch, Display, TEXT("Single Mount failed"));

			// call the delegate
			OnSingleChunkPatchComplete.Broadcast(false);
		}
	};

	// mount the Paks so their assets can be accessed
	Downloader->MountChunk(ChunkID, MountCompleteCallback);

	return true;
}

void UMGameInstance::OnSingleChunkDownloadComplete(bool bSuccess)
{
	bIsDownloadingSingleChunks = false;

	// was Pak download successful?
	if (!bSuccess)
	{
		UE_LOG(LogPatch, Display, TEXT("Patch Download Failed"));

		// call the delegate
		OnSingleChunkPatchComplete.Broadcast(false);

		return;
	}

	UE_LOG(LogPatch, Display, TEXT("Patch Download Complete"));

	// get the chunk downloader
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

	// build the downloaded chunk list
	FJsonSerializableArrayInt DownloadedChunks;

	for (int32 ChunkID : SingleChunkDownloadList)
	{
		DownloadedChunks.Add(ChunkID);
	}

	// prepare the pak mounting callback lambda function
	TFunction<void(bool)> MountCompleteCallback = [this](bool bSuccess)
	{
		if (bSuccess)
		{
			UE_LOG(LogPatch, Display, TEXT("Single Chunk Mount complete"));

			// call the delegate
			OnSingleChunkPatchComplete.Broadcast(true);
		}
		else {
			UE_LOG(LogPatch, Display, TEXT("Single Mount failed"));

			// call the delegate
			OnSingleChunkPatchComplete.Broadcast(false);
		}
	};

	// mount the Paks so their assets can be accessed
	Downloader->MountChunks(DownloadedChunks, MountCompleteCallback);
}

bool UMGameInstance::IsChunkLoaded(int32 ChunkID)
{
	// ensure our manifest is up to date
	if (!bIsDownloadManifestUpToDate)
		return false;

	// get the chunk downloader
	TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

	// query the chunk downloader for chunk status. Return true only if the chunk is mounted
	return Downloader->GetChunkStatus(ChunkID) == FChunkDownloader::EChunkStatus::Mounted;
}
