// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "FlowSave.h"
#include "FlowTypes.h"
#include "Interfaces/FlowOwnerInterface.h"
#include "FlowComponent.generated.h"

class UFlowAsset;
class UFlowSubsystem;

USTRUCT()
struct FNotifyTagReplication
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag ActorTag;

	UPROPERTY()
	FGameplayTag NotifyTag;

	FNotifyTagReplication() {}

	FNotifyTagReplication(const FGameplayTag& InActorTag, const FGameplayTag& InNotifyTag)
		: ActorTag(InActorTag)
		, NotifyTag(InNotifyTag)
	{
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlowComponentTagsReplicated, class UFlowComponent*, FlowComponent, const FGameplayTagContainer&, CurrentTags);

DECLARE_MULTICAST_DELEGATE_TwoParams(FFlowComponentNotify, class UFlowComponent*, const FGameplayTag&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFlowComponentDynamicNotify, class UFlowComponent*, FlowComponent, const FGameplayTag&, NotifyTag);

/**
* Base component of Flow System - makes possible to communicate between Actor, Flow Subsystem and Flow Graphs
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class FLOW_API UFlowComponent : public UActorComponent, public IFlowOwnerInterface
{
	GENERATED_UCLASS_BODY()

	friend class UFlowSubsystem;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
//////////////////////////////////////////////////////////////////////////
// Identity Tags

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow", meta = (Validate, Categories = "Flow"))
	FGameplayTagContainer IdentityTags;

private:
	// Used to replicate tags added during gameplay
	UPROPERTY(ReplicatedUsing = OnRep_AddedIdentityTags)
	FGameplayTagContainer AddedIdentityTags;

	// Used to replicate tags removed during gameplay
	UPROPERTY(ReplicatedUsing = OnRep_RemovedIdentityTags)
	FGameplayTagContainer RemovedIdentityTags;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void AddIdentityTag(UPARAM(meta = (Categories = "Flow")) const FGameplayTag Tag, const EFlowNetMode NetMode = EFlowNetMode::Authority);

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void AddIdentityTags(UPARAM(meta = (Categories = "Flow")) FGameplayTagContainer Tags, const EFlowNetMode NetMode = EFlowNetMode::Authority);

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void RemoveIdentityTag(UPARAM(meta = (Categories = "Flow")) const FGameplayTag Tag, const EFlowNetMode NetMode = EFlowNetMode::Authority);

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void RemoveIdentityTags(UPARAM(meta = (Categories = "Flow")) FGameplayTagContainer Tags, const EFlowNetMode NetMode = EFlowNetMode::Authority);

protected:
	void RegisterWithFlowSubsystem();
	void UnregisterWithFlowSubsystem();
	virtual void BeginRootFlow(bool bComponentLoadedFromSaveGame);

private:
	UFUNCTION()
	void OnRep_AddedIdentityTags();

	UFUNCTION()
	void OnRep_RemovedIdentityTags();

public:
	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FFlowComponentTagsReplicated OnIdentityTagsAdded;

	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FFlowComponentTagsReplicated OnIdentityTagsRemoved;

public:
	void VerifyIdentityTags() const;
		
	UFUNCTION(BlueprintCallable, Category = "Flow")
	void LogError(FString Message, const EFlowOnScreenMessageType OnScreenMessageType = EFlowOnScreenMessageType::Permanent) const;

//////////////////////////////////////////////////////////////////////////
// Component sending Notify Tags to Flow Graph, or any other listener

private:
	// Stores only recently sent tags
	UPROPERTY(ReplicatedUsing = OnRep_SentNotifyTags)
	FGameplayTagContainer RecentlySentNotifyTags;

public:
	const FGameplayTagContainer& GetRecentlySentNotifyTags() const { return RecentlySentNotifyTags; }

	// Send single notification from the actor to Flow graphs
	// If set on server, it always going to be replicated to clients
	UFUNCTION(BlueprintCallable, Category = "Flow")
	void NotifyGraph(const FGameplayTag NotifyTag, const EFlowNetMode NetMode = EFlowNetMode::Authority);

	// Send multiple notifications at once - from the actor to Flow graphs
	// If set on server, it always going to be replicated to clients
	UFUNCTION(BlueprintCallable, Category = "Flow")
	void BulkNotifyGraph(const FGameplayTagContainer NotifyTags, const EFlowNetMode NetMode = EFlowNetMode::Authority);

private:
	UFUNCTION()
	void OnRep_SentNotifyTags();

public:
	FFlowComponentNotify OnNotifyFromComponent;

//////////////////////////////////////////////////////////////////////////
// Component receiving Notify Tags from Flow Graph

private:
	// Stores only recently replicated tags
	UPROPERTY(ReplicatedUsing = OnRep_NotifyTagsFromGraph)
	FGameplayTagContainer NotifyTagsFromGraph;

public:
	virtual void NotifyFromGraph(const FGameplayTagContainer& NotifyTags, const EFlowNetMode NetMode = EFlowNetMode::Authority);

private:
	UFUNCTION()
	void OnRep_NotifyTagsFromGraph();

public:
	// Receive notification from Flow graph or another Flow Component
	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FFlowComponentDynamicNotify ReceiveNotify;

//////////////////////////////////////////////////////////////////////////
// Sending Notify Tags between Flow components

private:
	// Stores only recently replicated tags
	UPROPERTY(ReplicatedUsing = OnRep_NotifyTagsFromAnotherComponent)
	TArray<FNotifyTagReplication> NotifyTagsFromAnotherComponent;

public:
	// Send notification to another actor containing Flow Component
	UFUNCTION(BlueprintCallable, Category = "Flow")
	virtual void NotifyActor(const FGameplayTag ActorTag, const FGameplayTag NotifyTag, const EFlowNetMode NetMode = EFlowNetMode::Authority);

private:
	UFUNCTION()
	void OnRep_NotifyTagsFromAnotherComponent();

//////////////////////////////////////////////////////////////////////////
// Root Flow

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RootFlow")
	bool bHasRootFlow = false;
	
	// Asset that might instantiated as "Root Flow" 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RootFlow", meta = (EditCondition = "bHasRootFlow", Validate))
	UFlowAsset* RootFlow;

	// If true, component will start Root Flow on Begin Play
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RootFlow", meta = (EditCondition = "bHasRootFlow"))
	bool bAutoStartRootFlow;

	// Networking mode for creating this Root Flow
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RootFlow", meta = (EditCondition = "bHasRootFlow"))
	EFlowNetMode RootFlowMode;

	// If false, another Root Flow instance won't be created from this component, if this Flow Asset is already instantiated
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RootFlow", meta = (EditCondition = "bHasRootFlow"))
	bool bAllowMultipleInstances;

	UPROPERTY(SaveGame)
	FString SavedAssetInstanceName;
	
	// This will instantiate Flow Asset assigned on this component.
	// Created Flow Asset instance will be a "root flow", as additional Flow Assets can be instantiated via Sub Graph node
	UFUNCTION(BlueprintCallable, Category = "RootFlow")
	void StartRootFlow();

	// This will destroy instantiated Flow Asset - created from asset assigned on this component.
	UFUNCTION(BlueprintCallable, Category = "RootFlow")
	void FinishRootFlow(UFlowAsset* TemplateAsset, const EFlowFinishPolicy FinishPolicy);

	UFUNCTION(BlueprintPure, Category = "FlowSubsystem")
	TSet<UFlowAsset*> GetRootInstances(const UObject* Owner) const;

	UFUNCTION(BlueprintPure, Category = "RootFlow", meta = (DeprecatedFunction, DeprecationMessage="Use GetRootInstances() instead."))
	UFlowAsset* GetRootFlowInstance() const;

//////////////////////////////////////////////////////////////////////////
// UFlowComponent overrideable events

public:
	// Called when a Root flow asset triggers a CustomOutput
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnTriggerRootFlowOutputEvent")
	void BP_OnTriggerRootFlowOutputEvent(UFlowAsset* RootFlowInstance, const FName& EventName);

	virtual void OnTriggerRootFlowOutputEvent(UFlowAsset* RootFlowInstance, const FName& EventName) {}

	// UFlowAsset-only access
	void OnTriggerRootFlowOutputEventDispatcher(UFlowAsset* RootFlowInstance, const FName& EventName);
	// ---

//////////////////////////////////////////////////////////////////////////
// SaveGame

public:
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void SaveRootFlow(TArray<FFlowAssetSaveData>& SavedFlowInstances);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	virtual void LoadRootFlow();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	FFlowComponentSaveData SaveInstance();

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	bool LoadInstance();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	void OnSave();
	
	UFUNCTION(BlueprintNativeEvent, Category = "SaveGame")
	void OnLoad();
	
//////////////////////////////////////////////////////////////////////////
// Helpers

public:
	UFlowSubsystem* GetFlowSubsystem() const;
	bool IsFlowNetMode(const EFlowNetMode NetMode) const;
};
