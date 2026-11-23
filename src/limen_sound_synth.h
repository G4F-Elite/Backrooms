#pragma once
// Part of limen_sound.h: the synthesizer itself.

// ------------------------------------------------------------------ the synthesizer
struct LmSoundSynth {
    static const int CONTROL = 32;              // samples per control tick
    static const int ECHO_LEN = 65536;          // 1.48 s: a wall 250 m away and back
    static const int STEPS = 3, DRIPS = 4, CREAKS = 2;

    LmSoundScene sc;                            // latest target scene
    LsRng rng;
    int ctl = 0;
    float stale = 10.0f, world = 0.0f;          // seconds since the last scene; 0..1 fade of the world layers
    float rightX = -1.0f, rightZ = 0.0f;

    // Lamps
    LsOsc mains2, mains4, mains6;
    LsLampVoice lamps[LS_LAMPS];
    // Room tone
    LsPink roomPinkL, roomPinkR;
    LsSvf modeL[2], modeR[2];
    float roomGain = 0.0f, hissGain = 0.0f, hissL = 0.0f, hissR = 0.0f, hissC = 0.2f;
    float curMode[3] = {80.0f, 76.0f, 60.0f};
    // Shaft draught
    float brown = 0.0f, voidG = 0.0f, voidHz = 110.0f;
    LsSvf howl, howl2;
    LsWander gust, gustFast;
    LsPan voidPan;
    // Air plant
    LsOsc fan1, fan2, fan3;
    LsPink ductPink;
    LsSvf duct;
    LsWander fanDrift;
    float plantG = 0.0f, plantHz = 80.0f, plantTilt = 0.5f, fanAm = 0.0f;
    // Party behind the wall
    float partyG = 0.0f, partyBeat = 0.0f, partyBps = 2.0f, partyKey = 55.0f, partyNote = 55.0f;
    float partyKickPh = 0.0f, partyBassPh = 0.0f, partyLp1 = 0.0f, partyLp2 = 0.0f;
    int partyStep = -1;
    LsPan partyPan;
    // Drone
    LsOsc drone[4];
    LsWander droneWow, droneSwell;
    float droneG = 0.0f, droneHz = 45.0f, ratioA = 1.3f, ratioB = 2.0f, warble = 0.0f;
    // Events
    LsDripVoice drips[DRIPS];
    LsCreakVoice creaks[CREAKS];
    // Footsteps
    LsStepVoice steps[STEPS];
    int nextStep = 0;
    bool leftFoot = false;
    // Space
    float echoBuf[ECHO_LEN] = {};
    int echoW = 0;
    LsEchoTap taps[2];
    LsReverb reverb;
    float rt60 = 0.8f, absorb = 0.2f, meanR = 5.0f;
    float sendSteps = 0.3f;
    // Meters
    double power[LSL_COUNT] = {};
    long long metered = 0;

    LmSoundSynth() {
        memset(&sc, 0, sizeof(sc));
        mains2.set(120.0f); mains4.set(240.0f); mains6.set(360.0f);
        howl.set(110.0f, 3.0f); howl2.set(420.0f, 0.8f);
        duct.set(600.0f, 0.7f);
        for (int i = 0; i < 2; i++) { modeL[i].set(80.0f, 8.0f); modeR[i].set(90.0f, 8.0f); }
    }

    // Audio thread, once per buffer: the freshest scene the game thread measured.
    void setScene(const LmSoundScene& s) { sc = s; stale = 0.0f; }

