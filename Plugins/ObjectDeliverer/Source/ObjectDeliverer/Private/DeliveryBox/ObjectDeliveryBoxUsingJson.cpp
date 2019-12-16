// Copyright 2019 ayumax. All Rights Reserved.
#include "DeliveryBox/ObjectDeliveryBoxUsingJson.h"
#include "Utils/ODStringUtil.h"
#include "../Utils/JsonSerializer/ODJsonDeserializer.h"
#include "../Utils/JsonSerializer/ODJsonSerializer.h"

UObjectDeliveryBoxUsingJson::UObjectDeliveryBoxUsingJson()
{
	Serializer = CreateDefaultSubobject<UODJsonSerializer>(TEXT("UODJsonSerializer"));
	Deserializer = CreateDefaultSubobject<UODJsonDeserializer>(TEXT("UODJsonDeserializer"));
}

UObjectDeliveryBoxUsingJson::~UObjectDeliveryBoxUsingJson()
{
}

void UObjectDeliveryBoxUsingJson::Initialize(UClass* _TargetClass)
{
	TargetClass = _TargetClass;
}

void UObjectDeliveryBoxUsingJson::Send(const UObject* message)
{
	SendTo(message, nullptr);
}

void UObjectDeliveryBoxUsingJson::SendTo(const UObject* message, const UObjectDelivererProtocol* Destination)
{
	auto jsonObject = Serializer->CreateJsonObject(message);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(jsonObject.ToSharedRef(), Writer);

	TArray<uint8> buffer;
	UODStringUtil::StringToBuffer(OutputString, buffer);

	RequestSend.ExecuteIfBound(Destination, buffer);
}

void UObjectDeliveryBoxUsingJson::NotifyReceiveBuffer(const UObjectDelivererProtocol* FromObject, const TArray<uint8>& buffer)
{
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(UODStringUtil::BufferToString(buffer));
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid()) return;

	UObject* createdObj = Deserializer->JsonObjectToUObject(JsonObject, TargetClass);

	Received.Broadcast(createdObj, FromObject);
}

