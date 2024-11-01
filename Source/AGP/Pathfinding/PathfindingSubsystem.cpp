// Fill out your copyright notice in the Description page of Project Settings.


#include "PathfindingSubsystem.h"
#include "AGP/Characters/EnemyCharacter.h"
#include "EngineUtils.h"
#include "NavigationNode.h"
#include "Components/BoxComponent.h"

void UPathfindingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	UE_LOG(LogTemp, Warning, TEXT("Creating the UPathfindingSubsystem."))
	PopulateNodes();

	TArray<AActor*> HidingSpots;
	for(TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* CheckActor = *It;

		//check if actor has hideableobject tag
		if(CheckActor->ActorHasTag("HideableObject"))
		{
			HidingSpots.Add(CheckActor);
			UE_LOG(LogTemp, Error, TEXT("hiding spots: %d"), HidingSpots.Num());
		}
	}

	AddHidingSpotNode(HidingSpots);
}

TArray<FVector> UPathfindingSubsystem::GetWaypointPositions() const
{
	TArray<FVector> NodePositions;
	for (ANavigationNode* Node : Nodes)
	{
		if (Node)
		{
			NodePositions.Add(Node->GetActorLocation());
		}
	}
	return NodePositions;
}

TArray<FVector> UPathfindingSubsystem::GetRandomPath(const FVector& StartLocation)
{
	return GetPath(FindNearestNode(StartLocation), GetRandomNode());
}

TArray<FVector> UPathfindingSubsystem::GetPath(const FVector& StartLocation, const FVector& TargetLocation)
{
	return GetPath(FindNearestNode(StartLocation), FindNearestNode(TargetLocation));
}

TArray<FVector> UPathfindingSubsystem::GetPathAway(const FVector& StartLocation, const FVector& TargetLocation)
{
	return GetPath(FindNearestNode(StartLocation), FindFurthestNode(TargetLocation));
}

void UPathfindingSubsystem::PlaceProceduralNodes(const TArray<FVector>& LandscapeVertexData, int32 MapWidth, int32 MapHeight)
{
	// Clear existing nodes
	RemoveAllNodes();

	// Place nodes if they are above solid ground
	for (const FVector& Location : LandscapeVertexData)
	{
		if (IsLocationAboveSolidGround(Location)) // Use the existing function to verify solid ground
		{
			if (ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>())
			{
				Node->SetActorLocation(Location);
				ProcedurallyPlacedNodes.Add(Node);
			}
		}
	}
}

void UPathfindingSubsystem::PopulateNodes()
{
	Nodes.Empty();

	for (TActorIterator<ANavigationNode> It(GetWorld()); It; ++It)
	{
		Nodes.Add(*It);
		//UE_LOG(LogTemp, Warning, TEXT("NODE: %s"), *(*It)->GetActorLocation().ToString())
	}
}

void UPathfindingSubsystem::RemoveAllNodes()
{
	Nodes.Empty();
	ProcedurallyPlacedNodes.Empty();

	for (TActorIterator<ANavigationNode> It(GetWorld()); It; ++It)
	{
		GetWorld()->DestroyActor(*It);
	}
}

ANavigationNode* UPathfindingSubsystem::GetRandomNode()
{
	// Failure condition
	if (Nodes.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("The nodes array is empty."))
		return nullptr;
	}
	const int32 RandIndex = FMath::RandRange(0, Nodes.Num()-1);
	return Nodes[RandIndex];
}

ANavigationNode* UPathfindingSubsystem::FindNearestNode(const FVector& TargetLocation)
{
	// Failure condition.
	if (Nodes.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("The nodes array is empty."))
		return nullptr;
	}

	// Using the minimum programming pattern to find the closest node.
	// What is the Big O complexity of this? Can you do it more efficiently?
	ANavigationNode* ClosestNode = nullptr;
	float MinDistance = UE_MAX_FLT;
	for (ANavigationNode* Node : Nodes)
	{
		const float Distance = FVector::Distance(TargetLocation, Node->GetActorLocation());
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			ClosestNode = Node;
		}
	}

	return ClosestNode;
}

ANavigationNode* UPathfindingSubsystem::FindFurthestNode(const FVector& TargetLocation)
{
	// Failure condition.
	if (Nodes.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("The nodes array is empty."))
		return nullptr;
	}

	// Using the minimum programming pattern to find the closest node.
	// What is the Big O complexity of this? Can you do it more efficiently?
	ANavigationNode* FurthestNode = nullptr;
	float MaxDistance = -1.0f;
	for (ANavigationNode* Node : Nodes)
	{
		const float Distance = FVector::Distance(TargetLocation, Node->GetActorLocation());
		if (Distance > MaxDistance)
		{
			MaxDistance = Distance;
			FurthestNode = Node;
		}
	}

	return FurthestNode;
}

