extern player_t *currentplayer;

uint32_t CreateSound(jass_t *j) {
    cstring_t fileName = jass_checkstring(j, 1);
    bool looping = jass_checkboolean(j, 2);
    bool is3D = jass_checkboolean(j, 3);
    bool stopwhenoutofrange = jass_checkboolean(j, 4);
    int32_t fadeInRate = jass_checkinteger(j, 5);
    int32_t fadeOutRate = jass_checkinteger(j, 6);
    cstring_t eaxSetting = jass_checkstring(j, 7);
    (void)eaxSetting;
    API_ALLOC(gsound_t, sound);
    strlcpy(sound->fileName, fileName, sizeof(sound->fileName));
    sound->looping = looping;
    sound->is3D = is3D;
    sound->stopwhenoutofrange = stopwhenoutofrange;
    sound->fadeInRate = fadeInRate;
    sound->fadeOutRate = fadeOutRate;
    sound->soundIndex = gi.SoundIndex(fileName);
    G_JassSoundRuntimeInit(sound);
    return 1;
}
/* Label constructors resolve WC3 sound-data rows into the same game-owned sound
 * descriptor as CreateSound. Playback is sent through the entity/server sound
 * path; JASS never owns or calls the client mixer directly. */
uint32_t CreateSoundFilenameWithLabel(jass_t *j) {
    cstring_t fileName = jass_checkstring(j, 1);
    bool looping = jass_checkboolean(j, 2);
    bool is3D = jass_checkboolean(j, 3);
    bool stopwhenoutofrange = jass_checkboolean(j, 4);
    int32_t fadeInRate = jass_checkinteger(j, 5);
    int32_t fadeOutRate = jass_checkinteger(j, 6);
    cstring_t SLKEntryName = jass_checkstring(j, 7);
    float volume = 1.0f;

    API_ALLOC(gsound_t, sound);
    strlcpy(sound->fileName, fileName, sizeof(sound->fileName));
    sound->looping = looping;
    sound->is3D = is3D;
    sound->stopwhenoutofrange = stopwhenoutofrange;
    sound->fadeInRate = fadeInRate;
    sound->fadeOutRate = fadeOutRate;
    sound->soundIndex = gi.SoundIndex(fileName);
    G_JassSoundRuntimeInit(sound);
    if (G_SoundLabelDescriptor(SLKEntryName, NULL, 0, NULL, &volume))
        G_JassSoundSetVolume(sound, volume);
    return 1;
}
uint32_t CreateSoundFromLabel(jass_t *j) {
    cstring_t soundLabel = jass_checkstring(j, 1);
    bool looping = jass_checkboolean(j, 2);
    bool is3D = jass_checkboolean(j, 3);
    bool stopwhenoutofrange = jass_checkboolean(j, 4);
    int32_t fadeInRate = jass_checkinteger(j, 5);
    int32_t fadeOutRate = jass_checkinteger(j, 6);
    char path[sizeof(((gsound_t *)0)->fileName)] = { 0 };
    float volume = 1.0f;
    int sound_index = 0;

    API_ALLOC(gsound_t, sound);
    G_SoundLabelDescriptor(soundLabel, path, sizeof(path), &sound_index, &volume);
    strlcpy(sound->fileName, path, sizeof(sound->fileName));
    sound->looping = looping;
    sound->is3D = is3D;
    sound->stopwhenoutofrange = stopwhenoutofrange;
    sound->fadeInRate = fadeInRate;
    sound->fadeOutRate = fadeOutRate;
    sound->soundIndex = sound_index;
    G_JassSoundRuntimeInit(sound);
    G_JassSoundSetVolume(sound, volume);
    return 1;
}
uint32_t CreateMIDISound(jass_t *j) {
    cstring_t label = jass_checkstring(j, 1);
    int fadein = jass_checkinteger(j, 2), fadeout = jass_checkinteger(j, 3);
    char path[sizeof(((gsound_t *)0)->fileName)] = {0};
    int sound_index = 0;
    float volume = 1.0f;
    API_ALLOC(gsound_t, sound);
    G_JassSoundRuntimeInit(sound);
    if (G_SoundLabelDescriptor(label, path, sizeof(path), &sound_index, &volume)) {
        strlcpy(sound->fileName, path, sizeof(sound->fileName));
        sound->soundIndex = sound_index;
        G_JassSoundSetVolume(sound, volume);
    }
    sound->fadeInRate = fadein; sound->fadeOutRate = fadeout;
    return 1;
}
uint32_t SetSoundParamsFromLabel(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    cstring_t soundLabel = jass_checkstring(j, 2);
    float volume = 1.0f;

    /* This native changes the label-authored playback parameters, not the
     * filename chosen when the handle was created. OpenRealm currently
     * transports authored volume; pitch/channel/distance remain mixer gaps. */
    if (sound && G_SoundLabelDescriptor(soundLabel, NULL, 0, NULL, &volume))
        G_JassSoundSetVolume(sound, volume);
    return 0;
}
uint32_t SetSoundDistanceCutoff(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    float cutoff = jass_checknumber(j, 2);
    if (sound && isfinite(cutoff) && cutoff >= 0.0f) sound->distance_cutoff = cutoff;
    return 0;
}
uint32_t SetSoundChannel(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    int channel = jass_checkinteger(j, 2);
    if (sound && channel >= 0 && channel <= 7) sound->channel = channel;
    return 0;
}
uint32_t SetSoundVolume(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    int32_t volume = jass_checkinteger(j, 2);
    if (sound) G_JassSoundSetVolume(sound, (float)MAX(0, MIN(volume, 127)) / 127.0f);
    return 0;
}
uint32_t SetSoundPitch(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    float pitch = jass_checknumber(j, 2);
    if (sound && isfinite(pitch) && pitch > 0.0f) sound->pitch = pitch;
    return 0;
}
uint32_t SetSoundDistances(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    float minv = jass_checknumber(j, 2), maxv = jass_checknumber(j, 3);
    if (sound && isfinite(minv) && isfinite(maxv) && minv >= 0.0f && maxv >= minv) {
        sound->min_distance = minv; sound->max_distance = maxv;
    }
    return 0;
}
uint32_t SetSoundConeAngles(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    float inner = jass_checknumber(j, 2), outer = jass_checknumber(j, 3);
    int vol = jass_checkinteger(j, 4);
    if (sound && isfinite(inner) && isfinite(outer) &&
        inner >= 0.0f && outer >= inner && outer <= 360.0f) {
        sound->cone_inner = inner;
        sound->cone_outer = outer;
        sound->cone_outside_volume = MAX(0, MIN(127, vol));
    }
    return 0;
}
uint32_t SetSoundConeOrientation(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    float x = jass_checknumber(j, 2), y = jass_checknumber(j, 3), z = jass_checknumber(j, 4);
    if (sound && isfinite(x) && isfinite(y) && isfinite(z))
        sound->cone_orientation = MAKE(vec3_t, x, y, z);
    return 0;
}
uint32_t SetSoundPosition(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    float x = jass_checknumber(j, 2);
    float y = jass_checknumber(j, 3);
    float z = jass_checknumber(j, 4);
    if (sound) G_JassSoundSetPosition(sound, &MAKE(vec3_t, x, y, z));
    return 0;
}
uint32_t SetSoundVelocity(jass_t *j) {
    //handle_t soundHandle = jass_checkhandle(j, 1, "sound");
    //float x = jass_checknumber(j, 2);
    //float y = jass_checknumber(j, 3);
    //float z = jass_checknumber(j, 4);
    return 0;
}
uint32_t AttachSoundToUnit(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    edict_t *whichUnit = jass_checkhandle(j, 2, "unit");
    if (sound) G_JassSoundAttach(sound, whichUnit);
    return 0;
}
/* StartSound snapshots the current transient sound-handle presentation state
 * into one generic sound packet. Continuous attachment tracking and stop/fade
 * playback lifetime remain separate mixer work. */
