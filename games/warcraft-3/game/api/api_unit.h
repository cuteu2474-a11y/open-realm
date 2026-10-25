extern player_t *currentplayer;

static uint32_t const order_ugol = BZ_WC3_UNIT_HAUNTED_GOLD_MINE;
static uint32_t const unit_ngol = BZ_WC3_UNIT_GOLD_MINE;

#define UNIT_TYPED_ACCESS(NAME, FIELD, TYPE) \
uint32_t SetUnit##NAME(jass_t *j) {  \
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");  \
    if (whichUnit) { \
        memcpy(&whichUnit->FIELD, jass_checkhandle(j, 2, #TYPE), sizeof(whichUnit->FIELD)); \
        if (whichUnit->s.flags & EF_FOW_BLOCKER) G_FowMarkBlockersDirty(); \
        gi.LinkEntity(whichUnit); \
    } \
    return 0; \
}  \
uint32_t GetUnit##NAME(jass_t *j) {  \
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");  \
    return whichUnit ? jass_pushlighthandle(j, &whichUnit->FIELD, #TYPE) : jass_pushnullhandle(j, #TYPE); \
}

#define UNIT_ACCESS(NAME, FIELD) \
uint32_t SetUnit##NAME(jass_t *j) {  \
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");  \
    if (whichUnit) { \
        whichUnit->FIELD = jass_checknumber(j, 2); \
        if (whichUnit->s.flags & EF_FOW_BLOCKER) G_FowMarkBlockersDirty(); \
    } \
    return 0; \
}  \
uint32_t GetUnit##NAME(jass_t *j) {  \
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");  \
    return jass_pushnumber(j, whichUnit ? whichUnit->FIELD : 0); \
}

#define UNITINFO_ACCESS(FIELD) UNIT_ACCESS(FIELD, unitinfo.FIELD)

#define UNIT_POSITION_ACCESS(NAME, FIELD) \
uint32_t SetUnit##NAME(jass_t *j) { \
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit"); \
    if (whichUnit) { \
        vec2_t old_position = whichUnit->s.origin2; \
        whichUnit->FIELD = jass_checknumber(j, 2); \
        if (whichUnit->s.flags & EF_FOW_BLOCKER) G_FowMarkBlockersDirty(); \
        gi.LinkEntity(whichUnit); \
        G_UnitPositionChanged(whichUnit, &old_position); \
    } \
    return 0; \
} \
uint32_t GetUnit##NAME(jass_t *j) { \
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit"); \
    return jass_pushnumber(j, whichUnit ? whichUnit->FIELD : 0); \
}

UNIT_POSITION_ACCESS(X, s.origin.x);
UNIT_POSITION_ACCESS(Y, s.origin.y);
#undef UNIT_POSITION_ACCESS

uint32_t SetUnitPositionLoc(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    vec2_t const *whichLocation = jass_checkhandle(j, 2, "location");
    vec2_t position;

    if (whichUnit && whichLocation) {
        vec2_t old_position = whichUnit->s.origin2;
        G_FindUnitUnstuckPosition(whichUnit, whichLocation, &position);
        whichUnit->s.origin.x = position.x;
        whichUnit->s.origin.y = position.y;
        if (whichUnit->s.flags & EF_FOW_BLOCKER) G_FowMarkBlockersDirty();
        gi.LinkEntity(whichUnit);
        G_UnitPositionChanged(whichUnit, &old_position);
    }
    return 0;
}
uint32_t GetUnitPositionLoc(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return whichUnit ? jass_pushlighthandle(j, &whichUnit->s.origin2, "location") : jass_pushnullhandle(j, "location");
}
UNITINFO_ACCESS(MoveSpeed);

uint32_t SetUnitFlyHeight(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    float const newHeight = jass_checknumber(j, 2);
    (void)jass_checknumber(j, 3); /* Warsmash currently ignores rate too. */
    if (whichUnit) {
        whichUnit->unitinfo.FlyHeight = newHeight;
        M_CheckGround(whichUnit);
        if (whichUnit->s.flags & EF_FOW_BLOCKER) G_FowMarkBlockersDirty();
        gi.LinkEntity(whichUnit);
    }
    return 0;
}

uint32_t GetUnitFlyHeight(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, whichUnit ? whichUnit->unitinfo.FlyHeight : 0);
}
UNITINFO_ACCESS(TurnSpeed);
UNITINFO_ACCESS(AcquireRange);

/* UnitData.uprw is authored in degrees, while the native setter/getter use
 * radians. Keep the runtime value in native units for movement and JASS. */
uint32_t SetUnitPropWindow(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    if (whichUnit) whichUnit->unitinfo.PropWindow = (float)jass_checknumber(j, 2);
    return 0;
}
uint32_t GetUnitPropWindow(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, whichUnit ? whichUnit->unitinfo.PropWindow : 0);
}

uint32_t GetUnitFacing(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    if (!whichUnit) {
        return jass_pushnumber(j, 0);
    }
    float facingAngle = whichUnit->s.angle;
    jass_pushnumber(j, RAD2DEG(facingAngle));
    return 1;
}

uint32_t SetUnitFacing(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    float facingAngle = jass_checknumber(j, 2);
    if (whichUnit) whichUnit->s.angle = DEG2RAD(facingAngle);
    return 0;
}

uint32_t SetUnitFacingTimed(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    float facingAngle = jass_checknumber(j, 2);
//    float duration = jass_checknumber(j, 3);
    if (whichUnit) whichUnit->s.angle = DEG2RAD(facingAngle);
    return 0;
}

uint32_t KillUnit(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    /* KillUnit is a death transition, not a raw life write; unit_die owns the death animation, events, and cleanup. */
    if (whichUnit && whichUnit->inuse && !(whichUnit->svflags & SVF_DEADMONSTER)) {
        unit_die(whichUnit, NULL);
    }
    return 0;
}
uint32_t RemoveUnit(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    if (whichUnit) {
        gameClient_t *owner = G_GetPlayerClientByNumber(whichUnit->s.player);
        if (owner && owner->ps.number == whichUnit->s.player) G_InvalidateCommands(owner);
        G_DeferFreeEdict(whichUnit);
    }
    return 0;
}
uint32_t ShowUnit(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    bool show = jass_checkboolean(j, 2);
    bool was_hidden;
    bool is_hidden;
    if (!whichUnit) {
        return 0;
    }
    if (show && !G_UnitIsWorldActive(whichUnit)) return 0;
    was_hidden = !!(whichUnit->s.renderfx & RF_HIDDEN);
    if (show) {
        whichUnit->s.renderfx &= ~RF_HIDDEN;
    } else {
        whichUnit->s.renderfx |= RF_HIDDEN;
    }
    is_hidden = !!(whichUnit->s.renderfx & RF_HIDDEN);
    if (was_hidden != is_hidden) {
        if (whichUnit->s.flags & EF_FOW_BLOCKER) G_FowMarkBlockersDirty();
        /* Visibility is part of both Hero-shortcut and idle-worker eligibility.
         * Rebuild only on a real transition; the shared hook cheaply rejects
         * ordinary non-Hero/non-worker units. */
        G_InvalidateUnitShortcuts(G_GetPlayerClientByNumber(whichUnit->s.player));
    }
    return 0;
}