    // --- footsteps ---------------------------------------------------------------------
    // The floor is whatever the generator put under the player at this very moment.
    void startStep(float pitch, float sprint) {
        LsStepVoice& v = steps[nextStep];
        nextStep = (nextStep + 1) % STEPS;
        leftFoot = !leftFoot;
        v = LsStepVoice();
        v.t = 0.0f;
        v.pitch = lsClamp(pitch, 0.7f, 1.45f);
        v.weight = 0.8f + sprint * 0.45f;
        v.hard = sc.hard; v.grit = sc.grit; v.hollow = sc.hollow; v.wet = sc.wet; v.metal = sc.metal;
        v.toe = rng.range(0.065f, 0.095f) * (1.0f - 0.25f * sprint);
        v.clickDecay = 260.0f + 900.0f * v.hard;
        v.click.set((1300.0f + 2500.0f * v.hard) * v.pitch * rng.range(0.9f, 1.1f), 1.1f + 1.4f * v.hard);
        v.scuff.set(rng.range(650.0f, 1150.0f) * v.pitch, 0.9f);
        v.splash.set(rng.range(2600.0f, 4200.0f), 0.8f);
        // A deck over a drop rings like a plate: inharmonic partials over a low boom.
        float base = rng.range(88.0f, 128.0f) * v.pitch * (1.0f - 0.2f * v.hollow);
        static const float ratio[5] = {1.0f, 2.31f, 3.87f, 5.40f, 8.93f};
        for (int k = 0; k < 5; k++) v.mhz[k] = base * ratio[k] * rng.range(0.97f, 1.03f);
        v.dur = 0.30f + 0.35f * v.hollow + 0.15f * v.wet;
        v.pan.set(leftFoot ? -0.16f : 0.16f);
    }

    float renderStep(LsStepVoice& v) {
        float t = v.t, toe = t - v.toe;
        float n = rng.bi();
        // Heel: the softer the floor, the more of the hit turns into a low thud.
        v.phase += LS_TAU * (50.0f + 72.0f * expf(-t * 46.0f)) * v.pitch * LS_DT;
        float thud = sinf(v.phase) * lsPluck(t, 0.004f, 30.0f + 26.0f * v.hard) * (0.80f - 0.36f * v.hard);
        // Hard floors answer with a sharp heel click and a second one as the toe lands.
        float cenv = expf(-t * v.clickDecay) + (toe >= 0.0f ? 0.6f * expf(-toe * v.clickDecay * 0.8f) : 0.0f);
        float roll = lsPluck(t, 0.006f, 44.0f) + lsPluck(toe, 0.012f, 38.0f) * 0.55f;
        // Grit: loose grains crack under the sole, each one a separate tiny impact.
        float crunch = (rng.uni() < v.grit * 0.035f) ? rng.bi() * 4.0f * roll * v.grit : 0.0f;
        float click = v.click.bp(n * cenv * (0.08f + 1.25f * v.hard * v.hard) + crunch);
        // Soft floors: fibres scuffing as the foot rolls.
        v.body += (n - v.body) * 0.035f;
        float soft = (1.0f - v.hard) * 0.95f + v.grit * 0.25f;
        float scuff = (v.scuff.bp(n) * 0.42f + v.body * 1.3f) * roll * soft;
        float boom = 0.0f;
        if (v.hollow > 0.01f) {
            static const float decay[5] = {13.0f, 21.0f, 34.0f, 16.0f, 27.0f};
            static const float amp[5] = {1.0f, 0.5f, 0.28f, 0.0f, 0.0f};
            float attack = t < 0.003f ? t / 0.003f : 1.0f;
            for (int k = 0; k < 5; k++) {
                float a = k < 3 ? amp[k] : v.metal * 0.22f;
                if (a <= 0.0f) continue;
                v.mph[k] += LS_TAU * v.mhz[k] * LS_DT;
                boom += sinf(v.mph[k]) * expf(-t * decay[k]) * a;
            }
            boom *= attack * v.hollow * 0.55f;
        }
        float splash = 0.0f;
        if (v.wet > 0.01f) {
            float env = lsPluck(t - 0.006f, 0.008f, 24.0f) + lsPluck(toe - 0.004f, 0.010f, 20.0f) * 0.7f;
            splash = v.splash.bp(n) * env * v.wet * 1.1f;
        }
        v.t += LS_DT;
        if (v.t > v.dur) v.t = -1.0f;
        return (thud * 0.75f + click + scuff + boom + splash) * v.weight * 0.50f;
    }

