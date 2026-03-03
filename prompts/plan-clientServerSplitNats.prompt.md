# Plan: Client/Server Split + NATS Map Management

The project splits into a dedicated UE game server (game logic, replication, NATS for all service comms) and a UE client (auth, character creation/selection, UI, gRPC). gRPC and TurboLink are **client-only**. The server never instantiates any `UGrpcHandlerSubsystem` subclass.

---

## Communication Architecture

```
[UE Game Client] ──gRPC──► [Character Service]
                 ──gRPC──► [Chat Service]

[UE Game Server] ──NATS──► [Character Service]   (fetch appearance on player join)
                 ──NATS──► [Persistence Service]  (save player state)
                 ──NATS──► [Map Manager]           (receive map assignments)
                 ──NATS──► [Chat Service]           (relay in-world events)
```

---

## Phase 1 — Module Split

Move all client-only code into a new `TPSCoreMechanicsClient` module. `TPSCoreMechanics` becomes the **shared** module with no gRPC dependency.

**Moves to `TPSCoreMechanicsClient`:**
- All of `Source/TPSCoreMechanics/Public/CharacterCreation/` — orchestrator, creation, selection, game mode base, `SelectedCharacterSubsystem`
- `Auth/KeycloakAuthService.h` and `TokenManager`
- `Chat/ChatSubsystem.h`
- `Characters/CharactersSubsystem.h` — gRPC-based, client-only
- `GrpcHandlerSubsystem.h` — base gRPC class, moves with its dependents
- All of `UI/`

**Stays shared in `TPSCoreMechanics`:**
- All proto-generated data structs (reused as C++ types regardless of transport)
- `CommonTypes.h`, `ItemTypes.h`
- GAS: `TPSCoreAbilitySystemComponent`, `TPSCoreAttributeSet`, `CharacterClassInfo`
- Inventory: `InventoryComponent`, `ItemTypesToTables`
- Game framework: `TPSCoreGameMode`, `TPSCorePlayerController`, `TPSCorePlayerState`, `TPSCoreMechanicsCharacter`
- `CharacterAppearanceHelper`

**Steps:**
1. Create `Source/TPSCoreMechanicsClient/` with `TPSCoreMechanicsClient.Build.cs` — depends on `TPSCoreMechanics` + `TurboLinkGrpc`
2. Remove `TurboLinkGrpc` from `TPSCoreMechanics.Build.cs` entirely
3. Move headers/sources; fix all `#include` paths
4. Confirm shared module compiles with no TurboLink or auth symbols

---

## Phase 2 — Server & Client Targets

1. Add `Source/TPSCoreMechanicsServer.Target.cs` — `TargetType.Server`, modules: `["TPSCoreMechanics", "NatsClient"]`, explicitly excludes `TPSCoreMechanicsClient`
2. Update `Source/TPSCoreMechanics.Target.cs` and `TPSCoreMechanicsEditor.Target.cs` to include `TPSCoreMechanicsClient`
3. `Config/DefaultEngine.ini` server section: `GameDefaultMap` → game world map, `GlobalDefaultGameMode` → `ATPSCoreGameMode`
4. `Config/DefaultServerGame.ini`: NATS broker URL, server instance ID, character service NATS subject prefix

---

## Phase 3 — NATS Plugin (`Plugins/NatsClient/`)

Wrap [nats.c](https://github.com/nats-io/nats.c) as a first-party UE plugin, platform libs for Mac + Linux (server) + Win64 (dev).

**Layout:**
```
Plugins/NatsClient/
  NatsClient.uplugin
  Source/NatsClient/
    NatsClient.Build.cs
    Public/
      NatsClientSubsystem.h     ← UGameInstanceSubsystem
      NatsMessage.h             ← FNatsMessage { FString Subject; TArray<uint8> Payload; }
      NatsSubscription.h        ← RAII handle, fires FOnNatsMessage delegate
    Private/
      NatsClientSubsystem.cpp
    ThirdParty/nats.c/          ← vendored static lib per platform
```

**`UNatsClientSubsystem` API:**
- `Connect(FString Url, FString CredentialsOrNKey)` → `OnConnected` / `OnConnectionLost` delegates
- `PublishJson(FString Subject, FString JsonPayload)`
- `RequestJson(FString Subject, FString JsonPayload, float TimeoutSecs, FOnNatsReply Delegate)` — covers the synchronous fetch pattern
- `Subscribe(FString Subject, FOnNatsMessage Delegate) → FNatsSubscriptionHandle`
- `Unsubscribe(FNatsSubscriptionHandle)`

---

## Phase 4 — Server-Side Character Loading

On player join, the server fetches character appearance over NATS instead of gRPC.

**New class:** `UServerCharacterLoaderSubsystem` — server-only `GameInstanceSubsystem` (guarded by `IsRunningDedicatedServer()`). No gRPC dependency whatsoever.

**Flow:**
1. Client sends `CharacterId` as a URL option during `OpenLevel` (e.g. `?CharacterId=abc123`)
2. `ATPSCoreGameMode::PostLogin(APlayerController*)` reads the option, calls `UServerCharacterLoaderSubsystem::FetchCharacter(CharacterId, Callback)`
3. Subsystem calls `UNatsClientSubsystem::RequestJson("characters.get", "{\"id\":\"abc123\"}", 5.0f, ...)`
4. On reply: deserializes JSON into `FGrpcCharactersCharacter` (proto structs reused, just populated from JSON instead of protobuf wire format)
5. `ATPSCoreMechanicsCharacter::InitializeFromCharacterData()` called authoritatively on server; replication propagates appearance to all clients

**Auth:** deferred — static service token in ini for now.

---

## Phase 5 — Map Management via NATS

**New class:** `UServerMapManagerSubsystem` — server-only `GameInstanceSubsystem`.

On `Initialize()`:
- Reads `InstanceId` from `-InstanceId=xyz` command-line arg
- Subscribes to `server.{InstanceId}.map` — payload: `{ "map": "/Game/Maps/Zone1" }` → calls `GetWorld()->ServerTravel(MapURL)`
- Subscribes to `server.{InstanceId}.status` — replies with current map, player count, uptime

---

## Verification

- Server build: confirm zero TurboLink/gRPC symbols linked — `nm TPSCoreMechanicsServer | grep -i grpc` should return empty
- Client build: confirm all existing features still compile
- Run server: `./TPSCoreMechanicsServer GameWorldMap -log -NATSUrl=nats://localhost:4222 -InstanceId=srv1`
- Publish `server.srv1.map` → confirm `ServerTravel` fires
- Client connect → verify character data fetched via NATS and appearance replicates

---

## Decisions

- gRPC (TurboLink) is **client-only** — server has zero gRPC dependency
- NATS via custom nats.c plugin is the sole server↔service transport
- NATS request/reply pattern used for synchronous fetches (character data, etc.)
- Module split: `TPSCoreMechanicsClient` for all client+gRPC code, `TPSCoreMechanics` shared with no transport dependency
- Server auth: deferred, static token for now
