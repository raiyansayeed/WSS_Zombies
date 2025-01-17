// Fill out your copyright notice in the Description page of Project Settings.

#include "SWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "CulminatingProj.h"

#define OUT 

// Sets default values
ASWeapon::ASWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;

	MuzzleSocketName = "MuzzleSocket";

	BaseDamage = 20.f;

	AmmoCount = 30;

	AmmoMax = 330;

	FireRate = 0.1;

	OriginalCount = AmmoCount;

	OriginalMax = AmmoMax;

}

// Called when the game starts or when spawned
void ASWeapon::BeginPlay()
{
	Super::BeginPlay();

}

void ASWeapon::Fire()
{
	if (AmmoCount == 0) {
		return;
	}

	AActor* MyOwner = GetOwner();
	if (MyOwner) {
		FVector EyeLocation;
		FRotator EyeRotation;

		UE_LOG(LogTemp, Warning, TEXT("MyOwner(): %s"), *(MyOwner->GetName()));
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		//calculation of shot trajectory 
		FVector ShotDirection = EyeRotation.Vector();
		FVector TraceEnd = EyeLocation + ShotDirection * 1000 * 5;


		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(Projectile_Class, MuzzleLocation, EyeRotation, SpawnParams);

		//ignore materials, actors except enemies 
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(MyOwner);
		QueryParams.AddIgnoredActor(this);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;

		FHitResult Hit;

		//if enemy is in line of trajectory 
		if (GetWorld()->LineTraceSingleByChannel(OUT Hit, MuzzleLocation, TraceEnd, COLLISION_WEAPON, QueryParams))
		{
			//Blocking hit, process damage
			AActor* HitActor = Hit.GetActor();

			EPhysicalSurface SurfaceType = UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

			ActualDamage = BaseDamage;
			
			//Headshot damage
			if (SurfaceType == SURFACE_HEADFLESH)
			{
				ActualDamage *= 2.5;
			}

			UGameplayStatics::ApplyPointDamage(HitActor, ActualDamage, ShotDirection, Hit, MyOwner->GetInstigatorController(), this, DamageType);

			UParticleSystem* SelectedEffect = nullptr;

			//headshot (SURFACE_FLESHVULNERABLE) or not? 
			switch (SurfaceType)
			{
			case SURFACE_FLESHDEFAULT:
				SelectedEffect = FleshImpactEffect;
				break;
			case SURFACE_HEADFLESH:
				SelectedEffect = FleshImpactEffect;
				break;
			default:
				SelectedEffect = DefaultImpactEffect;
				break;
			}

			if (SelectedEffect)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SelectedEffect, Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
			}

		}

		FireEffects(MuzzleLocation, TraceEnd);
		//FireEffects(EyeLocation, TraceEnd);
		AmmoCount--;
		OnAmmoUsed.Broadcast(AmmoCount);
		//GetWorld()->GetTimerManager().ClearTimer(FireRateHandle);

	}
}

void ASWeapon::Used()
{
	GetWorld()->GetTimerManager().SetTimer(FireRateHandle, this, &ASWeapon::Fire, FireRate, false, 0.f);
}

void ASWeapon::Stop()
{
	GetWorld()->GetTimerManager().ClearTimer(FireRateHandle);
}

void ASWeapon::Reload()
{
	AmmoMax -= OriginalCount - AmmoCount;
	AmmoCount = OriginalCount;
}

void ASWeapon::StockUp()
{
	AmmoMax = OriginalMax;
}

// Called every frame
void ASWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASWeapon::FireEffects(FVector EyeLocation, FVector TraceEnd)
{
		//red line shooting out of primary gun
		//DrawDebugLine(GetWorld(), EyeLocation, TraceEnd, FColor::Red, false, 1.5, 0, 1.f);
		//DrawDebugCircle(GetWorld(), TraceEnd, 20, 100, FColor(255, 0, 255), true, 2.f);
		
		if (MuzzleEffect)
		{
			UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
		}
	
		//Cast to pawn for access to camera shake method
		APawn* MyOwner = Cast<APawn>(GetOwner());
		if (MyOwner)
		{
			APlayerController* PC = Cast<APlayerController>(MyOwner->GetController());
			if (PC)
			{
				PC->ClientPlayCameraShake(FireCamShake);
			}
		}
}