uint32_t StartSound(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    jassSoundPlayback_t playback;
    float attenuation;

    if (!sound || !sound->soundIndex) return 0;
    G_JassSoundPlayback(sound, &playback);
    attenuation = sound->is3D ? 1.0f : 0.0f;

    if (currentplayer) {
        edict_t *recipient = PLAYER_ENT(currentplayer);
        if (!recipient || !recipient->client || !recipient->client->connected) return 0;
        if (playback.positioned)
            G_PlaySound(&playback.origin, recipient, CHAN_OWNER | CHAN_RELIABLE | (sound->channel & 7), sound->soundIndex,
                               playback.volume, attenuation, 0.0f);
        else
            G_PlaySound(NULL, recipient, CHAN_OWNER | CHAN_RELIABLE | (sound->channel & 7), sound->soundIndex,
                     playback.volume, attenuation, 0.0f);
        G_JassSoundMarkStarted(sound);
        return 0;
    }

    if (playback.positioned)
        G_PlaySound(&playback.origin, playback.emitter, CHAN_RELIABLE | (sound->channel & 7), sound->soundIndex,
                           playback.volume, attenuation, 0.0f);
    else
        G_PlaySound(NULL, NULL, CHAN_RELIABLE | (sound->channel & 7), sound->soundIndex, playback.volume, attenuation, 0.0f);
    G_JassSoundMarkStarted(sound);
    return 0;
}
/* StopSound sends an explicit command to the iPad mixer; clearing a JASS
 * boolean is insufficient because sound playback is client-owned. */