uint32_t SetUnitState(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    UNITSTATE *whichUnitState = jass_checkhandle(j, 2, "unitstate");
    float newVal = jass_checknumber(j, 3);
    float oldLife;
    bool was_dead;
    if (!whichUnit || !whichUnitState) {
        return 0;
    }
    was_dead = M_IsDead(whichUnit);
    oldLife = whichUnit->health.value;
    if (*whichUnitState == WC3_UNIT_STATE_LIFE) {
        G_SetHealth(whichUnit, newVal);
        /* A JASS life write to zero is a death transition. G_SetHealth updates
         * the value and state-limit events; unit_die owns death events,
         * animation, cleanup, and campaign triggers that react to death. */
        if (oldLife > 0.0f && newVal <= 0.0f && !(whichUnit->svflags & SVF_DEADMONSTER) && whichUnit->die)
            whichUnit->die(whichUnit, NULL);
    }
    else (&whichUnit->health.value)[*whichUnitState] = newVal;
    if ((whichUnit->s.flags & EF_FOW_BLOCKER) && was_dead != M_IsDead(whichUnit)) G_FowMarkBlockersDirty();
    return 0;
}
//uint32_t SetUnitState(jass_t *j) {
//    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
//    UNITSTATE *whichUnitState = jass_checkhandle(j, 2, "unitstate");
//    float newVal = jass_checknumber(j, 3);
//    (&whichUnit->health.value)[*whichUnitState] = newVal;
//    return 0;
//}
uint32_t GetUnitState(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    UNITSTATE *whichUnitState = jass_checkhandle(j, 2, "unitstate");
    float value = whichUnit && whichUnitState ? (&whichUnit->health.value)[*whichUnitState] : 0;
    return jass_pushnumber(j, value);
}
uint32_t SetUnitPosition(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    vec2_t requested = MAKE(vec2_t, jass_checknumber(j, 2), jass_checknumber(j, 3));
    vec2_t position;

    if (whichUnit) {
        vec2_t old_position = whichUnit->s.origin2;
        G_FindUnitUnstuckPosition(whichUnit, &requested, &position);
        whichUnit->s.origin.x = position.x;
        whichUnit->s.origin.y = position.y;
        if (whichUnit->s.flags & EF_FOW_BLOCKER) G_FowMarkBlockersDirty();
        gi.LinkEntity(whichUnit);
        G_UnitPositionChanged(whichUnit, &old_position);
    }
    return 0;
}
uint32_t GetUnitDefaultAcquireRange(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, whichUnit ? whichUnit->unitinfo.AcquireRange : 0);
}
uint32_t GetUnitDefaultTurnSpeed(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, whichUnit ? whichUnit->unitinfo.TurnSpeed : 0);
}
uint32_t GetUnitDefaultPropWindow(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, whichUnit && whichUnit->data.UnitData
        ? whichUnit->data.UnitData->propWin : 0);
}
uint32_t GetUnitDefaultFlyHeight(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, whichUnit && whichUnit->data.UnitData
        ? whichUnit->data.UnitData->moveHeight : 0);
}
uint32_t SetUnitOwner(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    player_t const *whichPlayer = jass_checkhandle(j, 2, "player");
    bool change_color = jass_checkboolean(j, 3);
    if (whichUnit && whichPlayer) {
        uint32_t const previous_player = whichUnit->s.player;
        uint32_t const previous_color = G_GetUnitTeamColor(whichUnit);
        G_SetUnitPlayer(whichUnit, PLAYER_NUM(whichPlayer));
        if (change_color) {
            G_ClearUnitColorOverride(whichUnit);
            G_SetUnitTeamColor(whichUnit, whichPlayer->color);
        } else if (previous_player != PLAYER_NUM(whichPlayer)) {
            G_SetUnitColorOverride(whichUnit, previous_color);
        }
    }
    return 0;
}
uint32_t SetUnitColor(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t *pColor = jass_checkhandle(j, 2, "playercolor");
    if (whichUnit && pColor) G_SetUnitColorOverride(whichUnit, *pColor);
    return 0;
}
uint32_t SetUnitScale(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    float scaleX = jass_checknumber(j, 2);
    (void)jass_checknumber(j, 3);
    (void)jass_checknumber(j, 4);
    if (whichUnit) {
        /* Warsmash applies WC3's XYZ API as uniform model scale from X. */
        whichUnit->s.scale = scaleX;
        if (whichUnit->s.flags & EF_FOW_BLOCKER) G_FowMarkBlockersDirty();
    }
    return 0;
}
uint32_t SetUnitTimeScale(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    float timeScale = jass_checknumber(j, 2);

    if (whichUnit) whichUnit->animation_speed = MAX(0.0f, timeScale);
    return 0;
}
uint32_t SetUnitBlendTime(jass_t *j) {
    //edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    //float blendTime = jass_checknumber(j, 2);
    return 0;
}
/* Store the clamped persistent RGBA override consumed by the WC3 presentation datagram. */
uint32_t SetUnitVertexColor(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    int32_t red = jass_checkinteger(j, 2);
    int32_t green = jass_checkinteger(j, 3);
    int32_t blue = jass_checkinteger(j, 4);
    int32_t alpha = jass_checkinteger(j, 5);
    if (whichUnit) {
        whichUnit->vertex_color = MAKE(color32_t,
            BZ_CLAMP_U8(red), BZ_CLAMP_U8(green), BZ_CLAMP_U8(blue), BZ_CLAMP_U8(alpha));
        whichUnit->vertex_color_set = true;
        whichUnit->vertex_color_override_set = true;
    }
    return 0;
}
uint32_t SetUnitUserData(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    if (whichUnit) whichUnit->user_data = jass_checkinteger(j, 2);
    return 0;
}
uint32_t GetUnitUserData(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichUnit ? whichUnit->user_data : 0);
}
/* BZ_COMPAT_GUESS: retail does not document the return contract for null or
 * repeated requests.  For a valid unit, report success.  The getter always
 * reports the stored flag; this only mutes alarms, never retaliation/damage. */
uint32_t UnitIgnoreAlarm(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    bool ignore = jass_checkboolean(j, 2);
    if (unit && unit->inuse) unit->ignore_alarm = ignore;
    return jass_pushboolean(j, unit && unit->inuse);
}
uint32_t UnitIgnoreAlarmToggled(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, unit && unit->inuse && unit->ignore_alarm);
}
uint32_t UnitSetUsesAltIcon(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    if (whichUnit) whichUnit->uses_alt_icon = jass_checkboolean(j, 2);
    return 0;
}
uint32_t QueueUnitAnimation(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    cstring_t animation = jass_checkstring(j, 2);
    if (unit && animation) {
        strlcpy(unit->queued_animation, animation, sizeof(unit->queued_animation));
    }
    return 0;
}
uint32_t SetUnitAnimation(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    cstring_t whichAnimation = jass_checkstring(j, 2);
    if (whichUnit) G_SetUnitAnimation(whichUnit, whichAnimation);
    return 0;
}
uint32_t SetUnitAnimationByIndex(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    int32_t index = jass_checkinteger(j, 2);
    animation_t const *animation = unit ? G_GetAnimationByIndex(unit->s.model, index) : NULL;
    if (animation) {
        unit->animation = animation;
        unit->animation_index = (uint32_t)index + 1;
        strlcpy(unit->animation_request, animation->name, sizeof(unit->animation_request));
        unit->s.frame = animation->interval[0];
        unit->animation_override = true;
    }
    return 0;
}
uint32_t SetUnitAnimationWithRarity(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    cstring_t whichAnimation = jass_checkstring(j, 2);
    uint32_t *rarity = jass_checkhandle(j, 3, "raritycontrol");
    animation_t const *animation;

    if (whichUnit && whichAnimation) {
        animation = G_GetAnimationVariant(whichUnit->s.model, whichAnimation, rarity && *rarity == 1);
        G_SetUnitAnimation(whichUnit, whichAnimation);
        if (animation) {
            whichUnit->animation = animation;
            whichUnit->s.frame = animation->interval[0];
            whichUnit->animation_override = true;
        }
    }
    return 0;
}
uint32_t AddUnitAnimationProperties(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    cstring_t animProperties = jass_checkstring(j, 2);
    bool add = jass_checkboolean(j, 3);
    if (whichUnit) G_AddUnitAnimationProperties(whichUnit, animProperties, add);
    return 0;
}

