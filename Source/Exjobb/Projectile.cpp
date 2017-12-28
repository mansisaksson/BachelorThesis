// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Exjobb.h"
#include "Projectile.h"
//#include "ProjectileMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectile::AProjectile()
{
	Collider = CreateDefaultSubobject<USphereComponent>(TEXT("Collider"));
	Collider->SetSphereRadius(32.f);
	SetRootComponent(Collider);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Projectile Mesh"));
	Mesh->SetupAttachment(Collider);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

}