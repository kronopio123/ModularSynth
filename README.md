# Modular Synth

Sintetizzatore modulare polifonico, JUCE-based, compilato come app **Standalone**
(suona il MIDI da solo, senza DAW) e anche come **VST3** se preferisci caricarlo
in un host.

## Architettura (catena fissa + matrice di modulazione)

```
Osc 1 ─┐
       ├─ Mix ─ Filtro (LP/BP/HP) ─ Amp Env (ADSR) ─ Out
Osc 2 ─┘

Mod Env (ADSR) ──┐
LFO (4 forme)  ──┴─ Matrice (3 slot) ─→ Osc1 Pitch / Osc2 Pitch / Filter Cutoff / Osc Mix
```

- **2 oscillatori** (sine/saw/square/triangle), livello e accordatura indipendenti (±24 semitoni)
- **1 filtro** multimodo (Low/Band/High pass), cutoff e risonanza
- **2 inviluppi ADSR**: uno per l'ampiezza, uno dedicato alla modulazione
- **1 LFO** (4 forme d'onda, rate 0.01–20 Hz)
- **Matrice di modulazione a 3 slot**: ogni slot sceglie sorgente (Mod Env / LFO),
  destinazione (pitch osc1, pitch osc2, cutoff filtro, bilanciamento mix) e profondità (-1..1)
- **Polifonia**: 16 voci simultanee

## Build (cloud, senza toolchain locale)

Il workflow `.github/workflows/build.yml` scarica JUCE via `FetchContent` e compila
in automatico su Windows/macOS/Linux ad ogni push su `main`. Gli eseguibili
Standalone (uno per piattaforma) vengono caricati come artifact scaricabili
dalla pagina "Actions" del repository — stesso schema usato per il plugin
dell'ampli Coverplay.

Per farlo partire:
1. Crea un repo GitHub e pusha questa cartella
2. Vai su "Actions" → il workflow parte da solo → scarica l'artifact per il tuo OS

## Build locale (opzionale)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

L'eseguibile Standalone si trova in
`build/ModularSynth_artefacts/Release/Standalone/`.

## Note

- Gli oscillatori sono "naive" (non band-limited): a note molto acute puoi sentire
  un po' di aliasing. Se ti serve un suono più pulito in zona alta, si può
  aggiungere oversampling o oscillatori PolyBLEP in un secondo passaggio.
- La modulazione viene ricalcolata a livello di blocco audio (non sample-accurate),
  scelta comune nei synth semplici per stabilità/performance.