//JASS_API(SetUnitLookAt,
//(EDICT, whichUnit, "unit"),
//(string, whichBone),
//(EDICT, lookAtTarget, "unit"),
//(number, offsetX),
//(number, offsetY),
//(number, offsetZ))
//{
//}
uint32_t SetUnitLookAt(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    cstring_t bone = jass_checkstring(j, 2);
    edict_t *target = jass_checkhandle(j, 3, "unit");
    vec3_t offset = {jass_checknumber(j, 4), jass_checknumber(j, 5), jass_checknumber(j, 6)};
    if (!unit || !unit->inuse) return 0;
    if (!target || !target->inuse || !isfinite(offset.x) ||
        !isfinite(offset.y) || !isfinite(offset.z)) {
        unit->look_at_target_number = 0;
        unit->s.look_at_packed = 0;
        return 0;
    }
    unit->look_at_target_number = target->s.number;
    unit->look_at_target_generation = target->spawn_time;
    unit->look_at_offset = offset;
    unit->look_at_bone_hash = WC3_LookBoneHash(bone);
    G_UpdateUnitLookAt(unit);
    return 0;
}
uint32_t ResetUnitLookAt(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    if (!unit) return 0;
    unit->look_at_target_number = 0;
    unit->look_at_target_generation = 0;
    unit->s.look_at_packed = 0;
    return 0;
}
uint32_t SetUnitRescuable(jass_t *j) {
    //edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    //handle_t byWhichPlayer = jass_checkhandle(j, 2, "player");
    //bool flag = jass_checkboolean(j, 3);
    return 0;
}
uint32_t SetUnitRescueRange(jass_t *j) {
    //edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    //float range = jass_checknumber(j, 2);
    return 0;
}
uint32_t SetHeroStr(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t newStr = jass_checkinteger(j, 2);
//    bool permanent = jass_checkboolean(j, 3);
    if (whichHero) { whichHero->hero.str = (uint32_t)MAX(0, newStr); G_RecomputeHeroStats(whichHero); }
    return 0;
}
uint32_t SetHeroAgi(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t newAgi = jass_checkinteger(j, 2);
//    bool permanent = jass_checkboolean(j, 3);
    if (whichHero) { whichHero->hero.agi = (uint32_t)MAX(0, newAgi); G_RecomputeHeroStats(whichHero); }
    return 0;
}
uint32_t SetHeroInt(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t newInt = jass_checkinteger(j, 2);
//    bool permanent = jass_checkboolean(j, 3);
    if (whichHero) { whichHero->hero.intel = (uint32_t)MAX(0, newInt); G_RecomputeHeroStats(whichHero); }
    return 0;
}
uint32_t GetHeroXP(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichHero ? (int32_t)whichHero->hero.xp : 0);
}
uint32_t SetHeroXP(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t newXpVal = jass_checkinteger(j, 2);
//    bool showEyeCandy = jass_checkboolean(j, 3);
    if (whichHero && !whichHero->hero.suspend_xp) {
        G_HeroSetXP(whichHero, (uint32_t)MAX(0, newXpVal));
    }
    return 0;
}
uint32_t GetHeroSkillPoints(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t const points = whichHero && whichHero->data.UnitBalance && G_UnitIsHero(whichHero)
        ? (int32_t)whichHero->hero.skillpoints : 0;
    return jass_pushinteger(j, points);
}
uint32_t UnitModifySkillPoints(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t const delta = jass_checkinteger(j, 2);
    bool const result = G_HeroModifySkillPoints(whichHero, delta);
    return jass_pushboolean(j, result);
}
uint32_t AddHeroXP(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t xpToAdd = jass_checkinteger(j, 2);
//    bool showEyeCandy = jass_checkboolean(j, 3);
    if (whichHero && !whichHero->hero.suspend_xp && xpToAdd > 0) {
        uint32_t add = (uint32_t)xpToAdd;
        uint32_t cur = whichHero->hero.xp;
        /* Cap at INT32_MAX so GetHeroXP (signed return) never reads negative. */
        uint32_t sum = cur + add;
        G_HeroSetXP(whichHero, (sum < cur || sum > (uint32_t)INT32_MAX) ? (uint32_t)INT32_MAX : sum);
    }
    return 0;
}
uint32_t UnitStripHeroLevel(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t const levels = jass_checkinteger(j, 2);
    return jass_pushboolean(j, levels > 0 && G_HeroStripLevels(whichHero, (uint32_t)levels));
}
uint32_t SetHeroLevelBJ(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t const newLevel = jass_checkinteger(j, 2);
    /* The Blizzard.j wrapper uses SetHeroLevel for raises and the strip
     * native for reductions. The third showEyeCandy argument applies only
     * to the raise path and is currently a presentation TODO. */
    if (!whichHero || !G_UnitIsHero(whichHero)) return 0;
    if (newLevel < (int32_t)whichHero->hero.level) {
        int64_t const remove = (int64_t)whichHero->hero.level - newLevel;
        G_HeroStripLevels(whichHero, (uint32_t)MIN(remove, (int64_t)UINT32_MAX));
    } else if (newLevel > (int32_t)whichHero->hero.level) {
        uint32_t const target = MIN((uint32_t)newLevel, G_MaxHeroLevel());
        G_HeroSetXP(whichHero, MAX(whichHero->hero.xp, G_HeroXPForLevel(target)));
    }
    return 0;
}
uint32_t SetHeroLevel(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t level = jass_checkinteger(j, 2);
//    bool showEyeCandy = jass_checkboolean(j, 3);
    if (whichHero && level > (int32_t)whichHero->hero.level) {
        /* WC3 SetHeroLevel raises the level by granting enough XP to reach it
         * (level only increases). Route through the XP transition so skill
         * points and both Hero-level event families per crossed level stay
         * identical to ordinary XP gains. */
        uint32_t const target = MIN((uint32_t)level, G_MaxHeroLevel());
        uint32_t const need = G_HeroXPForLevel(target);
        G_HeroSetXP(whichHero, MAX(whichHero->hero.xp, need));
    }
    return 0;
}
uint32_t GetHeroLevel(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichHero ? whichHero->hero.level : 0);
}
uint32_t SuspendHeroXP(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    bool flag = jass_checkboolean(j, 2);
    if (whichHero) whichHero->hero.suspend_xp = flag;
    return 0;
}
uint32_t IsSuspendedXP(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, whichHero && whichHero->hero.suspend_xp);
}
uint32_t SelectHeroSkill(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    int32_t abilcode = jass_checkinteger(j, 2);
    if (whichHero) {
        G_HeroLearnSkill(whichHero, (uint32_t)abilcode);
    }
    return 0;
}
/* Protected ORDR JASS stores integer rawcodes in written big-endian form
 * ($41393030 = "A900"), whereas decoded UnitAbilities.slk stores ordinary
 * MAKEFOURCC values. Retain canonical calls unchanged and only try a byte
 * swap for unknown abilities in the exact ORDR function family. Otherwise
 * BC6's button selection gate silently fails for every menu button. */
static uint32_t G_ORDRResolveJassAbilityCode(jass_t *j, edict_t *unit, uint32_t code) {
    uint32_t swapped;
    if (!j || !unit || !code || G_UnitAbilityLevel(unit, code) ||
        !jass_functionbyname(j, "CUO") ||
        !jass_functionbyname(j, "CUW") ||
        !jass_functionbyname(j, "CUN")) return code;
    swapped = ((code & 0x000000ffu) << 24) |
              ((code & 0x0000ff00u) << 8) |
              ((code & 0x00ff0000u) >> 8) |
              ((code & 0xff000000u) >> 24);
    return G_UnitAbilityLevel(unit, swapped) ? swapped : code;
}

uint32_t GetUnitAbilityLevel(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t const code = (uint32_t)jass_checkinteger(j, 2);
    uint32_t const resolved = G_ORDRResolveJassAbilityCode(j, whichUnit, code);
    return jass_pushinteger(j, whichUnit ? (int32_t)G_UnitAbilityLevel(whichUnit, resolved) : 0);
}
uint32_t SetUnitAbilityLevel(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t const code = (uint32_t)jass_checkinteger(j, 2);
    int32_t level = jass_checkinteger(j, 3);
    uint32_t const resolved = G_ORDRResolveJassAbilityCode(j, whichUnit, code);
    return jass_pushinteger(j, whichUnit ? (int32_t)G_UnitSetAbilityLevel(whichUnit, resolved, level) : 0);
}
uint32_t IncUnitAbilityLevel(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t const code = (uint32_t)jass_checkinteger(j, 2);
    uint32_t abilcode = G_ORDRResolveJassAbilityCode(j, whichUnit, code);
    uint32_t cur;
    if (!whichUnit) return jass_pushinteger(j, 0);
    cur = G_UnitAbilityLevel(whichUnit, abilcode);
    if (!cur) return jass_pushinteger(j, 0);
    return jass_pushinteger(j, (int32_t)G_UnitSetAbilityLevel(whichUnit, (uint32_t)abilcode, (int32_t)cur + 1));
}
uint32_t GetHeroStr(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    bool includeBonuses = jass_checkboolean(j, 2);
    (void)includeBonuses; /* TODO: attribute bonuses from items/auras are not tracked separately yet. */
    return jass_pushinteger(j, whichHero ? (int32_t)whichHero->hero.str : 0);
}
uint32_t GetHeroAgi(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    bool includeBonuses = jass_checkboolean(j, 2);
    (void)includeBonuses;
    return jass_pushinteger(j, whichHero ? (int32_t)whichHero->hero.agi : 0);
}
uint32_t GetHeroInt(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    bool includeBonuses = jass_checkboolean(j, 2);
    (void)includeBonuses;
    return jass_pushinteger(j, whichHero ? (int32_t)whichHero->hero.intel : 0);
}
uint32_t GetUnitLevel(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    if (!whichUnit) return jass_pushinteger(j, 0);
    if (G_UnitIsHero(whichUnit)) return jass_pushinteger(j, (int32_t)whichUnit->hero.level);
    return jass_pushinteger(j, whichUnit->data.UnitBalance ? whichUnit->data.UnitBalance->level : 0);
}
uint32_t GetUnitCurrentOrder(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichUnit ? (int32_t)G_GetIssuedOrderId(whichUnit) : 0);
}
uint32_t UnitInventorySize(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichUnit ? (int32_t)G_InventoryCapacity(whichUnit) : 0);
}
uint32_t UnitDropItemPoint(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *whichItem = jass_checkhandle(j, 2, "item");
    float x = jass_checknumber(j, 3);
    float y = jass_checknumber(j, 4);
    int32_t slot;
    if (!whichUnit || !G_IsItem(whichItem) || whichItem->item->carrier != whichUnit)
        return jass_pushboolean(j, 0);
    slot = whichItem->item->inventory_slot;
    if (slot < 0) return jass_pushboolean(j, 0);
    return jass_pushboolean(j, G_DropItemAtScripted(whichUnit, (uint32_t)slot, &MAKE(vec2_t, x, y)));
}
uint32_t UnitDropItemTarget(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *whichItem = jass_checkhandle(j, 2, "item");
    edict_t *target = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, G_OrderGiveItemTarget(whichUnit, whichItem, target));
}
uint32_t UnitDamageTarget(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *target = jass_checkhandle(j, 2, "widget");
    float amount = jass_checknumber(j, 3);
    /* attack/ranged/attackType/damageType/weaponType are accepted but not yet modeled. */
    (void)jass_checkboolean(j, 4);
    (void)jass_checkboolean(j, 5);
    (void)jass_checkhandle(j, 6, "attacktype");
    (void)jass_checkhandle(j, 7, "damagetype");
    (void)jass_checkhandle(j, 8, "weapontype");
    if (!whichUnit || !target || amount <= 0.0f) return jass_pushboolean(j, 0);
    if (target->invulnerable || M_IsDead(target)) return jass_pushboolean(j, 0);
    T_Damage(target, whichUnit, (int)amount);
    return jass_pushboolean(j, 1);
}

/* The JASS attacktype enum does not have the internal ATK_NONE slot. Damage
 * type NORMAL uses attack/armor mitigation, while UNIVERSAL bypasses it.
 * Other damage-type immunities and attack/ranged event flags remain separate
 * compatibility work (UnitDamageTarget does not model them yet either). */
