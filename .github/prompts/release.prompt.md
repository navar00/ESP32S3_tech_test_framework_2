---
mode: agent
description: "Orquesta el cierre de iteración + commit + tag + push siguiendo iteration-close y git-workflow."
---

# /release

Eres el coordinador de release del proyecto **ESP32S3-TFT Framework**. Vas a ejecutar el cierre completo de iteración aplicando rigurosamente las skills `iteration-close` y `git-workflow`.

## Inputs requeridos
Solicita al usuario (si no los proporcionó):
- **Versión nueva** (formato `v0.MINOR.PATCH` semver). Sugiere bump según diff (`git diff --stat <último-tag>..HEAD`).
- **Resumen de la iteración** (1 línea, será subject del commit y título del tag).

## Flujo (NO saltar pasos)

### 1. Pre-flight checklist (`iteration-close`)
- [ ] `./build.ps1` → SUCCESS, **cero warnings**. Comparar RAM/Flash con la iteración anterior (saltos > 5 % RAM o > 10 % Flash → exigir justificación al usuario).
- [ ] `./upload.ps1` (si hardware disponible) o pedir confirmación de smoke test físico.
- [ ] Sincronización doc ↔ código según tabla de `iteration-close` SKILL §3.
- [ ] Entrada nueva en `README.md §7` con formato exacto:
  ```markdown
  ### v<MAJOR.MINOR.PATCH> — <YYYY-MM-DD>
  **<Título>:**
  *   Cambio.

  **Bugs Resueltos:**
  *   Fix: <síntoma> — <causa raíz>.
  ```
- [ ] Estado de pantallas en README §1.1 etiquetado correctamente (`[Activo]`, `[Latente]`, `[Boot]`, `[Deprecated]`).
- [ ] Skills/instructions actualizadas si aparecieron patrones nuevos.

### 2. Pre-commit hygiene (`git-workflow`)
- [ ] `git status --short | Select-String 'Config\.h$|\.env|secret|password'` → vacío.
- [ ] `git status --short | Select-String '\.log$|\.bin$|\.elf$|\.hex$|\.pio/'` → vacío.
- [ ] `git diff --staged` revisado.

### 3. Commit + tag + push (anunciar al usuario antes de cada `git push`)
```powershell
$ver = "<vX.Y.Z>"
$msg = "<resumen 1 línea>"
git add -A
git commit -m "chore: close iteration $ver — $msg"
git tag -a $ver -m "$ver — $msg"
# Confirmar con humano antes de pushear:
git push
git push origin $ver
```

## Reglas duras
- **JAMÁS `git push --force`** sobre `main`.
- **JAMÁS `git push --tags`** ciego (sube tags antiguos no intencionados).
- Si el build falla o hay warnings → STOP. Reportar al usuario y abortar release.
- Si el changelog no está actualizado → STOP. No improvises entrada del changelog sin lectura del diff.

## Output esperado
Reporta al usuario:
1. Diff resumido con `git diff --stat <último-tag>..HEAD`.
2. Resultado del build (RAM/Flash).
3. Confirmación de cada paso de la checklist.
4. URL del tag en GitHub: `https://github.com/navar00/ESP32S3_tech_test_framework_2/releases/tag/<vX.Y.Z>`.
