#pragma once
#include "GameFramework/Character.h"
#include "Projectile.generated.h"

class UInputComponent;

UCLASS(config=Game)
class AProjectile : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, Category=Collider)
		class USphereComponent* Collider;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		class UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		class UProjectileMovementComponent* ProjectileMovement;

public:
	AProjectile();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE USphereComponent* GetCollisionComp() const { return Collider; }

protected:
	virtual void BeginPlay() override;

};