static uint32_t jass_point_attack_type(uint32_t type) {
    static uint32_t const types[] = {
        ATK_NORMAL, ATK_NORMAL, ATK_PIERCE, ATK_SIEGE, ATK_MAGIC, ATK_CHAOS, ATK_HERO
    };
    return type < sizeof(types) / sizeof(types[0]) ? types[type] : ATK_SPELLS;
}

static bool jass_point_damage_apply(edict_t *attacker, vec2_t const *point, float radius,
                                    float amount, uint32_t attack_type, uint32_t damage_type) {
    bool hit = false;
    if (!attacker || !attacker->inuse || !point) return false;
    /* Do not filter by alliance. JASS point damage selects units in range,
     * including allies; map script authors explicitly filter when desired. */
    FOR_LOOP(i, globals.num_edicts) {
        edict_t *target = &globals.edicts[i];
        int damage = (int)amount;
        if (!target->inuse || !(target->svflags & SVF_MONSTER) || G_IsDeferredFree(target) ||
            target->invulnerable || M_IsDead(target) ||
            Vector2_distance(&target->s.origin2, point) > radius) continue;
        if (damage_type == 4) damage = G_AttackDamageWithType(attacker, target, damage,
                                                              jass_point_attack_type(attack_type));
        if (damage <= 0) continue;
        T_Damage(target, attacker, damage);
        hit = true;
    }
    return hit;
}

/* The game scheduler drives deferred point damage through an ordinary hidden
 * edict. All payload fields are existing persisted scalar edict fields:
 * wait=radius, velocity=amount, damage=JASS attacktype,
 * projectile_attack_type=JASS damagetype, resources=source spawn generation.
 * Owner generation must be checked because edict slots are reusable. */
void JassUnitDamagePointThink(edict_t *thinker) {
    edict_t *attacker;
    if (!thinker || !thinker->inuse || G_Time() < thinker->freetime) return;
    attacker = thinker->owner;
    if (attacker && attacker->inuse && attacker->spawn_time == thinker->resources)
        jass_point_damage_apply(attacker, &thinker->s.origin2, thinker->wait, thinker->velocity,
                                thinker->damage, thinker->projectile_attack_type);
    G_FreeEdict(thinker);
}

uint32_t UnitDamagePoint(jass_t *j) {
    edict_t *attacker = jass_checkhandle(j, 1, "unit");
    float const delay = jass_checknumber(j, 2);
    float const radius = jass_checknumber(j, 3);
    float const x = jass_checknumber(j, 4);
    float const y = jass_checknumber(j, 5);
    float const amount = jass_checknumber(j, 6);
    uint32_t *attack_type;
    uint32_t *damage_type;
    edict_t *thinker;
    (void)jass_checkboolean(j, 7);
    (void)jass_checkboolean(j, 8);
    attack_type = jass_checkhandle(j, 9, "attacktype");
    damage_type = jass_checkhandle(j, 10, "damagetype");
    (void)jass_checkhandle(j, 11, "weapontype");
    if (!attacker || !attacker->inuse || !isfinite(delay) || !isfinite(radius) ||
        !isfinite(x) || !isfinite(y) || !isfinite(amount) || radius < 0.0f || amount <= 0.0f)
        return jass_pushboolean(j, false);
    if (delay <= 0.0f)
        return jass_pushboolean(j, jass_point_damage_apply(attacker, &MAKE(vec2_t, x, y), radius,
                                                           amount, attack_type ? *attack_type : 0,
                                                           damage_type ? *damage_type : 26));
    thinker = G_Spawn();
    if (!thinker) return jass_pushboolean(j, false);
    thinker->owner = attacker;
    thinker->resources = attacker->spawn_time;
    thinker->s.origin2 = MAKE(vec2_t, x, y);
    thinker->s.player = attacker->s.player;
    thinker->wait = radius;
    thinker->velocity = amount;
    thinker->damage = attack_type ? *attack_type : 0;
    thinker->projectile_attack_type = damage_type ? *damage_type : 26;
    thinker->freetime = G_Time() + (uint32_t)(delay * 1000.0f);
    thinker->think = JassUnitDamagePointThink;
    return jass_pushboolean(j, true);
}
uint32_t UnitAddType(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t *whichUnitType = jass_checkhandle(j, 2, "unittype");
    if (!whichUnit || !whichUnitType || *whichUnitType >= 32) return jass_pushboolean(j, 0);
    whichUnit->script_unit_types |= (1u << *whichUnitType);
    return jass_pushboolean(j, 1);
}
uint32_t UnitRemoveType(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t *whichUnitType = jass_checkhandle(j, 2, "unittype");
    if (!whichUnit || !whichUnitType || *whichUnitType >= 32) return jass_pushboolean(j, 0);
    whichUnit->script_unit_types &= ~(1u << *whichUnitType);
    return jass_pushboolean(j, 1);
}
/* Keep removal and count queries consistent with the existing status model.
 * Unknown polarity is matched only by an explicit broad (+/-) request. */
static bool WC3_BuffMatchesFilter(heroabilitystatus_t const *status,
    bool positive, bool negative, bool magic, bool physical,
    bool timed_life, bool aura, bool auto_dispel) {
    uint32_t polarity, school;
    if (!status || !status->level) return false;
    if (status->code == MAKEFOURCC('B','T','L','F')) return timed_life;
    polarity = status->buff_flags & (WC3_STATUS_BUFF_POSITIVE | WC3_STATUS_BUFF_NEGATIVE);
    school = status->buff_flags & (WC3_STATUS_BUFF_MAGICAL | WC3_STATUS_BUFF_PHYSICAL);
    if (!positive && !negative) return false;
    if (polarity == WC3_STATUS_BUFF_POSITIVE && !positive) return false;
    if (polarity == WC3_STATUS_BUFF_NEGATIVE && !negative) return false;
    if (!polarity && !(positive && negative)) return false;
    if (school == WC3_STATUS_BUFF_MAGICAL && !magic) return false;
    if (school == WC3_STATUS_BUFF_PHYSICAL && !physical) return false;
    if (!magic && !physical && !aura && !auto_dispel) return false;
    return true;
}
uint32_t UnitCountBuffsEx(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    bool const positive = jass_checkboolean(j, 2), negative = jass_checkboolean(j, 3);
    bool const magic = jass_checkboolean(j, 4), physical = jass_checkboolean(j, 5);
    bool const timed_life = jass_checkboolean(j, 6), aura = jass_checkboolean(j, 7);
    bool const auto_dispel = jass_checkboolean(j, 8);
    int32_t count = 0;
    if (unit) FOR_LOOP(i, MAX_UNIT_STATUSES)
        if (WC3_BuffMatchesFilter(&unit->abilstatus[i], positive, negative, magic,
                                  physical, timed_life, aura, auto_dispel)) count++;
    return jass_pushinteger(j, count);
}
uint32_t UnitPauseTimedLife(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    bool flag = jass_checkboolean(j, 2);
    uint32_t now;
    if (!whichUnit || whichUnit->timed_life_paused == flag) return 0;
    now = G_Time();
    FOR_LOOP(i, MAX_UNIT_STATUSES) {
        heroabilitystatus_t *s = whichUnit->abilstatus + i;
        if (!s->level || s->code != MAKEFOURCC('B', 'T', 'L', 'F')) continue;
        if (flag) {
            s->data = s->timestamp > now ? s->timestamp - now : 0;
            s->timestamp = 0;
        } else if (s->data) {
            s->timestamp = now + s->data;
            s->data = 0;
        }
    }
    whichUnit->timed_life_paused = flag;
    return 0;
}
uint32_t UnitResetCooldown(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    S_SpellResetCooldowns(whichUnit);
    return 0;
}

uint32_t BlzGetUnitAbilityCooldownRemaining(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t const abilityId = (uint32_t)jass_checkinteger(j, 2);
    return jass_pushnumber(j, S_SpellCooldownRemaining(whichUnit, abilityId));
}

uint32_t BlzEndUnitAbilityCooldown(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t const abilityId = (uint32_t)jass_checkinteger(j, 2);
    S_SpellEndCooldown(whichUnit, abilityId);
    return 0;
}

uint32_t BlzStartUnitAbilityCooldown(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t const abilityId = (uint32_t)jass_checkinteger(j, 2);
    float const cooldown = jass_checknumber(j, 3);
    S_SpellStartCooldownDuration(whichUnit, abilityId, cooldown);
    return 0;
}