uint32_t StopSound(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    bool kill = jass_checkboolean(j, 2), fade = jass_checkboolean(j, 3);
    (void)fade; /* Mixer ramp remains separate work. */
    if (!sound) return 0;
    sound->kill_when_done = kill;
    sound->started = false;
    if (sound->soundIndex <= 0) return 0;
    FOR_LOOP(i, game.max_clients) {
        gameClient_t *client = &game.clients[i];
        if (!client->connected) continue;
        if (currentplayer && client->ps.number != PLAYER_NUM(currentplayer)) continue;
        edict_t *ent = G_GetPlayerEntityByNumber(client->ps.number);
        if (!ent || !ent->client) continue;
        gi.Write(PF_BYTE, &(int32_t){svc_sound});
        gi.Write(PF_BYTE, &(int32_t){SND_STOP});
        gi.Write(PF_SHORT, &(int32_t){sound->soundIndex});
        gi.unicast(ent);
    }
    return 0;
}
uint32_t KillSoundWhenDone(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    if (sound) sound->kill_when_done = true;
    return 0;
}
uint32_t SetMusicVolume(jass_t *j) {
    G_MusicSetVolume(jass_checkinteger(j, 1));
    return 0;
}
uint32_t SetMusicPlayPosition(jass_t *j) {
    G_MusicSetPosition(jass_checkinteger(j, 1));
    return 0;
}
uint32_t SetThematicMusicVolume(jass_t *j) {
    G_MusicSetThematicVolume(jass_checkinteger(j, 1));
    return 0;
}
uint32_t SetThematicMusicPlayPosition(jass_t *j) {
    G_MusicSetThematicPosition(jass_checkinteger(j, 1));
    return 0;
}
uint32_t PlayMusic(jass_t *j) {
    G_MusicPlay(jass_checkstring(j, 1), 0, 0);
    return 0;
}
uint32_t PlayMusicEx(jass_t *j) {
    cstring_t musicName = jass_checkstring(j, 1);
    int32_t frommsecs = jass_checkinteger(j, 2);
    int32_t fadeinmsecs = jass_checkinteger(j, 3);
    G_MusicPlay(musicName, MAX(0, frommsecs), MAX(0, fadeinmsecs));
    return 0;
}
uint32_t SetMapMusic(jass_t *j) {
    cstring_t musicName = jass_checkstring(j, 1);
    bool random = jass_checkboolean(j, 2);
    int32_t index = jass_checkinteger(j, 3);
    G_MusicSetMap(musicName, random, index);
    return 0;
}
uint32_t ClearMapMusic(jass_t *j) {
    G_MusicClearMap();
    return 0;
}
uint32_t PlayThematicMusic(jass_t *j) {
    G_MusicPlayThematic(jass_checkstring(j, 1), 0);
    return 0;
}
uint32_t PlayThematicMusicEx(jass_t *j) {
    cstring_t musicFileName = jass_checkstring(j, 1);
    int32_t frommsecs = jass_checkinteger(j, 2);
    G_MusicPlayThematic(musicFileName, MAX(0, frommsecs));
    return 0;
}
uint32_t EndThematicMusic(jass_t *j) {
    G_MusicEndThematic();
    return 0;
}
uint32_t StopMusic(jass_t *j) {
    G_MusicStop(jass_checkboolean(j, 1));
    return 0;
}
uint32_t ResumeMusic(jass_t *j) {
    G_MusicResume();
    return 0;
}
uint32_t SetSoundDuration(jass_t *j) {
    gsound_t *soundHandle = jass_checkhandle(j, 1, "sound");
    soundHandle->duration = jass_checkinteger(j, 2);
    return 0;
}
uint32_t GetSoundDuration(jass_t *j) {
    gsound_t *soundHandle = jass_checkhandle(j, 1, "sound");
    return jass_pushinteger(j, soundHandle->duration);
}
uint32_t GetSoundFileDuration(jass_t *j) {
    return jass_pushinteger(j, G_SoundFileDuration(jass_checkstring(j, 1)));
}
uint32_t VolumeGroupSetVolume(jass_t *j) {
    //handle_t vgroup = jass_checkhandle(j, 1, "volumegroup");
    //float scale = jass_checknumber(j, 2);
    return 0;
}
uint32_t VolumeGroupReset(jass_t *j) {
    return 0;
}
uint32_t GetSoundIsPlaying(jass_t *j) {
    gsound_t *sound = jass_checkhandle(j, 1, "sound");
    return jass_pushboolean(j, sound && sound->started &&
        (sound->looping || !sound->duration || G_JassSoundRemainingDuration(sound) > 0));
}
uint32_t GetSoundIsLoading(jass_t *j) {
    //handle_t soundHandle = jass_checkhandle(j, 1, "sound");
    return jass_pushboolean(j, 0);
}
uint32_t RegisterStackedSound(jass_t *j) {
    //handle_t soundHandle = jass_checkhandle(j, 1, "sound");
    //bool byPosition = jass_checkboolean(j, 2);
    //float rectwidth = jass_checknumber(j, 3);
    //float rectheight = jass_checknumber(j, 4);
    return 0;
}
uint32_t UnregisterStackedSound(jass_t *j) {
    //handle_t soundHandle = jass_checkhandle(j, 1, "sound");
    //bool byPosition = jass_checkboolean(j, 2);
    //float rectwidth = jass_checknumber(j, 3);
    //float rectheight = jass_checknumber(j, 4);
    return 0;
}
