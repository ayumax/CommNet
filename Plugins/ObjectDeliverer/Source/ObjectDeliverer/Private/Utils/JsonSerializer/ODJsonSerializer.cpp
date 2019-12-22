// Copyright 2019 ayumax. All Rights Reserved.
#include "ODJsonSerializer.h"
#include "Utils/ODObjectUtil.h"
#include "JsonObjectConverter.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/PropertyPortFlags.h"
#include "IODConvertPropertyName.h"
#include "ODOverrideJsonSerializer.h"

UODJsonSerializer::UODJsonSerializer()
{
	DefaultObjectSerializer = CreateDefaultSubobject<UODNoWriteTypeJsonSerializer>(TEXT("DefaultObjectSerializer"));
}

void UODJsonSerializer::AddOverrideJsonSerializers(const TMap<UClass*, UODOverrideJsonSerializer*>& OverrideObjectSerializers)
{
	ObjectSerializers = OverrideObjectSerializers;
}

TSharedPtr<FJsonObject> UODJsonSerializer::CreateJsonObject(const UObject* Obj, int64 CheckFlags /*= 0*/, int64 SkipFlags /*= 0*/)
{
	return UObjectToJsonObject(Obj, CheckFlags, SkipFlags);
}

TSharedPtr<FJsonValue> UODJsonSerializer::ObjectJsonCallback(UProperty* Property, const void* Value)
{
	if (UObjectProperty* ObjectProperty = Cast<UObjectProperty>(Property))
	{
		if (!ObjectProperty->HasAnyFlags(RF_Transient)) // We are taking Transient to mean we don't want to serialize to Json either (could make a new flag if nessasary)
		{
			return MakeShareable(new FJsonValueObject(UObjectToJsonObject(ObjectProperty->GetObjectPropertyValue(Value))));
		}
	}

	// invalid
	return TSharedPtr<FJsonValue>();
}

TSharedPtr<FJsonObject> UODJsonSerializer::UObjectToJsonObject(const UObject* Obj, int64 CheckFlags, int64 SkipFlags)
{
	auto objectSerializer = DefaultObjectSerializer;

	if (ObjectSerializers.Contains(Obj->GetClass()))
	{
		objectSerializer = ObjectSerializers[Obj->GetClass()];
	}

	return objectSerializer->UObjectToJsonObject(this, Obj, CheckFlags, SkipFlags);
}

void UODJsonSerializer::AddJsonValue(TSharedPtr<FJsonObject> JsonObject, const UObject* Obj, UProperty* Property, int64 CheckFlags, int64 SkipFlags)
{
	FString PropertyName = Property->GetName();
	if (Obj->GetClass()->ImplementsInterface(UODConvertPropertyName::StaticClass()))
	{
		PropertyName = IODConvertPropertyName::Execute_ConvertUPropertyName(Obj, Property->GetFName());
	}

	uint8* CurrentPropAddr = Property->ContainerPtrToValuePtr<uint8>((UObject*)Obj);

	FJsonObjectConverter::CustomExportCallback CustomCB;
	CustomCB.BindLambda([this](UProperty* Property, const void* Value)
		{
			return ObjectJsonCallback(Property, Value);
		});

	JsonObject->SetField(PropertyName, FJsonObjectConverter::UPropertyToJsonValue(Property, CurrentPropAddr, CheckFlags, SkipFlags, &CustomCB));
}