TArray<FVector> UPathfindingSubsystem::GetPath(ANavigationNode* StartNode, ANavigationNode* EndNode)
{
	if (!StartNode || !EndNode)
	{
		UE_LOG(LogTemp, Error, TEXT("Either the start or end node are nullptrs."))
		return TArray<FVector>();
	}

	// Setup the open set and add the start node.
	TArray<ANavigationNode*> OpenSet;
	OpenSet.Add(StartNode);

	// StartNode->GScore = UE_MAX_FLT;
	// StartNode->HScore = FVector::Distance(StartNode->GetActorLocation(), EndNode->GetActorLocation());

	// Setup the maps that will hold the GScores, HScores and CameFrom
	TMap<ANavigationNode*, float> GScores, HScores;
	TMap<ANavigationNode*, ANavigationNode*> CameFrom;
	// You could pre-populate the GScores and HScores maps with all of the GScores (at infinity) and HScores here by looping over
	// all the nodes in the Nodes array. However it is more efficient to only calculate these when you need them
	// as some nodes might not be explored when finding a path.

	// Setup the start nodes G and H score.
	GScores.Add(StartNode, 0);
	HScores.Add(StartNode, FVector::Distance(StartNode->GetActorLocation(), EndNode->GetActorLocation()));
	CameFrom.Add(StartNode, nullptr);

	while (!OpenSet.IsEmpty())
	{
		// Find the node in the open set with the lowest FScore.
		ANavigationNode* CurrentNode = OpenSet[0]; // We know this exists because the OpenSet is not empty.
		for (int32 i = 1; i < OpenSet.Num(); i++)
		{
			// We can be sure that all the nodes in the open set have already had their GScores and HScores calculated.
			if (GScores[OpenSet[i]] + HScores[OpenSet[i]] < GScores[CurrentNode] + HScores[CurrentNode])
			{
				CurrentNode = OpenSet[i];
			}
		}

		// Remove the current node from the OpenSet
		OpenSet.Remove(CurrentNode);

		if (CurrentNode == EndNode)
		{
			// Then we have found the path so reconstruct it and get the positions of each of the nodes in the path.
			// UE_LOG(LogTemp, Display, TEXT("PATH FOUND"))
			return ReconstructPath(CameFrom, EndNode);
		}

		for (ANavigationNode* ConnectedNode : CurrentNode->ConnectedNodes)
		{
			if (!ConnectedNode) continue; // Failsafe if the ConnectedNode is a nullptr.
			const float TentativeGScore = GScores[CurrentNode] + FVector::Distance(CurrentNode->GetActorLocation(), ConnectedNode->GetActorLocation());
			// Because we didn't setup all the scores and came from at the start, we need to check if the connected node has a gscore
			// already otherwise set it. If it doesn't have a gscore then it won't have all the other things either so initialise them as well.
			if (!GScores.Contains(ConnectedNode))
			{
				GScores.Add(ConnectedNode, UE_MAX_FLT);
				HScores.Add(ConnectedNode, FVector::Distance(ConnectedNode->GetActorLocation(), EndNode->GetActorLocation()));
				CameFrom.Add(ConnectedNode, nullptr);
			}

			// Then update this nodes scores and came from if the tentative g score is lower than the current g score.
			if (TentativeGScore < GScores[ConnectedNode])
			{
				CameFrom[ConnectedNode] = CurrentNode;
				GScores[ConnectedNode] = TentativeGScore;
				// HScore is already set when adding the node to the HScores map.
				// Then add connected node to the open set if it isn't already in there.
				if (!OpenSet.Contains(ConnectedNode))
				{
					OpenSet.Add(ConnectedNode);
				}
			}
		}
	}

	// If we get here, then no path has been found so return an empty array.
	return TArray<FVector>();
	
}

TArray<FVector> UPathfindingSubsystem::ReconstructPath(const TMap<ANavigationNode*, ANavigationNode*>& CameFromMap, ANavigationNode* EndNode)
{
	TArray<FVector> NodeLocations;

	const ANavigationNode* NextNode = EndNode;
	while(NextNode)
	{
		NodeLocations.Push(NextNode->GetActorLocation());
		NextNode = CameFromMap[NextNode];
	}

	return NodeLocations;
}