    // --- events ------------------------------------------------------------------------
    void startDrip() {
        LsDripVoice* v = nullptr;
        for (int i = 0; i < DRIPS; i++) if (drips[i].t < 0.0f) { v = &drips[i]; break; }
        if (!v) return;
        *v = LsDripVoice();
        v->t = 0.0f;
        v->wet = rng.uni() < sc.dripWet ? 1.0f : 0.0f;
        v->f0 = v->wet > 0.5f ? rng.range(650.0f, 1500.0f) : rng.range(1700.0f, 3000.0f);
        v->sweep = rng.range(0.9f, 1.9f);
        v->dur = v->wet > 0.5f ? rng.range(0.045f, 0.095f) : rng.range(0.010f, 0.018f);
        v->gain = rng.range(0.35f, 1.0f);
        v->tick.set(v->f0 * 2.2f, 1.5f);
        // Heard from where the water is, or from anywhere if it is just the ceiling leaking.
        float side = sc.dripDx * rightX + sc.dripDz * rightZ;
        v->pan.set(lsClamp(side * 0.8f + rng.range(-0.6f, 0.6f), -0.95f, 0.95f));
    }

    float renderDrip(LsDripVoice& v) {
        float k = v.t / v.dur;
        float out;
        if (v.wet > 0.5f) {
            // A drop closing a bubble under water: a short sine that chirps upward.
            v.phase += LS_TAU * v.f0 * (1.0f + v.sweep * k) * LS_DT;
            out = sinf(v.phase) * lsPluck(v.t, 0.0012f, 3.2f / v.dur) * 0.9f;
            out += v.tick.bp(rng.bi()) * expf(-v.t * 900.0f) * 0.35f;
        } else {
            out = v.tick.bp(rng.bi()) * expf(-v.t * 260.0f) * 1.1f;
        }
        v.t += LS_DT;
        if (v.t > v.dur * 3.0f + 0.02f) v.t = -1.0f;
        return out * v.gain * 0.12f;
    }

    void startCreak() {
        LsCreakVoice* v = nullptr;
        for (int i = 0; i < CREAKS; i++) if (creaks[i].type < 0) { v = &creaks[i]; break; }
        if (!v) return;
        *v = LsCreakVoice();
        v->type = rng.uni() < 0.35f + 0.4f * sc.creakSize ? 1 : 0;
        v->pan.set(rng.range(-0.9f, 0.9f));
        v->gain = rng.range(0.5f, 1.0f);
        if (v->type == 0) {
            v->f = rng.range(380.0f, 1500.0f) * (1.0f - 0.5f * sc.creakSize);
            v->dur = 1.2f;
        } else {
            v->f = rng.range(70.0f, 170.0f) * (1.0f - 0.35f * sc.creakSize);
            v->dur = rng.range(1.4f, 3.4f);
            v->slipHz = rng.range(7.0f, 19.0f);
            v->res.set(v->f * 3.0f, 9.0f);
        }
    }

    float renderCreak(LsCreakVoice& v) {
        float out = 0.0f;
        if (v.type == 0) {
            static const float ratio[3] = {1.0f, 2.76f, 5.40f};
            static const float amp[3] = {1.0f, 0.5f, 0.25f};
            static const float decay[3] = {5.0f, 9.0f, 16.0f};
            for (int k = 0; k < 3; k++) {
                v.ph[k] += LS_TAU * v.f * ratio[k] * LS_DT;
                out += sinf(v.ph[k]) * amp[k] * expf(-v.t * decay[k]);
            }
            out *= (v.t < 0.002f ? v.t / 0.002f : 1.0f) * 0.035f;
        } else {
            // Stick-slip: the beam lets go in a train of tiny jerks whose rate wanders.
            float k = v.t / v.dur;
            float w = v.wander.next(rng, 6.0f * LS_DT);
            v.slipPh += v.slipHz * (1.0f + 0.5f * w - 0.3f * k) * LS_DT;
            if (v.slipPh >= 1.0f) v.slipPh -= 1.0f;
            float slip = expf(-v.slipPh * 9.0f);
            v.brown = (v.brown + 0.05f * rng.bi()) * 0.99f;
            float env = sinf(3.14159265f * k);
            out = v.res.bp(v.brown * slip * 6.0f + rng.bi() * slip * 0.15f) * env * env * 0.07f;
        }
        v.t += LS_DT;
        if (v.t >= v.dur) v.type = -1;
        return out * v.gain;
    }

