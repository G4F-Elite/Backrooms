R"(
 // Cell Ambient Occlusion + analytic dihedral corner darkening at floor/ceiling/beam junctions
 float wallCrease = 1.0;
 if (abs(N.y) < 0.5) {
  float dFloor = min(abs(fp.y), abs(fp.y + 2.85));
  float dCeil  = abs(fp.y - 4.5);
  wallCrease = (0.76 + 0.24 * smoothstep(0.0, 0.95, dFloor)) * (0.84 + 0.16 * smoothstep(0.0, 0.70, dCeil));
 }
 float ao = mix(1.0, cellAO, 0.78) * wallCrease;

 float ambVal = 0.132 * uMood.x * (1.0 - danger * 0.3);
 vec3 baseAmb = max(vec3(ambVal * 0.84) + uFogCol * 0.48, vec3(0.118 * uMood.x)) * tc * ao;
 vec3 skyDir = normalize(vec3(0.38, 0.82, 0.42));
 float hemi = 0.38 + 0.24 * clamp(N.y * 0.5 + 0.5, 0.0, 1.0) + 0.30 * max(dot(N, skyDir), 0.0)
            + 0.08 * max(dot(N, vec3(-0.65, 0.18, -0.72)), 0.0);
 float vertLum = (fp.y >= 0.0) ? (0.66 + 0.34 * smoothstep(0.0, 36.0, fp.y)) : max(exp(fp.y * 0.048), 0.46);
 vec3 megaAmb = tc * palAmbSmooth(palVal) * (0.56 * hemi * vertLum * mix(1.0, uMood.x, 0.6) * clamp(uMood.x * 2.2, 0.0, 1.0)) * ao;
 vec3 res = mix(baseAmb, megaAmb, mega);

 // 1. Near 16 point lights partitioned cleanly against far lightmap + crowd-normalized irradiance
 float nearDistFade = 1.0 - smoothstep(20.0, 38.0, dist);
 float quadAtt      = mix(0.042, 0.026, mega);
 float directGain   = mix(0.76, 0.94, mega) * uMood.y;
 vec3 viewDirW      = normalize(vp - fp);
 vec3 directIrr     = vec3(0.0);
 vec3 specAcc       = vec3(0.0);
 float lightScatter = farLight * 0.42;
 int lampCount = min(nl, nearDistFade < 0.001 ? 8 : 16);
 for(int i = 0; i < lampCount; i++) {
  vec3 toLight = lp[i] - fp;
  float d2 = dot(toLight, toLight);
  if (i < 8) lightScatter += 0.12 / (1.0 + d2 * 0.06);
  if (nearDistFade < 0.001) continue;
  float lightDistFromCam = length(lp[i] - vp);
  float fade = (1.0 - clamp((lightDistFromCam - 24.0) / 16.0, 0.0, 1.0)) * nearDistFade;
  if(fade < 0.001) continue;
  float invD = inversesqrt(max(d2, 0.0001));
  float d = d2 * invD;
  vec3 L = toLight * invD;
  float att = 1.0 / (1.0 + 0.11*d + quadAtt*d2);
  float df;
  if(isCeil && fp.y < 4.7) {
   float horizDist = length(vec2(toLight.x, toLight.z));
   float vertDist = abs(toLight.y);
   float spread = 1.0 / (1.0 + horizDist * 0.95);
   df = spread * 1.12 * (vertDist < 0.3 ? 1.55 : (vertDist < 1.0 ? 1.25 : 1.0));
   df += (1.0 / (1.0 + horizDist * 0.35)) * 0.20;
  } else {
   df = smoothstep(0.0, 0.52, max(dot(N, L), 0.0));
  }
  float fl = 1.0 + sin(tm*20.0 + float(i)*2.1) * 0.015 + sin(tm*30.0 + float(i)*3.7) * 0.05 * danger;
  vec3 lcol = lmLampCol(lp[i].xz);
  directIrr += lcol * (df * fl * att * directGain * fade);
  if (isFloor && uFogDens.z > 0.15) {
   vec3 H = normalize(L + viewDirW);
   float spec = pow(max(dot(N, H), 0.0), 16.0 + uFogDens.z * 28.0);
   specAcc += lcol * (spec * uFogDens.z * 0.14 * att * fade);
  }
 }
 directIrr = directIrr / (vec3(1.0) + directIrr * 0.28);
 res += tc * directIrr + specAcc / (vec3(1.0) + specAcc * 0.65);

 // 2. Unlimited 2D far-lamp lightmap (crossfaded cleanly with nearDistFade — no double counting)
 float farWeight = 1.0 - nearDistFade * 0.82;
 if (farLight > 0.005 && fp.y > -32.0) {
  float upFace = 0.60 + 0.40 * clamp(N.y * 0.6 + 0.5, 0.0, 1.0);
  float heightAtt = 1.0 - smoothstep(12.0, 42.0, abs(fp.y - 2.5));
  vec3 farCol = mix(uLightCol, palAmbSmooth(palVal), 0.32);
  res += tc * farCol * (farLight * 1.12 * uMood.y * upFace * heightAtt * ao) * farWeight;
 }
 if (N.y > 0.6 && fp.y < -2.70 && fp.y > -2.95) {
  res += uLightCol * 0.16 * farLight * (0.45 + 0.55 * ao);
 }

 if(flashOn == 1) {
  vec3 toFrag = normalize(fp - flashPos);
  float spotAngle = dot(toFrag, flashDir);
  float distF = length(fp - flashPos);
  float maxFD = mix(16.0, 36.0, mega);
  if(spotAngle > 0.85 && distF < maxFD) {
   float spotAtt = smoothstep(0.85, 0.95, spotAngle);
   float distAtt = 1.0 - distF / maxFD;
   float NdotL = max(dot(N, -toFrag), 0.0);
   vec3 flashColor = mix(vec3(1.0, 0.99, 0.92), vec3(1.0, 0.3, 0.2), danger * 0.6);
   res += tc * flashColor * spotAtt * distAtt * NdotL * 1.5;
  }
 }
 for(int i = 0; i < rfc && i < 4; i++) {
  vec3 toFrag = normalize(fp - rfp[i]);
  float spotAngle = dot(toFrag, rfd[i]);
  float distR = length(fp - rfp[i]);
  if(spotAngle > 0.88 && distR < 13.0) {
   res += tc * vec3(0.9, 0.95, 1.0) * smoothstep(0.88, 0.96, spotAngle) * (1.0 - distR / 13.0) * max(dot(N, -toFrag), 0.0) * 0.95;
  }
 }
 if(isCeil && fp.y < 4.7) {
  float maxB = max(res.r, max(res.g, res.b));
  if(maxB > 0.1) res = max(res, tc * 0.14 * ao);
 }

 // --- quirks that relight the world
 if (skyOn) {                                                    // open sky: hemisphere light, and a sun/moon without shadows
  int sk = int(uQ(5).x + 0.5) - 1;
  float open = 1.0 - 0.85 * smoothstep(0.35, 0.9, env.a);
  float kAmb = (sk <= 1) ? 0.62 : ((sk == 3) ? 0.34 : ((sk == 2) ? 0.15 : 0.06));
  vec3 skyAmb = mix(uSkyHor, uSkyTop, 0.45);
  res += tc * skyAmb * (0.32 + 0.68 * clamp(N.y * 0.5 + 0.5, 0.0, 1.0)) * (kAmb * open) * mix(1.0, cellAO, 0.55);
  vec3 sunDir = (sk == 3) ? normalize(vec3(0.80, 0.16, 0.55)) : ((sk == 2) ? normalize(vec3(-0.42, 0.52, 0.74)) : normalize(vec3(0.42, 0.78, 0.36)));
  vec3 sunCol = (sk == 0) ? vec3(0.10) : ((sk == 1) ? vec3(0.52, 0.50, 0.44) : ((sk == 3) ? vec3(0.62, 0.34, 0.16) : ((sk == 2) ? vec3(0.07, 0.08, 0.11) : vec3(0.0))));
  res += tc * sunCol * max(dot(N, sunDir), 0.0) * open * mix(1.0, cellAO, 0.7);
 }
 if (lavaKind > 0) {                                             // the floor is the lamp
  float hgt = max(fp.y, 0.0) + max(-2.85 - fp.y, 0.0);
  if (fp.y < -0.1 && fp.y > -2.9) hgt = fp.y + 2.85;
  float face = isFloor ? (onGround ? 0.0 : 0.10) : (isCeil ? 1.0 : 0.62);
  float kUp = (lavaKind == 3) ? 1.05 : 0.60;
  float pulse = (lavaKind == 3) ? 1.0 : 0.85 + 0.15 * sin(tm * 0.9 + (fp.x + fp.z) * 0.05);
  res += tc * uQ(6).rgb * (face * kUp * pulse * exp(-hgt * 0.24) * (1.0 - 0.7 * smoothstep(0.4, 1.0, env.a)));
 }
 res += emis;

 // Flood: there is no water mesh. Every fragment below the surface is seen through it: the ray
 // is cut at the plane, the stretch below is absorbed, and the plane itself mirrors the ceiling
 // light above the hit point (from the lamp map), the lamps (glints) or the sky.
 float fogDist = dist;
 if (uQ(0).x > 0.5) {
  float wy = uQ(0).y;
  vec3 wcol = uQ(1).rgb;
  float murk = uQ(0).z;
  vec3 rd = fp - vp;
  if (vp.y > wy) {
   if (fp.y < wy) {
    float tW = (wy - vp.y) / rd.y;
    vec3 sp = vp + rd * tW;
    float sdist = length(sp - vp);
    float path = length(fp - sp);
    float rf = 1.0 - smoothstep(8.0, 46.0, sdist);
    vec2 w2 = sp.xz;
    vec2 gr = vec2(
     cos(w2.x * 1.9 + tm * 1.1) * 0.5 + cos((w2.x + w2.y) * 3.1 - tm * 1.7) * 0.3 + cos(w2.x * 7.3 + w2.y * 2.1 + tm * 2.3) * 0.14,
     cos(w2.y * 2.3 - tm * 0.9) * 0.5 + cos((w2.x + w2.y) * 3.1 - tm * 1.7) * 0.3 + cos(w2.y * 6.7 - w2.x * 1.7 - tm * 2.0) * 0.14);
    vec3 wn = normalize(vec3(-gr.x * 0.055 * rf, 1.0, -gr.y * 0.055 * rf));
    vec3 vd = normalize(-rd);
    float fres = 0.025 + 0.975 * pow(1.0 - max(dot(vd, wn), 0.0), 5.0);
    float kAbs = mix(0.36, 2.5, murk);
    vec3 trans = exp(-path * kAbs * (vec3(1.06) - wcol * 0.92));
    float ca = 0.0;
    if (N.y > 0.5) {
     vec2 cq = fp.xz * 1.3;
     float c1 = sin(cq.x * 2.1 + tm * 0.8 + sin(cq.y * 1.7 + tm * 0.6) * 1.4);
     float c2 = sin(cq.y * 2.4 - tm * 0.7 + sin(cq.x * 1.9 - tm * 0.5) * 1.4);
     ca = pow(clamp(0.5 + 0.5 * c1 * c2, 0.0, 1.0), 4.0) * (1.0 - murk) * exp(-(wy - fp.y) * 0.5);
    }
    vec3 lightHere = vec3(0.075 * uMood.x) + uLightCol * (farLight * 0.42) + directIrr * 0.40;
    if (skyOn) lightHere += mix(uSkyHor, uSkyTop, 0.5) * 0.35;
    vec3 under = res * (1.0 + ca * 1.5) * trans + wcol * lightHere * (1.0 - exp(-path * kAbs * 0.7));
    vec3 R = reflect(-vd, wn);
    vec3 refl;
    if (skyOn && env.a < 0.5) {
     refl = lmSky(normalize(vec3(R.x, max(R.y, 0.02), R.z)));
    } else {
     vec2 hit = sp.xz + R.xz * ((4.4 - wy) / max(R.y, 0.10));
     float hl = texture(envMap, clamp((hit - envRect.xy) * envRect.z, 0.004, 0.996)).r;
     refl = max(uFogCol, vec3(0.025)) * 1.1 + uLightCol * (hl * 0.50) + vec3(0.05 * uMood.x);
    }
    vec3 glint = vec3(0.0);
    for (int i = 0; i < nl && i < 16; i++) {
     vec3 Lw = lp[i] - sp;
     float ld = length(Lw);
     float s = pow(max(dot(R, Lw / ld), 0.0), 420.0);
     glint += lmLampCol(lp[i].xz) * (s * 2.6 / (1.0 + ld * 0.03));
    }
    res = mix(under, refl, fres) + glint * (0.35 + 0.65 * fres);
    fogDist = sdist;
   } else if (abs(N.y) < 0.5 && fp.y < wy + 0.24) {
    res *= mix(0.70, 1.0, smoothstep(wy, wy + 0.24, fp.y));       // tide line
   }
  } else {
   // the camera is under the surface (walking the bottom of what used to be a dry basin)
   float path = dist;
   if (fp.y > wy) path = dist * (wy - vp.y) / max(fp.y - vp.y, 0.001);
   float kAbs = mix(0.10, 0.85, murk);
   vec3 trans = exp(-path * kAbs * (vec3(1.06) - wcol * 0.92));
   vec3 lightHere = vec3(0.10 * uMood.x) + uLightCol * (farLight * 0.40);
   vec2 cq = fp.xz * 1.1;
   float ca = pow(clamp(0.5 + 0.5 * sin(cq.x * 2.1 + tm * 0.8 + sin(cq.y * 1.7 + tm * 0.6) * 1.4) * sin(cq.y * 2.4 - tm * 0.7), 0.0, 1.0), 3.0);
   res = res * (1.0 + ca * 0.9 * step(fp.y, wy) * max(N.y, 0.0)) * trans + wcol * lightHere * (1.0 - exp(-path * kAbs * 0.9));
  }
 }

 // Continuous DNA-driven world-space fog (sized so megastructures keep crisp silhouettes!)
 float normFogD = mix(uFogDens.x * 1.18, uFogDens.x * 0.84, smoothstep(0.0, 2.5, fp.y));
 float pitExtra = 1.0 - smoothstep(-52.0, -16.0, fp.y);
 float megaFogD = uFogDens.y + 0.024 * pitExtra;
 // A slowly moving density field stays continuous across neighbouring fragments.
 // Varying extinction leaves surfaces next to the camera clear.
 float fogDensity = 1.0 + 0.09 * sin(dot(fp, vec3(0.11, 0.04, 0.06)) + tm * 0.025)
                             * cos(dot(fp, vec3(-0.04, 0.07, 0.09)) - tm * 0.017);
 float fog = exp(-fogDist * mix(normFogD, megaFogD, mega) * fogDensity * (skyOn ? 0.45 : 1.0));
 fog = max(fog, mega * 0.18 * (1.0 - pitExtra * 0.7));

 lightScatter = min(lightScatter, 0.68);

 vec3 normFogCol = max(uFogCol, vec3(0.064, 0.060, 0.068)) + uLightCol * (0.065 * lightScatter);
 vec3 highHaze   = palFogSmooth(palVal) + uLightCol * (0.080 * lightScatter);
 vec3 abyssHaze  = max(uFogCol * 0.68, vec3(0.065, 0.068, 0.078));
 vec3 fogColor   = mix(normFogCol, mix(highHaze, abyssHaze, pitExtra), mega);

 float desat = smoothstep(6.0, 24.0, dist) * (1.0 - mega * 0.55);
 float gray = dot(res, vec3(0.3, 0.6, 0.1));
 res = mix(res, vec3(gray) * mix(vec3(1.0), normalize(uLightCol + vec3(0.35)), 0.25), desat * 0.30);
 float distDarken = smoothstep(14.0, 28.0, dist) * (1.0 - mega * 0.92);
 res *= (1.0 - distDarken * 0.18);

 if (!skyOn) res = mix(fogColor, res, fog);
 if (uUnstruct.x > 0.5) {
  int uf = int(uUnstruct.x + 0.5);
  vec3 uRes = tc * 0.25;
  vec3 uFogCol = vec3(0.5);
  float uFogDens = 0.014;
  float blk = (uQ(5).y > 0.5) ? 0.07 : 1.0;
  if (uf == 1) {
   float upN = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
   float sideN = 0.75 + 0.25 * abs(N.x) + 0.15 * abs(N.z);
   float hGrad = 0.62 + 0.38 * smoothstep(0.0, 32.0, fp.y);
   uRes = tc * vec3(0.56, 0.58, 0.61) * (0.48 + 0.52 * upN) * sideN * hGrad * ao * blk;
   uFogCol = vec3(0.67, 0.69, 0.71) * blk;
   uFogDens = 0.0135;
  } else if (uf == 2) {
   vec3 sunD = normalize(vec3(0.35, 0.85, 0.30));
   float hemiU = 0.52 + 0.38 * clamp(N.y * 0.5 + 0.5, 0.0, 1.0) + 0.22 * max(dot(N, sunD), 0.0);
   uRes = tc * vec3(0.66, 0.71, 0.84) * hemiU * ao * blk;
   uFogCol = vec3(0.41, 0.46, 0.61) * blk;
   uFogDens = 0.0145;
  } else if (uf == 3) {
   float radC = length(fp.xz - uUnstructInfo.xz);
   float shaft = exp(-radC * radC / 145.0);
   float roofRim = smoothstep(uUnstructInfo.y - 3.0, uUnstructInfo.y + 4.0, fp.y);
   float hemiG = 0.44 + 0.56 * clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
   vec3 caveAmb = tc * vec3(0.34, 0.30, 0.40) * hemiG * ao;
   vec3 skyBeam = tc * vec3(0.58, 0.64, 0.76) * (0.45 + 0.55 * clamp(N.y, 0.0, 1.0)) * shaft * (0.72 + 0.28 * ao);
   vec3 rimSun  = tc * vec3(0.76, 0.74, 0.72) * (0.45 + 0.55 * clamp(dot(N, normalize(vec3(0.3, 0.7, 0.4))), 0.0, 1.0)) * roofRim;
   uRes = (caveAmb + skyBeam + rimSun) * blk;
   uFogCol = vec3(0.30, 0.28, 0.35) * blk;
   uFogDens = 0.0125;
  } else if (uf == 4) {
   vec3 sunD = normalize(vec3(0.66, 0.64, 0.40));
   float ndl = max(dot(N, sunD), 0.0);
   float upN = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
   vec3 lightT = vec3(0.36, 0.44, 0.60) * (0.55 + 0.45 * upN) + vec3(0.78, 0.64, 0.50) * ndl + vec3(0.16, 0.14, 0.12) * (1.0 - upN);
   if (uUnstruct.y > 0.5) lightT *= 1.18;
   uRes = tc * lightT * mix(0.84, 1.0, ao) * blk;
   uFogCol = mix(vec3(0.78, 0.85, 0.96), vec3(0.88, 0.84, 0.80), 0.20) * blk;
   uFogDens = 0.0075;
  }
  if (flashOn == 1) {
   vec3 toFrag = normalize(fp - flashPos);
   float spotAngle = dot(toFrag, flashDir);
   float distF = length(fp - flashPos);
   if (spotAngle > 0.85 && distF < 36.0) {
    uRes += tc * vec3(1.0, 0.99, 0.92) * smoothstep(0.85, 0.95, spotAngle) * (1.0 - distF / 36.0) * max(dot(N, -toFrag), 0.0) * 1.4;
   }
  }
  float uFog = clamp(exp(-fogDist * uFogDens), 0.0, 1.0);
  res = mix(uFogCol, uRes, uFog);
  if (uf == 2 && uQ(5).y < 0.5) {
   float sn = lmSnowFlakes(normalize(fp - vp), dist, tm);
   res = mix(res, vec3(0.90, 0.92, 0.96), sn);
  }
 }
 if(danger > 0.3) { res.r += danger * 0.08; res.gb *= (1.0 - danger * 0.15); }
 if(danger > 0.0) { float gD = dot(res, vec3(0.3, 0.6, 0.1)); res = mix(res, vec3(gD), danger * 0.2); }

 // Midtone-preserving exposure + smooth filmic highlight shoulder (prevents bleached >250 surfaces)
 res *= 1.38;
 float lumOut = max(dot(res, vec3(0.299, 0.587, 0.114)), 0.0001);
 float lumTonemapped = (lumOut <= 0.50)
  ? lumOut
  : (0.50 + (lumOut - 0.50) / (1.0 + (lumOut - 0.50) * 2.25));
 res *= lumTonemapped / lumOut;
 res = clamp(res, 0.0, 0.945);
 if (skyOn) {
  // far geometry dissolves into the very sky that stands behind it
  vec3 sd = normalize(fp - vp);
  res = mix(lmSky(normalize(vec3(sd.x, max(sd.y, 0.0) * 0.35, sd.z))), res, fog);
 }
 if (uQ(5).z > 0.5) {                                            // 15-bit colour, ordered dither
  float b = fract(dot(floor(gl_FragCoord.xy), vec2(0.5, 0.75)));
  res = floor(res * 31.0 + b) / 31.0;
 }
 F = vec4(res, 1.0);
})"