uint32_t ReviveHero(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    float x = jass_checknumber(j, 2);
    float y = jass_checknumber(j, 3);
    //bool doEyecandy = jass_checkboolean(j, 4);
    return jass_pushboolean(j, G_ReviveHero(whichHero, x, y));
}
uint32_t ReviveHeroLoc(jass_t *j) {
    edict_t *whichHero = jass_checkhandle(j, 1, "unit");
    vec2_t const *location = jass_checkhandle(j, 2, "location");
    //bool doEyecandy = jass_checkboolean(j, 3);
    return jass_pushboolean(j, whichHero && location &&
        G_ReviveHero(whichHero, location->x, location->y));
}
uint32_t SetUnitExploded(jass_t *j) {
    //edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    //bool exploded = jass_checkboolean(j, 2);
    return 0;
}
uint32_t SetUnitInvulnerable(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    bool flag = jass_checkboolean(j, 2);
    if (whichUnit) whichUnit->invulnerable = flag;
    return 0;
}
uint32_t PauseUnit(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    bool flag = jass_checkboolean(j, 2);
    if (whichUnit) whichUnit->paused = flag;
    return 0;
}
uint32_t IsUnitPaused(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, whichUnit && whichUnit->paused);
}
uint32_t SetUnitPathing(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    bool flag = jass_checkboolean(j, 2);
    if (whichUnit) whichUnit->no_pathing = !flag;
    return 0;
}
/* WC3 object editor point values are authored in UnitData (upoi). */
uint32_t GetUnitPointValue(jass_t *j) {
    edict_t *unit = jass_checkhandle(j,1,"unit");
    UnitData_t const *data = unit ? (unit->data.UnitData ?
        unit->data.UnitData : G_UnitData(unit->class_id)) : NULL;
    return jass_pushinteger(j,data ? (int32_t)data->points : 0);
}
uint32_t GetUnitPointValueByType(jass_t *j) {
    uint32_t rawcode = (uint32_t)jass_checkinteger(j,1);
    UnitData_t const *data = G_UnitData(rawcode);
    return jass_pushinteger(j,data ? (int32_t)data->points : 0);
}
uint32_t UnitAddItem(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *whichItem = jass_checkhandle(j, 2, "item");
    if (!whichUnit || !whichItem) return jass_pushboolean(j, false);
    if (G_ItemAbilityScriptedReattach(whichUnit, whichItem)) return jass_pushboolean(j, true);
    if (G_IsItem(whichItem) && !whichItem->item->in_world)
        return jass_pushboolean(j, G_TransferCarriedItem(whichUnit, whichItem));
    return jass_pushboolean(j, G_PickupItem(whichUnit, whichItem));
}
uint32_t UnitAddItemById(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    int32_t itemId = jass_checkinteger(j, 2);
    if (!whichUnit) return jass_pushnullhandle(j, "item");
    edict_t *item = SP_SpawnAtLocation(itemId, whichUnit->s.player, &whichUnit->s.origin2);
    if (item && G_PickupItem(whichUnit, item)) {
        return jass_pushlighthandle(j, item, "item");
    } else {
        if (item) G_RemoveItem(item);
        return jass_pushnullhandle(j, "item");
    }
}
uint32_t UnitAddItemToSlotById(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    int32_t itemId = jass_checkinteger(j, 2);
    int32_t itemSlot = jass_checkinteger(j, 3);
    if (!whichUnit || itemSlot < 0 || (uint32_t)itemSlot >= G_InventoryCapacity(whichUnit)) {
        return jass_pushboolean(j, false);
    }
    edict_t *item = SP_SpawnAtLocation(itemId, whichUnit->s.player, &whichUnit->s.origin2);
    if (item && G_AddItemToSlot(whichUnit, item, (uint32_t)itemSlot)) {
        return jass_pushboolean(j, true);
    } else {
        if (item) G_RemoveItem(item);
        return jass_pushboolean(j, false);
    }
}
uint32_t UnitRemoveItem(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *whichItem = jass_checkhandle(j, 2, "item");
    if (!whichUnit || !whichItem) {
        return 0;
    }
    FOR_LOOP(i, MAX_INVENTORY) {
        if (whichUnit->inventory[i] == whichItem) {
            if (!G_ItemAbilityScriptedRemove(whichUnit, whichItem))
                G_DropItemAtScripted(whichUnit, i, &whichUnit->s.origin2);
            break;
        }
    }
    return 0;
}
uint32_t UnitRemoveItemFromSlot(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    int32_t itemSlot = jass_checkinteger(j, 2);
    if (!whichUnit || itemSlot < 0 || (uint32_t)itemSlot >= G_InventoryCapacity(whichUnit)) {
        return jass_pushnullhandle(j, "item");
    }
    edict_t *item = whichUnit->inventory[itemSlot];
    if (!item) return jass_pushnullhandle(j, "item");
    if (!G_ItemAbilityScriptedRemove(whichUnit, item) &&
        !G_DropItemAtScripted(whichUnit, (uint32_t)itemSlot, &whichUnit->s.origin2)) {
        return jass_pushnullhandle(j, "item");
    }
    return jass_pushlighthandle(j, item, "item");
}
uint32_t UnitHasItem(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *whichItem = jass_checkhandle(j, 2, "item");
    if (!whichUnit || !whichItem) return jass_pushboolean(j, 0);
    FOR_LOOP(i, MAX_INVENTORY) {
        if (whichUnit->inventory[i] == whichItem) return jass_pushboolean(j, 1);
    }
    return jass_pushboolean(j, 0);
}
uint32_t UnitItemInSlot(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    int32_t itemSlot = jass_checkinteger(j, 2);
    if (!whichUnit || itemSlot < 0 || (uint32_t)itemSlot >= G_InventoryCapacity(whichUnit)) {
        return jass_pushnullhandle(j, "item");
    }
    edict_t *item = whichUnit->inventory[itemSlot];
    if (!item) return jass_pushnullhandle(j, "item");
    return jass_pushlighthandle(j, item, "item");
}
uint32_t UnitUseItem(jass_t *j) {
    //edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    //handle_t whichItem = jass_checkhandle(j, 2, "item");
    return jass_pushboolean(j, 0);
}
uint32_t UnitUseItemPoint(jass_t *j) {
    //edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    //handle_t whichItem = jass_checkhandle(j, 2, "item");
    //float x = jass_checknumber(j, 3);
    //float y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
uint32_t UnitUseItemTarget(jass_t *j) {
    //edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    //handle_t whichItem = jass_checkhandle(j, 2, "item");
    //handle_t target = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, 0);
}
uint32_t GetUnitRallyPoint(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    API_ALLOC(vec2_t, location);
    if (whichUnit) {
        G_ResolveRallyTarget(whichUnit, location, NULL);
    }
    return 1;
}
uint32_t GetUnitRallyUnit(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *target = NULL;
    rallyTargetType_t type;

    if (!whichUnit) return jass_pushnullhandle(j, "unit");
    type = G_ResolveRallyTarget(whichUnit, NULL, &target);
    if ((type == RALLY_TARGET_SELF || type == RALLY_TARGET_ENTITY) &&
        target && (target->svflags & SVF_MONSTER)) {
        return jass_pushlighthandle(j, target, "unit");
    }
    return jass_pushnullhandle(j, "unit");
}
uint32_t GetUnitRallyDestructable(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *target = NULL;
    rallyTargetType_t type;

    if (!whichUnit) return jass_pushnullhandle(j, "destructable");
    type = G_ResolveRallyTarget(whichUnit, NULL, &target);
    if (type == RALLY_TARGET_ENTITY && target && G_IsDestructable(target)) {
        return jass_pushlighthandle(j, target, "destructable");
    }
    return jass_pushnullhandle(j, "destructable");
}
uint32_t GetUnitLoc(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    API_ALLOC(vec2_t, location);
    if (whichUnit) {
        *location = whichUnit->s.origin2;
    }
    return 1;
}
uint32_t GetUnitDefaultMoveSpeed(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, whichUnit ? whichUnit->unitinfo.MoveSpeed : 0);
}
uint32_t GetOwningPlayer(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    if (!whichUnit) {
        return jass_pushnullhandle(j, "player");
    }
    return jass_pushlighthandle(j, G_GetPlayerByNumber(whichUnit->s.player), "player");
}
uint32_t GetUnitTypeId(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichUnit ? (int32_t)whichUnit->class_id : 0);
}
static uint32_t JassPushRaceHandle(jass_t *j, int32_t value);
uint32_t GetUnitRace(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    int32_t race = whichUnit && whichUnit->data.UnitData
        ? WC3_JassRaceFromString(whichUnit->data.UnitData->race) : 0;
    return JassPushRaceHandle(j, race);
}
uint32_t GetUnitName(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    cstring_t name = whichUnit ? G_UnitName(whichUnit->class_id) : NULL;
    return jass_pushstring(j, name ? name : "");
}
uint32_t GetUnitFoodUsed(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichUnit ? whichUnit->data.UnitBalance->foodUsed : 0);
}
uint32_t GetUnitFoodMade(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichUnit ? whichUnit->data.UnitBalance->foodMade : 0);
}
uint32_t IsUnitInGroup(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    ggroup_t *whichGroup = jass_checkhandle(j, 2, "group");
    if (!whichUnit || !whichGroup) return jass_pushboolean(j, 0);
    FOR_LOOP(i, whichGroup->num_units) {
        if (whichGroup->units[i] == whichUnit) return jass_pushboolean(j, 1);
    }
    return jass_pushboolean(j, 0);
}
uint32_t IsUnitInForce(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t *whichForce = jass_checkhandle(j, 2, "force");
    if (!whichUnit || !whichForce) return jass_pushboolean(j, 0);
    return jass_pushboolean(j, (*whichForce) & (1 << whichUnit->s.player));
}
uint32_t IsUnitOwnedByPlayer(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    player_t *whichPlayer = jass_checkhandle(j, 2, "player");
    if (!whichUnit || !whichPlayer) return jass_pushboolean(j, 0);
    return jass_pushboolean(j, whichUnit->s.player == PLAYER_NUM(whichPlayer));
}
uint32_t IsUnitAlly(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    player_t *whichPlayer = jass_checkhandle(j, 2, "player");
    if (!whichUnit || !whichPlayer) return jass_pushboolean(j, 0);
    return jass_pushboolean(j,
        G_PlayerTreatsPlayerAsAlly(PLAYER_NUM(whichPlayer), whichUnit->s.player));
}
uint32_t IsUnitEnemy(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    player_t *whichPlayer = jass_checkhandle(j, 2, "player");
    if (!whichUnit || !whichPlayer) return jass_pushboolean(j, 0);
    return jass_pushboolean(j,
        !G_PlayerTreatsPlayerAsAlly(PLAYER_NUM(whichPlayer), whichUnit->s.player));
}
uint32_t IsUnitVisible(jass_t *j) {
    edict_t *unit = jass_checkhandle(j,1,"unit");
    player_t *viewer = jass_checkhandle(j,2,"player");
    return jass_pushboolean(j,unit && unit->inuse && viewer &&
        G_FowPlayerCanHoverEntity(PLAYER_NUM(viewer),unit) &&
        !(unit->s.renderfx & RF_HIDDEN));
}
uint32_t IsUnitDetected(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    player_t *whichPlayer = jass_checkhandle(j, 2, "player");
    if (!whichUnit || !whichPlayer) return jass_pushboolean(j, 0);
    return jass_pushboolean(j, S_UnitIsDetectedByPlayer(whichUnit, PLAYER_NUM(whichPlayer)));
}
uint32_t IsUnitInvisible(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    player_t *whichPlayer = jass_checkhandle(j, 2, "player");
    if (!whichUnit || !whichPlayer) return jass_pushboolean(j, 0);
    return jass_pushboolean(j, S_UnitIsInvisibleToPlayer(whichUnit, PLAYER_NUM(whichPlayer)));
}
uint32_t IsUnitFogged(jass_t *j) {
    edict_t *unit = jass_checkhandle(j,1,"unit");
    player_t *viewer = jass_checkhandle(j,2,"player");
    return jass_pushboolean(j,unit && unit->inuse && viewer &&
        G_FowPointState(PLAYER_NUM(viewer),unit->s.origin.x,unit->s.origin.y)
            == WC3_FOG_STATE_FOGGED);
}
uint32_t IsUnitMasked(jass_t *j) {
    edict_t *unit = jass_checkhandle(j,1,"unit");
    player_t *viewer = jass_checkhandle(j,2,"player");
    return jass_pushboolean(j,unit && unit->inuse && viewer &&
        G_FowPointState(PLAYER_NUM(viewer),unit->s.origin.x,unit->s.origin.y)
            == WC3_FOG_STATE_MASKED);
}
uint32_t IsUnitSelected(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    player_t *whichPlayer = jass_checkhandle(j, 2, "player");
    if (!whichUnit || !whichPlayer) return jass_pushboolean(j, 0);
    return jass_pushboolean(j, (whichUnit->selected >> PLAYER_NUM(whichPlayer)) & 1);
}
uint32_t IsUnitRace(jass_t *j) {
    edict_t *unit = jass_checkhandle(j,1,"unit");
    uint32_t *race = jass_checkhandle(j,2,"race");
    UnitData_t const *data = unit ? (unit->data.UnitData ?
        unit->data.UnitData : G_UnitData(unit->class_id)) : NULL;
    return jass_pushboolean(j,data && race &&
        WC3_JassRaceFromString(data->race) == (int32_t)*race);
}
uint32_t IsUnitType(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t *whichUnitType = jass_checkhandle(j, 2, "unittype");
    if (!whichUnit || !whichUnitType) return jass_pushboolean(j, 0);
    if (*whichUnitType == WC3_UNIT_TYPE_STRUCTURE)
        return jass_pushboolean(j, G_UnitIsStructure(whichUnit));
    if (*whichUnitType < 32 && (whichUnit->script_unit_types & (1u << *whichUnitType)))
        return jass_pushboolean(j, 1);
    if (*whichUnitType == WC3_UNIT_TYPE_GROUND)
        return jass_pushboolean(j, !(whichUnit->aiflags & AI_FLYING));
    if (*whichUnitType == 0) /* UNIT_TYPE_HERO */
        return jass_pushboolean(j, G_UnitIsHero(whichUnit));
    if (*whichUnitType == 1) /* UNIT_TYPE_DEAD */
        return jass_pushboolean(j, M_IsDead(whichUnit));
    if (*whichUnitType == WC3_UNIT_TYPE_POLYMORPHED)
        return jass_pushboolean(j, S_UnitPolymorphed(whichUnit));
    if (*whichUnitType == 23) /* UNIT_TYPE_SLEEPING */
        return jass_pushboolean(j, G_UnitIsSleeping(whichUnit));
    if (*whichUnitType == 11) /* UNIT_TYPE_STUNNED */
        return jass_pushboolean(j, whichUnit->stunned);
    if (*whichUnitType == 3) /* UNIT_TYPE_FLYING */
        return jass_pushboolean(j, whichUnit->aiflags & AI_FLYING);
    if (*whichUnitType == 10) /* UNIT_TYPE_SUMMONED */
        return jass_pushboolean(j, whichUnit->summon_ability != 0);
    if (*whichUnitType == 14) /* UNIT_TYPE_UNDEAD */
        return jass_pushboolean(j, whichUnit->data.UnitData &&
            WC3_RaceFromString(whichUnit->data.UnitData->race) == RACE_UNDEAD);
    return jass_pushboolean(j, 0);
}
uint32_t IsUnit(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *whichSpecifiedUnit = jass_checkhandle(j, 2, "unit");
    return jass_pushboolean(j, whichUnit != NULL && whichUnit == whichSpecifiedUnit);
}
uint32_t IsUnitInRange(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *otherUnit = jass_checkhandle(j, 2, "unit");
    float distance = jass_checknumber(j, 3);
    if (!whichUnit || !otherUnit) return jass_pushboolean(j, 0);
    return jass_pushboolean(j, Vector2_distance(&whichUnit->s.origin2, &otherUnit->s.origin2) <= distance);
}
uint32_t IsUnitInRangeXY(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    float x = jass_checknumber(j, 2);
    float y = jass_checknumber(j, 3);
    float distance = jass_checknumber(j, 4);
    if (!whichUnit) return jass_pushboolean(j, 0);
    return jass_pushboolean(j, Vector2_distance(&whichUnit->s.origin2, &MAKE(vec2_t, x, y)) <= distance);
}
uint32_t IsUnitInRangeLoc(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    vec2_t const *whichLocation = jass_checkhandle(j, 2, "location");
    float distance = jass_checknumber(j, 3);
    if (!whichUnit || !whichLocation) return jass_pushboolean(j, 0);
    return jass_pushboolean(j, Vector2_distance(&whichUnit->s.origin2, whichLocation) <= distance);
}
uint32_t IsUnitHidden(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, whichUnit && (whichUnit->s.renderfx & RF_HIDDEN));
}
uint32_t IsUnitIllusion(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, whichUnit && (whichUnit->aiflags & AI_ILLUSION));
}
uint32_t IsUnitInTransport(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    edict_t *whichTransport = jass_checkhandle(j, 2, "unit");
    return jass_pushboolean(j, whichUnit && whichTransport &&
                              S_CargoTransportForUnit(whichUnit) == whichTransport);
}
uint32_t IsUnitLoaded(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, whichUnit && S_CargoTransportForUnit(whichUnit) != NULL);
}
uint32_t IsHeroUnitId(jass_t *j) {
    //int32_t unitId = jass_checkinteger(j, 1);
    return jass_pushboolean(j, 0);
}
uint32_t UnitShareVision(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    player_t *whichPlayer = jass_checkhandle(j, 2, "player");
    bool share = jass_checkboolean(j, 3);
    if (whichUnit && whichPlayer)
        G_SetUnitSharedVision(whichUnit, PLAYER_NUM(whichPlayer), share);
    return 0;
}
uint32_t UnitSuspendDecay(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    bool suspend = jass_checkboolean(j, 2);
    if (!unit || !unit->inuse) return 0;
    if (suspend) unit->aiflags |= AI_CORPSE_DECAY_SUSPENDED;
    else unit->aiflags &= ~AI_CORPSE_DECAY_SUSPENDED;
    return 0;
}
uint32_t UnitAddAbility(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t abilityId = jass_checkinteger(j, 2);
    return jass_pushboolean(j, G_ActorAddSkill(whichUnit, abilityId));
}
uint32_t UnitRemoveAbility(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t abilityId = jass_checkinteger(j, 2);
    return jass_pushboolean(j, G_ActorRemoveSkill(whichUnit, abilityId));
}
uint32_t UnitMakeAbilityPermanent(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    bool permanent = jass_checkboolean(j, 2);
    uint32_t abilityId = jass_checkinteger(j, 3);
    return jass_pushboolean(j, G_ActorSetSkillPermanent(whichUnit, abilityId, permanent));
}
static void WC3_RemoveBuffs(edict_t *unit, bool positive, bool negative,
        bool magic, bool physical, bool timed_life, bool aura, bool auto_dispel) {
    bool changed = false;
    if (!unit || !unit->inuse) return;
    FOR_LOOP(i, MAX_UNIT_STATUSES) {
        heroabilitystatus_t *status = &unit->abilstatus[i];
        if (!WC3_BuffMatchesFilter(status, positive, negative, magic,
                                   physical, timed_life, aura, auto_dispel)) continue;
        if (!unit_status_can_dispel(status) &&
            status->code != MAKEFOURCC('B','T','L','F')) continue;
        unit_removestatus_deferred(unit, status, STATUS_REMOVE_DISPEL);
        changed = true;
    }
    if (changed) {
        unit_refreshstatusflags(unit);
        G_InvalidateUnitInfoPanel(unit);
    }
}
uint32_t UnitRemoveBuffs(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    bool const positive = jass_checkboolean(j, 2), negative = jass_checkboolean(j, 3);
    WC3_RemoveBuffs(unit, positive, negative, true, true, false, false, false);
    return 0;
}
uint32_t UnitRemoveBuffsEx(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    bool const positive = jass_checkboolean(j, 2), negative = jass_checkboolean(j, 3);
    bool const magic = jass_checkboolean(j, 4), physical = jass_checkboolean(j, 5);
    bool const timed_life = jass_checkboolean(j, 6), aura = jass_checkboolean(j, 7);
    bool const auto_dispel = jass_checkboolean(j, 8);
    WC3_RemoveBuffs(unit, positive, negative, magic, physical,
                    timed_life, aura, auto_dispel);
    return 0;
}
uint32_t UnitAddSleep(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    bool add = jass_checkboolean(j, 2);
    G_UnitSetCanSleep(whichUnit, add);
    return 0;
}
uint32_t UnitCanSleep(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, G_UnitCanSleep(whichUnit));
}
uint32_t UnitAddSleepPerm(jass_t *j) {
    /* Sleep Always (Asla) has separate ability-owned semantics (including its
     * Sleep Once/Allow On Any Player Slot data). Keep this native conservative
     * until that ability is represented instead of aliasing it to night sleep. */
    (void)j;
    return 0;
}
uint32_t UnitCanSleepPerm(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, whichUnit && G_ActorHasSkill(whichUnit, "Asla"));
}
uint32_t UnitIsSleeping(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, G_UnitIsSleeping(whichUnit));
}
uint32_t UnitWakeUp(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    G_UnitWakeUp(whichUnit);
    return 0;
}
uint32_t UnitApplyTimedLife(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    uint32_t buff_id = (uint32_t)jass_checkinteger(j, 2);
    float duration = jass_checknumber(j, 3);
    if (!unit || !unit->inuse || duration <= 0.0f) return 0;
    /* OpenRealm already expires the BTLF lifecycle buff in its unit status
     * update. Reuse it instead of creating an unrelated timer. The supplied
     * buff rawcode labels the effect; BTLF owns the timed death lifecycle. */
    (void)buff_id;
    unit_addtimedstatus(unit, "BTLF", 1, duration);
    return 0;
}
uint32_t IssueImmediateOrder(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    cstring_t order = jass_checkstring(j, 2);
    return jass_pushboolean(j, unit_issueimmediateorder(whichUnit, order));
}
uint32_t IssueImmediateOrderById(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t order = (uint32_t)jass_checkinteger(j, 2);
    return jass_pushboolean(j, unit_issueimmediateorder(whichUnit, G_OrderId2String(order)));
}
uint32_t IssuePointOrder(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    cstring_t order = jass_checkstring(j, 2);
    float x = jass_checknumber(j, 3);
    float y = jass_checknumber(j, 4);
    bool ret = unit_issueorder(whichUnit, order, &MAKE(vec2_t, x, y));
    if (level.vm && jass_functionbyname(j, "CUN")) {
        static unsigned traced;
        if (traced++ < 40) {
            char detail[256];
            snprintf(detail, sizeof(detail),
                     "unit=%u raw=0x%08x owner=%u order=%.32s accepted=%u at=(%.0f,%.0f) hp=%.0f/%.0f ai=0x%x speed=%.0f",
                     (unsigned)(whichUnit ? whichUnit->s.number : 0u),
                     (unsigned)(whichUnit ? whichUnit->class_id : 0u),
                     (unsigned)(whichUnit ? whichUnit->s.player : 0u),
                     order ? order : "", (unsigned)ret, (double)x, (double)y,
                     (double)(whichUnit ? whichUnit->health.value : 0.0f),
                     (double)(whichUnit ? whichUnit->health.max_value : 0.0f),
                     (unsigned)(whichUnit ? whichUnit->aiflags : 0u),
                     (double)(whichUnit ? whichUnit->unitinfo.MoveSpeed : 0.0f));
            jass_ordr_log_event(j,"ORDR_POINT_ORDER",detail);
        }
    }
    return jass_pushboolean(j, ret);
}
uint32_t IssuePointOrderLoc(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    cstring_t order = jass_checkstring(j, 2);
    vec2_t const *whichLocation = jass_checkhandle(j, 3, "location");
    bool ret = unit_issueorder(whichUnit, order, whichLocation);
    return jass_pushboolean(j, ret);
}
uint32_t IssuePointOrderById(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t order = (uint32_t)jass_checkinteger(j, 2);
    vec2_t point = { jass_checknumber(j, 3), jass_checknumber(j, 4) };

    /* Building rawcodes are valid point-order ids, but they are not entries in
     * the canonical Warcraft order table. Keep construction on the existing
     * authoritative build path and leave ordinary ids to the generic router. */
    if (G_UnitIsBuilding(order))
        return jass_pushboolean(j, G_IssueBuildOrder(whichUnit, order, &point));
    bool accepted = unit_issueorder(whichUnit, G_OrderId2String(order), &point);
    if (level.vm && jass_functionbyname(j, "CUN")) {
        static unsigned traced;
        if (traced++ < 40) {
            char detail[256];
            snprintf(detail, sizeof(detail),
                     "unit=%u raw=0x%08x owner=%u order_id=%u accepted=%u hp=%.0f/%.0f ai=0x%x speed=%.0f",
                     (unsigned)(whichUnit ? whichUnit->s.number : 0u),
                     (unsigned)(whichUnit ? whichUnit->class_id : 0u),
                     (unsigned)(whichUnit ? whichUnit->s.player : 0u),
                     (unsigned)order, (unsigned)accepted,
                     (double)(whichUnit ? whichUnit->health.value : 0.0f),
                     (double)(whichUnit ? whichUnit->health.max_value : 0.0f),
                     (unsigned)(whichUnit ? whichUnit->aiflags : 0u),
                     (double)(whichUnit ? whichUnit->unitinfo.MoveSpeed : 0.0f));
            jass_ordr_log_event(j,"ORDR_POINT_ORDER",detail);
        }
    }
    return jass_pushboolean(j, accepted);
}
uint32_t IssuePointOrderByIdLoc(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t order = (uint32_t)jass_checkinteger(j, 2);
    vec2_t const *whichLocation = jass_checkhandle(j, 3, "location");

    if (G_UnitIsBuilding(order))
        return jass_pushboolean(j, G_IssueBuildOrder(whichUnit, order, whichLocation));
    return jass_pushboolean(j, unit_issueorder(whichUnit, G_OrderId2String(order), whichLocation));
}
uint32_t IssueTargetOrder(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    cstring_t order = jass_checkstring(j, 2);
    edict_t *targetWidget = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, unit_issuetargetorder(whichUnit, order, targetWidget));
}
uint32_t IssueTargetOrderById(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    uint32_t order = (uint32_t)jass_checkinteger(j, 2);
    edict_t *targetWidget = jass_checkhandle(j, 3, "widget");
    cstring_t order_name = G_OrderId2String(order);
    bool accepted;

    /* Blizzard.j routes IssueBuildOrderByIdLocBJ('ugol') through this target
     * native after finding the neutral ngol. Translate that retail-specific
     * target order into the shared build path so the Acolyte walks to the mine
     * and construction performs the normal overlay binding on arrival. */
    if (order == order_ugol && targetWidget && targetWidget->class_id == unit_ngol)
        accepted = G_IssueBuildOrder(whichUnit, order, &targetWidget->s.origin2);
    else
        accepted = unit_issuetargetorder(whichUnit, order_name, targetWidget);

    /* The numeric argument is an order id, not a unit rawcode. Build-on-mine
     * placement uses the explicit ugol target case above; other ids remain
     * ordinary order ids and must not be interpreted as unit rawcodes. */
    return jass_pushboolean(j, accepted);
}
uint32_t IssueInstantTargetOrder(jass_t *j) {
    //edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    //cstring_t order = jass_checkstring(j, 2);
    //handle_t targetWidget = jass_checkhandle(j, 3, "widget");
    //handle_t instantTargetWidget = jass_checkhandle(j, 4, "widget");
    return jass_pushboolean(j, 0);
}
uint32_t IssueInstantTargetOrderById(jass_t *j) {
    //edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    //int32_t order = jass_checkinteger(j, 2);
    //handle_t targetWidget = jass_checkhandle(j, 3, "widget");
    //handle_t instantTargetWidget = jass_checkhandle(j, 4, "widget");
    return jass_pushboolean(j, 0);
}
/* WC3 string build orders use the four-character unit object id, unlike
 * normal ability order strings. Route through the same path as ById. */