    // --- control rate: scene -> synth parameters -------------------------------------------
    void assignLamps(float humUser) {
        for (int i = 0; i < LS_LAMPS; i++) {
            LsLampVoice& v = lamps[i];
            if (v.id == 0) continue;
            const LmSoundLamp* src = nullptr;
            for (int k = 0; k < LS_LAMPS; k++) if (sc.lamp[k].id == v.id) src = &sc.lamp[k];
            if (!src || stale > 0.6f) {
                v.gT = 0.0f;
                if (v.g < 1e-5f && v.tickT < 0.0f) v.id = 0;
                continue;
            }
            updateLamp(v, *src, humUser);
        }
        if (stale > 0.6f) return;
        for (int k = 0; k < LS_LAMPS; k++) {
            const LmSoundLamp& L = sc.lamp[k];
            if (L.id == 0) continue;
            bool have = false;
            for (int i = 0; i < LS_LAMPS; i++) if (lamps[i].id == L.id) have = true;
            if (have) continue;
            for (int i = 0; i < LS_LAMPS; i++) {
                if (lamps[i].id != 0) continue;
                initLamp(lamps[i], L);
                updateLamp(lamps[i], L, humUser);
                break;
            }
        }
    }

    // Every tube is its own instrument: the identity of its cell decides how it hums.
    void initLamp(LsLampVoice& v, const LmSoundLamp& L) {
        v = LsLampVoice();
        v.id = L.id;
        u32 h = L.id * 0x9E3779B1u; h ^= h >> 15; h *= 0x85EBCA6Bu; h ^= h >> 13;
        auto u = [&](int shift) { return (float)((h >> shift) & 255u) / 255.0f; };
        float tilt = lsClamp(sc.humTilt + (u(0) - 0.5f) * 0.5f, 0.0f, 1.0f);
        v.w1 = 1.0f;
        v.w2 = 0.25f + 0.55f * tilt;
        v.w3 = 0.08f + 0.40f * tilt * u(8);
        float p = LS_TAU * u(16), q = LS_TAU * u(24), r = LS_TAU * u(4);
        v.cp = cosf(p); v.sp = sinf(p); v.cq = cosf(q); v.sq = sinf(q); v.cr = cosf(r); v.sr = sinf(r);
        v.buzz = 0.02f + 0.11f * tilt * (0.4f + 0.6f * u(12));
        v.bz.set(2600.0f + 3200.0f * u(20), 0.8f);
        v.tk.set(2200.0f + 1400.0f * u(2), 2.5f);
        v.restless = u(10) < sc.unstable * 0.55f;
        v.live = L.live;
        v.gate = v.gateT = L.live ? 1.0f : 0.0f;
        v.nextToggle = rng.range(1.0f, 8.0f);
    }

    void updateLamp(LsLampVoice& v, const LmSoundLamp& L, float humUser) {
        float dx = L.x - sc.px, dz = L.z - sc.pz;
        float d2 = dx * dx + dz * dz + L.h * L.h;
        float d = sqrtf(d2);
        v.live = L.live;
        v.gT = 0.26f * humUser * L.occl / (1.0f + d2 / 9.0f);
        v.pan.set((dx * rightX + dz * rightZ) / d);
        // Distance and walls take the buzz first and leave the hum.
        v.lpC = lsClamp((L.occl > 0.5f ? 0.85f : 0.10f) * 6.0f / (6.0f + d) * (1.0f - 0.4f * sc.airDull), 0.02f, 0.85f);
    }