void UPathfindingSubsystem::UpdatePathfindingNodes(const TArray<FVector>& RoomAndCorridorLocations, int32 MapWidth, int32 MapHeight, float RoomSize)
{
    // Clear existing nodes
    RemoveAllNodes();

    // Place nodes at room and corridor locations
    for (const FVector& Location : RoomAndCorridorLocations)
    {
        if (ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>())
        {
            Node->SetActorLocation(Location);
            ProcedurallyPlacedNodes.Add(Node);
        }
    }

    // Separate room and corridor nodes for more precise connection logic
    TArray<ANavigationNode*> RoomNodes;
    TArray<ANavigationNode*> CorridorNodes;

    for (ANavigationNode* Node : ProcedurallyPlacedNodes)
    {
        if (IsCorridorNode(Node))
        {
            CorridorNodes.Add(Node);
        }
        else
        {
            RoomNodes.Add(Node);
        }
    }

    // Connect room nodes to each other
    for (ANavigationNode* RoomNode : RoomNodes)
    {
        for (ANavigationNode* OtherRoomNode : RoomNodes)
        {
            if (RoomNode != OtherRoomNode && FVector::Dist(RoomNode->GetActorLocation(), OtherRoomNode->GetActorLocation()) <= RoomSize)
            {
                RoomNode->ConnectedNodes.Add(OtherRoomNode);
            }
        }
    }

    // Connect corridor nodes to room nodes and selectively to other corridor nodes
    for (ANavigationNode* CorridorNode : CorridorNodes)
    {
        // Connect corridor nodes to nearby room nodes
        for (ANavigationNode* RoomNode : RoomNodes)
        {
            if (FVector::Dist(CorridorNode->GetActorLocation(), RoomNode->GetActorLocation()) <= RoomSize)
            {
                CorridorNode->ConnectedNodes.Add(RoomNode);
                RoomNode->ConnectedNodes.Add(CorridorNode);
            }
        }

        // Connect corridor nodes to other corridor nodes only if they’re not immediately adjacent
        for (ANavigationNode* OtherCorridorNode : CorridorNodes)
        {
            if (CorridorNode != OtherCorridorNode)
            {
                float Distance = FVector::Dist(CorridorNode->GetActorLocation(), OtherCorridorNode->GetActorLocation());

                // Only connect if the distance is within range but not too close
                if (Distance > RoomSize * 0.5f && Distance <= RoomSize)  // Adjust the 0.5f factor as needed
                {
                    CorridorNode->ConnectedNodes.Add(OtherCorridorNode);
                }
            }
        }
    }
}

bool UPathfindingSubsystem::IsCorridorNode(ANavigationNode* Node)
{
	// Determine if a node is part of a corridor based on naming, tag, or another property
	return Node->GetActorLabel().Contains("Corridor"); // Adjust based on your naming/tagging setup
}

bool UPathfindingSubsystem::IsLocationAboveSolidGround(const FVector& Location) const
{
	FVector Start = Location + FVector(0, 0, 100.0f); // Start a bit above the location
	FVector End = Location - FVector(0, 0, 1000.0f);  // Trace downward

	FHitResult HitResult;

	// Perform the line trace
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility // You could use a custom channel if needed
	);

	// Draw a debug line to visualize the trace
	DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 1.0f);

	// Return true if the trace hits something (indicating solid ground)
	return bHit;
}

void UPathfindingSubsystem::AddHidingSpotNode(TArray<AActor*> HidingSpots)
{
	//get location of box collider
	for(AActor* HidingSpot : HidingSpots)
	{
		if(HidingSpot)
		{
			UBoxComponent* BoxCollider = Cast<UBoxComponent>(HidingSpot->GetComponentByClass(UBoxComponent::StaticClass()));

			FVector Spot = BoxCollider->GetComponentLocation();
			Spot.Z -= 96;

			ANavigationNode* HidingNode = GetWorld()->SpawnActor<ANavigationNode>(ANavigationNode::StaticClass(), Spot, FRotator::ZeroRotator);

			if(HidingNode)
			{
				Nodes.Add(HidingNode);
				UE_LOG(LogTemp, Error, TEXT("Added new hiding spot node"));
				ConnectToOtherNodes(HidingNode);
			} else
			{
				UE_LOG(LogTemp, Error, TEXT("Couldn't add new hiding spot node"));
			}
		}
	}
	//add node to hiding spot
}

void UPathfindingSubsystem::ConnectToOtherNodes(ANavigationNode* HidingNode)
{
	//if a node is close to new hiding spot node, add a connection between them
	for(ANavigationNode* Node : Nodes)
	{
		if(Node != HidingNode)
		{
			float distance = FVector::Distance(Node->GetActorLocation(), HidingNode->GetActorLocation());
			if(distance <= 250.0f)
			{
				HidingNode->ConnectedNodes.Add(Node);
				Node->ConnectedNodes.Add(HidingNode);
			}
		}
	}
} 