uint32_t IssueBuildOrder(jass_t *j) {
    edict_t *builder = jass_checkhandle(j,1,"unit");
    cstring_t unit_to_build = jass_checkstring(j,2);
    vec2_t location = { jass_checknumber(j,3), jass_checknumber(j,4) };
    uint32_t id;
    if (!unit_to_build || strlen(unit_to_build) != 4)
        return jass_pushboolean(j,false);
    id = MAKEFOURCC(unit_to_build[0],unit_to_build[1],
                    unit_to_build[2],unit_to_build[3]);
    return jass_pushboolean(j, G_UnitIsBuilding(id) &&
        G_IssueBuildOrder(builder,id,&location));
}
uint32_t IssueBuildOrderById(jass_t *j) {
    edict_t *whichPeon = jass_checkhandle(j, 1, "unit");
    uint32_t unitId = (uint32_t)jass_checkinteger(j, 2);
    vec2_t point = { jass_checknumber(j, 3), jass_checknumber(j, 4) };
    bool accepted;

    accepted = G_IssueBuildOrder(whichPeon, unitId, &point);
    return jass_pushboolean(j, accepted);
}
uint32_t SetResourceAmount(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    int32_t amount = jass_checkinteger(j, 2);
    if (whichUnit) S_GoldMineSetResourceAmount(whichUnit, MAX(0, amount));
    return 0;
}
uint32_t AddResourceAmount(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    int32_t amount = jass_checkinteger(j, 2);
    if (whichUnit) whichUnit->resources += amount;
    return 0;
}
uint32_t GetResourceAmount(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichUnit ? whichUnit->resources : 0);
}
uint32_t WaygateGetDestinationX(jass_t *j) {
    edict_t *waygate = jass_checkhandle(j, 1, "unit");
    vec2_t destination = {0};
    S_WaygateGetDestination(waygate, &destination);
    return jass_pushnumber(j, destination.x);
}
uint32_t WaygateGetDestinationY(jass_t *j) {
    edict_t *waygate = jass_checkhandle(j, 1, "unit");
    vec2_t destination = {0};
    S_WaygateGetDestination(waygate, &destination);
    return jass_pushnumber(j, destination.y);
}
uint32_t WaygateSetDestination(jass_t *j) {
    edict_t *waygate = jass_checkhandle(j, 1, "unit");
    vec2_t destination = { jass_checknumber(j, 2), jass_checknumber(j, 3) };
    S_WaygateSetDestination(waygate, &destination);
    return 0;
}
uint32_t WaygateActivate(jass_t *j) {
    edict_t *waygate = jass_checkhandle(j, 1, "unit");
    bool activate = jass_checkboolean(j, 2);
    S_WaygateSetActive(waygate, activate);
    return 0;
}
uint32_t WaygateIsActive(jass_t *j) {
    edict_t *waygate = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, S_WaygateIsActive(waygate));
}
uint32_t UnitAddIndicator(jass_t *j) {
    edict_t *whichUnit = jass_checkhandle(j, 1, "unit");
    int32_t red = jass_checkinteger(j, 2);
    int32_t green = jass_checkinteger(j, 3);
    int32_t blue = jass_checkinteger(j, 4);
    int32_t alpha = jass_checkinteger(j, 5);
    color32_t color = MAKE(color32_t,
        (uint8_t)MAX(0, MIN(255, red)),
        (uint8_t)MAX(0, MIN(255, green)),
        (uint8_t)MAX(0, MIN(255, blue)),
        (uint8_t)MAX(0, MIN(255, alpha)));

    G_SendWidgetIndicator(whichUnit, color, currentplayer);
    return 0;
}
/* Creep leash opt-in/out is separate from AI's replaceable guard-post roster. */
uint32_t SetUnitCreepGuard(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    bool enabled = jass_checkboolean(j, 2);
    if (unit) G_CreepGuardSetEnabled(unit, enabled);
    return 0;
}
uint32_t RemoveGuardPosition(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    if (unit) G_BotRemoveGuardPosition(unit);
    return 0;
}
uint32_t RecycleGuardPosition(jass_t *j) {
    edict_t *unit = jass_checkhandle(j, 1, "unit");
    if (unit) G_BotRecycleGuardPosition(unit);
    return 0;
}
uint32_t CreateUnit(jass_t *j) {
    player_t *player = jass_checkhandle(j, 1, "player");
    uint32_t unitid = jass_checkinteger(j, 2);
    vec2_t location = MAKE(vec2_t, jass_checknumber(j, 3), jass_checknumber(j, 4));
    float facing = jass_checknumber(j, 5);
    /* ORDR difficulty screen uses twelve neutral-passive units: u000-u009,
     * u00A-u00C. Confirm real engine entities, not merely CUN() entry. */
    bool ordr_choice_unit = (level.time >= 3900 && level.time <= 6000) &&
        ((unitid >= 0x75303030u && unitid <= 0x75303039u) ||
         (unitid >= 0x75303041u && unitid <= 0x75303043u));
    if (!player) {
        if (ordr_choice_unit)
            jass_ordr_log_event(j, "CHOICE_CREATE_FAIL", "no player for difficulty unit");
        return jass_pushnullhandle(j, "unit");
    }
    edict_t *unit = unit_create(PLAYER_NUM(player), unitid, &location, facing);
    /* Trace gameplay spawns as well as difficulty menu picks. Map scripting
     * can create an edict successfully even when SP_CallSpawn has no UnitUI
     * model row, sets SVF_NOCLIENT, or the player cannot see it through fog. */
    if (level.time >= 4000 && jass_functionbyname(j, "CUN") && unit) {
        static unsigned ordr_spawn_samples;
        if (ordr_spawn_samples++ < 48) {
            char detail[300];
            snprintf(detail, sizeof(detail),
                     "id=%c%c%c%c owner=%u edict=%d pos=(%.0f,%.0f) model=%u flags=0x%x svflags=0x%x unit_ui_model=%s visible_p0=%d",
                     (int)((unitid>>24)&255), (int)((unitid>>16)&255),
                     (int)((unitid>>8)&255), (int)(unitid&255),
                     (unsigned)PLAYER_NUM(player), (int)unit->s.number,
                     (double)unit->s.origin.x, (double)unit->s.origin.y,
                     (unsigned)unit->s.model, (unsigned)unit->s.flags,
                     (unsigned)unit->svflags,
                     unit->data.UnitUI && unit->data.UnitUI->modelFile ? unit->data.UnitUI->modelFile : "(none)",
                     (int)G_FowPlayerCanSeeEntity(0, unit));
            jass_ordr_log_event(j, "ORDR_UNIT_RENDER", detail);
        }
    }
    if (ordr_choice_unit) {
        char detail[256];
        snprintf(detail, sizeof(detail),
                 "type=%c%c%c%c player=%u position=(%.0f,%.0f) created=%d entity=%d actual=(%.0f,%.0f) flags=0x%x",
                 (int)((unitid >> 24) & 255), (int)((unitid >> 16) & 255),
                 (int)((unitid >> 8) & 255), (int)(unitid & 255),
                 (unsigned)PLAYER_NUM(player),
                 (double)location.x, (double)location.y,
                 unit && unit->inuse ? 1 : 0, unit ? (int)unit->s.number : -1,
                 unit ? (double)unit->s.origin.x : 0.0,
                 unit ? (double)unit->s.origin.y : 0.0,
                 unit ? (unsigned)unit->s.flags : 0u);
        jass_ordr_log_event(j, "CHOICE_CREATE", detail);
    }
    return jass_pushlighthandle(j, unit, "unit");
}
uint32_t CreateUnitByName(jass_t *j) {
    //handle_t whichPlayer = jass_checkhandle(j, 1, "player");
    //cstring_t unitname = jass_checkstring(j, 2);
    //float x = jass_checknumber(j, 3);
    //float y = jass_checknumber(j, 4);
    //float face = jass_checknumber(j, 5);
    return jass_pushnullhandle(j, "unit");
}
uint32_t CreateUnitAtLoc(jass_t *j) {
    player_t *player = jass_checkhandle(j, 1, "player");
    uint32_t unitid = jass_checkinteger(j, 2);
    vec2_t const *location = jass_checkhandle(j, 3, "location");
    float facing = jass_checknumber(j, 4);
    if (!player || !location) {
        return jass_pushnullhandle(j, "unit");
    }
    edict_t *unit = unit_create(PLAYER_NUM(player), unitid, location, facing);
    return jass_pushlighthandle(j, unit, "unit");
}
uint32_t CreateUnitAtLocByName(jass_t *j) {
    //handle_t id = jass_checkhandle(j, 1, "player");
    //cstring_t unitname = jass_checkstring(j, 2);
    //handle_t whichLocation = jass_checkhandle(j, 3, "location");
    //float face = jass_checknumber(j, 4);
    return jass_pushnullhandle(j, "unit");
}
uint32_t CreateCorpse(jass_t *j) {
    //handle_t whichPlayer = jass_checkhandle(j, 1, "player");
    //int32_t unitid = jass_checkinteger(j, 2);
    //float x = jass_checknumber(j, 3);
    //float y = jass_checknumber(j, 4);
    //float face = jass_checknumber(j, 5);
    return jass_pushnullhandle(j, "unit");
}
uint32_t CreateBlightedGoldmine(jass_t *j) {
    player_t *player = jass_checkhandle(j, 1, "player");
    vec2_t origin = MAKE(vec2_t, jass_checknumber(j, 2), jass_checknumber(j, 3));
    float facing = jass_checknumber(j, 4);
    edict_t *mine;

    if (!player || !(mine = S_CreateBlightedGoldmine(PLAYER_NUM(player), &origin, facing)))
        return jass_pushnullhandle(j, "unit");
    return jass_pushlighthandle(j, mine, "unit");
}