    void controlLamp(LsLampVoice& v, float dt) {
        v.g += (v.gT - v.g) * 0.06f;
        float rest = v.live ? 1.0f : 0.0f;
        if (!v.restless) { v.gateT = rest; }
        else {
            v.nextToggle -= dt;
            if (v.burst > 0.0f) {
                v.burst -= dt;
                if (v.nextToggle <= 0.0f) {
                    // A stutter: the arc drops or catches, the starter clicks.
                    bool on = v.gateT > 0.5f;
                    v.gateT = v.live ? (on ? rng.range(0.05f, 0.35f) : 1.0f) : (on ? 0.0f : rng.range(0.5f, 1.0f));
                    v.nextToggle = rng.range(0.02f, 0.11f);
                    v.tickT = 0.0f;
                }
                if (v.burst <= 0.0f) { v.gateT = rest; v.nextToggle = rng.range(2.5f, 11.0f) / (0.3f + sc.unstable); }
            } else {
                v.gateT = rest;
                if (v.nextToggle <= 0.0f) { v.burst = rng.range(0.25f, 1.1f); v.nextToggle = 0.0f; }
            }
        }
    }

    void control(const LsMix& mix) {
        const float dt = (float)CONTROL * LS_DT;
        stale += dt;
        world += ((stale < 0.6f ? 1.0f : 0.0f) - world) * 0.01f;
        rightX = -cosf(sc.yaw); rightZ = sinf(sc.yaw);

        // Lamps
        mains2.set(sc.mainsHz * 2.0f); mains4.set(sc.mainsHz * 4.0f); mains6.set(sc.mainsHz * 6.0f);
        mains2.renorm(); mains4.renorm(); mains6.renorm();
        assignLamps(mix.hum);
        for (int i = 0; i < LS_LAMPS; i++) if (lamps[i].id) controlLamp(lamps[i], dt);

        // Room: the modes follow the measured dimensions, small hard rooms ring the most
        const float glide = 0.02f;
        for (int k = 0; k < 3; k++) curMode[k] += (sc.mode[k] - curMode[k]) * glide;
        absorb += (sc.absorb - absorb) * glide;
        meanR += (sc.meanR - meanR) * glide;
        rt60 += (sc.rt60 - rt60) * 0.03f;
        float q = lsClamp(16.0f - 36.0f * absorb, 4.0f, 14.0f);
        modeL[0].set(curMode[0], q); modeL[1].set(curMode[2] * 1.02f, q * 0.7f);
        modeR[0].set(curMode[1], q); modeR[1].set(curMode[2] * 0.98f, q * 0.7f);
        roomGain += (0.55f + 0.45f * (1.0f - sc.open) - roomGain) * glide;
        hissGain += ((0.25f + 0.75f * sc.open) * (1.0f - 0.6f * sc.airDull) - hissGain) * glide;
        hissC = lsClamp(0.05f + 0.16f * sc.open, 0.03f, 0.25f);

        // Shaft draught
        voidG += (sc.voidGain - voidG) * 0.015f;
        if (sc.voidGain > 0.0f) {
            voidHz += (sc.voidHz - voidHz) * glide;
            voidPan.set((sc.voidDx * rightX + sc.voidDz * rightZ) * 0.85f);
        }
        float g1 = gust.next(rng, dt * 0.22f), g2 = gustFast.next(rng, dt * 1.3f);
        gustNow = lsClamp(0.55f + 0.35f * g1 + 0.18f * g2, 0.05f, 1.2f);
        howl.set(voidHz * (1.0f + 0.10f * g1 + 0.04f * g2), 3.2f);
        howl2.set(voidHz * 3.6f * (1.0f + 0.2f * g1), 0.9f);

        // Air plant
        plantG += (sc.plant - plantG) * 0.008f;
        plantHz += (sc.plantHz - plantHz) * 0.004f;
        plantTilt += (sc.plantTilt - plantTilt) * glide;
        float drift = 1.0f + 0.004f * fanDrift.next(rng, dt * 0.4f);
        fan1.set(plantHz * drift); fan2.set(plantHz * 2.0f * drift); fan3.set(plantHz * 3.02f * drift);
        fan1.renorm(); fan2.renorm(); fan3.renorm();
        duct.set(380.0f + 900.0f * plantTilt, 0.6f);

        // Party: the level's one tune, four bars, through however many walls
        partyG += (sc.party - partyG) * 0.01f;
        if (sc.party > 0.0f) {
            partyBps = sc.partyBpm / 60.0f;
            partyKey = sc.partyKey;
            partyPan.set((sc.partyDx * rightX + sc.partyDz * rightZ) * 0.7f);
        }

        // Drone: in drunken pockets the pitch sags and wows
        droneG += (sc.drone - droneG) * 0.006f;
        droneHz += (sc.droneHz - droneHz) * 0.004f;
        ratioA += (sc.ratioA - ratioA) * 0.004f;
        ratioB += (sc.ratioB - ratioB) * 0.004f;
        warble += (sc.warble - warble) * 0.01f;
        float wow = droneWow.next(rng, dt * 0.35f);
        float f = droneHz * (1.0f + warble * (0.030f * wow - 0.02f));
        drone[0].set(f); drone[1].set(f * ratioA * (1.0f + 0.012f * warble * wow));
        drone[2].set(f * ratioB); drone[3].set(f * 3.0f * (1.0f + 0.004f));
        for (int k = 0; k < 4; k++) drone[k].renorm();
        droneSwellNow = 0.7f + 0.3f * droneSwell.next(rng, dt * 0.08f);

        // Events: Poisson arrivals at the rates the place asks for
        if (stale < 0.6f) {
            if (rng.uni() < sc.dripRate * dt) startDrip();
            if (rng.uni() < sc.creakRate * dt) startCreak();
        }

        // Space
        reverb.tune(rt60, absorb, meanR, sc.airDull);
        sendSteps = 0.22f + 0.30f * (1.0f - absorb * 2.5f);
        for (int k = 0; k < 2; k++) {
            LsEchoTap& tap = taps[k];
            bool want = sc.echoGain[k] > 0.0f && sc.echoDelay[k] > 0.045f && stale < 0.6f;
            if (want) {
                int target = (int)(sc.echoDelay[k] * LS_SR);
                if (target > ECHO_LEN - 64) target = ECHO_LEN - 64;
                tap.dT = target;
            }
            // Never slide a delay (that would bend the pitch): fade out, jump, fade in.
            int diff = tap.dT - tap.d; if (diff < 0) diff = -diff;
            if (diff > 110 && want) {
                tap.g *= 0.80f;
                if (tap.g < 0.003f) {
                    tap.d = tap.dT;
                    tap.pan.set((sc.echoDx[k] * rightX + sc.echoDz[k] * rightZ) * 0.8f);
                }
            } else {
                tap.gT = want ? sc.echoGain[k] : 0.0f;
                tap.g += (tap.gT - tap.g) * 0.10f;
                if (want) tap.pan.set((sc.echoDx[k] * rightX + sc.echoDz[k] * rightZ) * 0.8f);
            }
        }
    }

