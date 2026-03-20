# NPC Asset + NPC Creation Guide

This guide explains the current workflow to:
1) add a new NPC visual/class asset in Unreal, and
2) create a new NPC definition that the server can spawn.

---

## Current runtime behavior (important)

- NPC metadata is fetched by the UE server from `npc-metadata-service` via NATS.
- Main read subjects are configurable in `Config/DefaultGame.ini` under:
  - `/Script/TPSCoreMechanics.TPSNatsSubjectsConfig`
- New NPC definitions are created/updated through `npc.meta.upsert` (used by WebAdmin).
- Spawn points are authoritative from metadata (one actor spawned per spawn point).

### Visual/class fields currently used

The UE client/server currently parse:
- `model_id`
- `skeletal_mesh_id` / `actor_class_id` (preferred when present)
- fallback from legacy: `skeletal_mesh_path` / `actor_class_path`

For now, the server-side metadata service/WebAdmin payload still uses `*_path` fields, so set those when authoring NPCs.

---

## Part A — Add a new NPC asset in Unreal

## 1) Create your assets in the NPC folder

Use this folder convention:
- `/Game/Characters/NPC/`

Recommended assets:
- Skeletal mesh (for example: `SKM_Blacksmith_M`)
- NPC blueprint class that derives from `ANPCCharacter` (for example: `BP_NPC_Blacksmith`)

## 2) Naming convention for actor class (required)

The server resolves actor class by **ID** into:
- `/Game/Characters/NPC/<ActorClassId>.<ActorClassId>_C`

So if your class id is `BP_NPC_Blacksmith`, make sure this asset exists:
- `/Game/Characters/NPC/BP_NPC_Blacksmith.BP_NPC_Blacksmith_C`

## 3) Keep base behavior in `ANPCCharacter`

Your BP should derive from `ANPCCharacter` so replicated NPC metadata initializes correctly (`OnNpcInitialized` event path).

---

## Part B — Create a new NPC definition (recommended: WebAdmin)

## Option 1 (recommended): WebAdmin UI

1. Open Admin NPCs page:
   - `/admin/npcs`
2. Click **New NPC**
3. Fill required fields:
   - `display_name`
   - `role`
   - `model_id`
   - `template_key`
   - at least one spawn point with `zone_id`
4. Fill visual fields (current backend contract):
   - `skeletal_mesh_path` (example: `/Game/Characters/NPC/SKM_Blacksmith_M.SKM_Blacksmith_M`)
   - `actor_class_path` (example: `/Game/Characters/NPC/BP_NPC_Blacksmith.BP_NPC_Blacksmith_C`)
5. Set spawn points (`x`,`y`,`z` in cm, `yaw` in degrees)
6. Save

## Option 2: API/NATS style payload

Equivalent payload shape for upsert (`npc.meta.upsert`):

```json
{
  "display_name": "Blacksmith Rowan",
  "role": "merchant",
  "model_id": "NPC_Blacksmith_M",
  "skeletal_mesh_path": "/Game/Characters/NPC/SKM_Blacksmith_M.SKM_Blacksmith_M",
  "actor_class_path": "/Game/Characters/NPC/BP_NPC_Blacksmith.BP_NPC_Blacksmith_C",
  "faction": "traders_guild",
  "template_key": "shopkeeper/blacksmith",
  "tone": "calm, practical, craft-focused",
  "rules": [
    "Focus on weapons and armor trade",
    "Keep answers concise and in character"
  ],
  "is_persistent": true,
  "spawn_points": [
    {
      "zone_id": "zone_tavern_district",
      "x": 1350.0,
      "y": -420.0,
      "z": 100.0,
      "yaw": 180.0,
      "spawn_policy": "always"
    }
  ],
  "behavior_config": {
    "interaction_radius": 300.0,
    "cooldown_ms": 4000,
    "max_concurrent_interactions": 1
  }
}
```

Notes:
- Include `npc_id` to update an existing NPC.
- Omit `npc_id` to create a new NPC (service generates UUID).

---

## Part C — Verify the NPC in UE

In PIE/server world (editor path), use console commands:
- `idk.npc.lookup <filter>`
- `idk.npc.spawn <npc_id>`

Expected behavior:
- Lookup prints matching NPCs.
- Spawn creates NPC near first player when `npc_id` exists.

---

## Troubleshooting

- **NPC not spawning on map load**
  - Verify spawn point `zone_id` matches resolved server zone for that map.
- **Class load warning/fallback in logs**
  - Ensure actor class asset is in `/Game/Characters/NPC/` and name matches `<ActorClassId>`.
- **NPC exists but visuals are wrong**
  - Check `skeletal_mesh_path` basename and `model_id` consistency with your client-side visual setup.
- **No NPC in lookup**
  - Confirm metadata service has the row and NATS is connected.

---

## Quick checklist

- [ ] Skeletal mesh asset added under `/Game/Characters/NPC/`
- [ ] NPC BP class derives from `ANPCCharacter`
- [ ] `actor_class_path` points to `/Game/Characters/NPC/<Name>.<Name>_C`
- [ ] NPC definition saved in WebAdmin `/admin/npcs`
- [ ] At least one spawn point with correct `zone_id`
- [ ] Verified with `idk.npc.lookup` and `idk.npc.spawn`