    float gustNow = 0.5f, droneSwellNow = 0.8f;

    // Renders `frames` stereo frames and ADDS them to out (interleaved L/R floats).
    void render(float* out, int frames, const LsMix& mix) {
        for (int n = 0; n < frames; n++) {
            if (ctl <= 0) { control(mix); ctl = CONTROL; }
            ctl--;
            float L = 0.0f, R = 0.0f, wet = 0.0f, echoIn = 0.0f;

            // ---- lamps: all on one mains phase, each with its own harmonics and buzz
            mains2.tick(); mains4.tick(); mains6.tick();
            float lampL = 0.0f, lampR = 0.0f;
            for (int i = 0; i < LS_LAMPS; i++) {
                LsLampVoice& v = lamps[i];
                if (v.id == 0) continue;
                v.gate += (v.gateT - v.gate) * 0.02f;
                float h2 = mains2.s * v.cp + mains2.c * v.sp;
                float tone = v.w1 * h2 + v.w2 * (mains4.s * v.cq + mains4.c * v.sq) + v.w3 * (mains6.s * v.cr + mains6.c * v.sr);
                float pulse = 0.5f + 0.5f * h2;
                float x = (tone * 0.2f + v.bz.bp(rng.bi()) * v.buzz * pulse * pulse * pulse * 6.0f) * v.gate;
                if (v.tickT >= 0.0f) {
                    x += v.tk.bp(rng.bi() * expf(-v.tickT * 900.0f)) * 1.6f;
                    v.tickT += LS_DT;
                    if (v.tickT > 0.03f) v.tickT = -1.0f;
                }
                v.lpS += (x - v.lpS) * v.lpC;
                float y = (v.lpS * 0.8f + tone * 0.2f * v.gate * 0.35f) * v.g;
                lampL += y * v.pan.l; lampR += y * v.pan.r;
            }

            // ---- room tone: the air of the room excites the room's own modes
            float pl = roomPinkL.next(rng), pr = roomPinkR.next(rng);
            float roomL = (modeL[0].bp(pl) + modeL[1].bp(pl) * 0.8f) * roomGain * 0.018f;
            float roomR = (modeR[0].bp(pr) + modeR[1].bp(pr) * 0.8f) * roomGain * 0.018f;
            hissL += (pl - hissL) * hissC; hissR += (pr - hissR) * hissC;
            roomL += (pl - hissL) * hissGain * 0.0055f; roomR += (pr - hissR) * hissGain * 0.0055f;

            // ---- shaft draught: air moving in a bottomless well, heard from where the well is
            float draftL = 0.0f, draftR = 0.0f;
            if (voidG > 1e-4f) {
                brown = (brown + 0.03f * rng.bi()) * 0.995f;
                float air = howl.bp(brown) * 1.1f + howl2.bp(brown) * 0.35f;
                air *= voidG * gustNow * 0.22f;
                draftL = air * voidPan.l; draftR = air * voidPan.r;
                wet += air * 0.35f;
            }

            // ---- air plant: fan tone with blade wobble, and the duct it breathes through
            float plantS = 0.0f;
            fan1.tick(); fan2.tick(); fan3.tick();
            if (plantG > 1e-4f) {
                fanAm += (rng.bi() - fanAm) * 0.0006f;
                float fan = fan1.s * 0.55f + fan2.s * (0.50f - 0.25f * plantTilt) + fan3.s * 0.18f;
                float ductN = duct.lp(ductPink.next(rng));
                plantS = (fan * (0.028f + 0.25f * fanAm) * (1.15f - 0.6f * plantTilt) + ductN * (0.05f + 0.10f * plantTilt)) * plantG * 0.60f;
            }

            // ---- level drone
            float dr = 0.0f;
            for (int k = 0; k < 4; k++) drone[k].tick();
            if (droneG > 1e-4f) {
                dr = (drone[0].s * 0.55f + drone[1].s * 0.36f + drone[2].s * 0.30f + drone[3].s * 0.10f)
                   * droneG * droneSwellNow * 0.075f;
            }
            float droneR = (drone[0].c * 0.55f + drone[1].s * 0.36f + drone[2].c * 0.30f + drone[3].s * 0.10f)
                         * droneG * droneSwellNow * 0.075f;

            // ---- party: kick on the beat, bass on the off-beat, everything above 250 Hz left in the walls
            float pmL = 0.0f, pmR = 0.0f;
            if (partyG > 1e-4f) {
                partyBeat += LS_DT * partyBps;
                if (partyBeat >= 16.0f) partyBeat -= 16.0f;
                int step = (int)partyBeat;
                if (step != partyStep) {
                    static const signed char line[16] = {0, 0, 3, 0, 0, 0, 5, 3, 0, 0, 3, 0, 7, 5, 3, -2};
                    partyStep = step;
                    partyNote = partyKey * powf(2.0f, (float)line[step & 15] / 12.0f);
                    partyKickPh = 0.0f;
                }
                float tb = (partyBeat - (float)step) / partyBps;      // seconds since the beat
                partyKickPh += LS_TAU * (44.0f + 78.0f * expf(-tb * 30.0f)) * LS_DT;
                float kick = sinf(partyKickPh) * expf(-tb * 7.5f);
                float ob = tb - 0.5f / partyBps;
                partyBassPh += partyNote * LS_DT;
                if (partyBassPh >= 1.0f) partyBassPh -= 1.0f;
                float bass = ob >= 0.0f ? (2.0f * partyBassPh - 1.0f) * expf(-ob * 6.0f) : 0.0f;
                partyLp1 += (kick * 0.95f + bass * 0.55f - partyLp1) * 0.034f;
                partyLp2 += (partyLp1 - partyLp2) * 0.050f;
                float y = partyLp2 * partyG * 0.20f;
                pmL = y * partyPan.l; pmR = y * partyPan.r;
                wet += y * 0.6f;
            }

            // ---- events
            float evL = 0.0f, evR = 0.0f;
            for (int i = 0; i < DRIPS; i++) {
                if (drips[i].t < 0.0f) continue;
                float s = renderDrip(drips[i]);
                evL += s * drips[i].pan.l; evR += s * drips[i].pan.r;
                wet += s * 0.9f; echoIn += s * 0.6f;
            }
            for (int i = 0; i < CREAKS; i++) {
                if (creaks[i].type < 0) continue;
                float s = renderCreak(creaks[i]);
                evL += s * creaks[i].pan.l * 0.5f; evR += s * creaks[i].pan.r * 0.5f;
                wet += s * 1.3f; echoIn += s * 0.3f;
            }

            float ambG = mix.amb * world;
            float aL = (lampL + roomL + draftL + plantS + dr + evL + pmL) * ambG;
            float aR = (lampR + roomR + draftR + plantS + droneR + evR + pmR) * ambG;
            wet = (wet + (lampL + lampR) * 0.10f + plantS * 0.08f) * ambG;
            echoIn *= ambG;

            // ---- footsteps
            float stL = 0.0f, stR = 0.0f;
            for (int i = 0; i < STEPS; i++) {
                if (steps[i].t < 0.0f) continue;
                float s = renderStep(steps[i]) * mix.sfx;
                stL += s * steps[i].pan.l; stR += s * steps[i].pan.r;
                wet += s * sendSteps; echoIn += s;
            }
            L = aL + stL; R = aR + stR;

            // ---- the space answers: discrete wall echoes, then the diffuse tail
            echoBuf[echoW] = echoIn;
            float echoL = 0.0f, echoR = 0.0f;
            for (int k = 0; k < 2; k++) {
                LsEchoTap& tap = taps[k];
                if (tap.g < 1e-4f) continue;
                float x = echoBuf[(echoW - tap.d) & (ECHO_LEN - 1)];
                tap.lp += (x - tap.lp) * 0.22f;      // a wall and 2d of air: the echo comes back dull
                float e = tap.lp * tap.g;
                echoL += e * tap.pan.l; echoR += e * tap.pan.r;
                wet += e * 0.5f;
            }
            echoW = (echoW + 1) & (ECHO_LEN - 1);
            float wl, wr;
            reverb.tick(wet, wl, wr);
            L += echoL + wl; R += echoR + wr;

            power[LSL_LAMPS] += (double)(lampL * lampL + lampR * lampR) * 0.5;
            power[LSL_ROOM] += (double)(roomL * roomL + roomR * roomR) * 0.5;
            power[LSL_DRAFT] += (double)(draftL * draftL + draftR * draftR) * 0.5;
            power[LSL_PLANT] += (double)(plantS * plantS);
            power[LSL_DRONE] += (double)(dr * dr + droneR * droneR) * 0.5;
            power[LSL_EVENTS] += (double)(evL * evL + evR * evR) * 0.5;
            power[LSL_STEPS] += (double)(stL * stL + stR * stR) * 0.5;
            power[LSL_WET] += (double)(wl * wl + wr * wr) * 0.5;
            metered++;

            out[n * 2] += L; out[n * 2 + 1] += R;
        }
    }

    float layerRms(int k) const { return metered > 0 ? (float)sqrt(power[k] / (double)metered) : 0.0f; }
    void resetMeters() { for (int k = 0; k < LSL_COUNT; k++) power[k] = 0.0; metered = 0; }
};
