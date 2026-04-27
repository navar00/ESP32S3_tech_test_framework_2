---
name: git-workflow
description: "Use when committing, pushing, tagging, or releasing in ESP32S3-TFT Framework: writing commit messages, validating pre-commit hygiene, recovering from mistakes, or executing a release (commit + tag + push) on top of iteration-close. Triggers: 'commit', 'git push', 'mensaje de commit', 'qué subir', 'convención de commits', 'pre-commit', 'tag', 'release', 'cherry-pick', 'amend', 'undo last commit', 'revertir commit'."
---

# Skill: Git Workflow (ESP32S3-TFT Framework)

## Cuándo aplica
- Crear un commit (diario, intermedio, o de cierre de iteración).
- Empujar a `origin/main`.
- Crear un tag de versión y publicarlo.
- Recuperarse de errores (commit con secret, push equivocado, mensaje mal escrito).

## Modelo del repositorio
- **Branch único:** `main` (sin feature branches, sin PRs).
- **Remote:** `origin` → GitHub público (`navar00/ESP32S3_tech_test_framework_2`).
- **Tags:** semver `v0.MINOR.PATCH` (proyecto sandbox pre-1.0).
- **Convención de commits:** Conventional Commits relajado, mixto ES/EN.

## Convención de commit message

```
<type>(<scope>): <subject in EN, lowercase, ≤72 chars>

<body opcional en ES o EN, líneas ≤100 chars>
<separación de ideas con líneas en blanco>

<footer opcional: refs, breaking changes>
```

### Types permitidos
| Type | Cuándo |
|---|---|
| `feat` | Funcionalidad nueva visible al usuario o a otra capa. |
| `fix` | Corrige un bug (incluir síntoma + causa raíz en body si no es trivial). |
| `refactor` | Reorganiza sin cambiar comportamiento observable. |
| `perf` | Mejora rendimiento medible (incluir números). |
| `docs` | Solo `README.md`, `DevGuidelines.md`, skills, comentarios. |
| `chore` | Limpieza, dependencias, mover archivos a `aux_/`. |
| `build` | `platformio.ini`, `build.ps1`, `upload.ps1`, scripts de build. |
| `style` | Formateo, whitespace, sin cambios semánticos. |
| `revert` | Revierte un commit anterior (incluir SHA en body). |

### Scopes canónicos del proyecto
`hal`, `core`, `services`, `screens`, `web`, `ble`, `time`, `geo`, `net`, `wdt`, `gol`, `palettes`, `boot`, `build`, `docs`, `scaffold`, `skills`. Combinar dos con `,` solo si el cambio es genuinamente cross-cutting (`feat(web,gol): …`).

### Reglas duras del subject
- Imperativo presente: `add`, `fix`, `refactor` (no `added`, no `adds`).
- Sin punto final.
- Sin mayúscula inicial tras el `:`.
- Mencionar el “qué”, no el “cómo”.

### Ejemplos válidos (estilo del repo)
```
feat(web): palette color preview with frequency ranking, aux_ scripts reorganization
fix(screens): blackscreen on ScreenAnalogClock after NTP resync — sprite reused tras delete
refactor(hal): collapse Display/Led HAL into singleton facade
build: relocate logs to <BuildDir>/.logs with rotation + _latest alias
docs(skills): add git-workflow + cross-link from iteration-close
chore: archive PROMPT_*.md legacy prompts into aux_/legacy_prompts/
```

### Body recomendado (cuando aplica)
```
fix(web): payload truncation on /api/palettes over 5 KB

Síntoma: ERR_CONNECTION_RESET intermitente al cargar la pestaña Palettes.
Causa raíz: send_P() supera la ventana TCP del ESP32-S3 (~5744 B) cuando
PALETTES_JSON crece > 5 KB. Solución: WiFiClient backpressure (chunks 256 B,
50 retries, delay(5) + yield()).

Refs: README §6.11, skill webservice-http.
```

## Pre-commit checklist
Ejecutar **antes** de `git add`:

1. **Build limpio**: `./build.ps1` → `SUCCESS`, **cero warnings**.
2. **No hay secrets ni config personal**:
   ```powershell
   git status --short | Select-String 'Config\.h$|\.env|secret|password'
   ```
   Si aparece algo → STOP. `Config.h` está en `.gitignore` por una razón.
3. **No hay logs ni binarios accidentales**:
   ```powershell
   git status --short | Select-String '\.log$|\.txt$|\.bin$|\.elf$|\.hex$|\.pio/'
   ```
4. **Diff revisado**: `git diff --staged --stat` y luego `git diff --staged` para ver el contenido real. Nunca commit a ciegas.
5. **Mensaje preparado**: redactar antes de teclear `git commit`, no improvisar.
6. **Encoding del mensaje**: si lleva acentos, em-dashes o caracteres no-ASCII en Windows/PowerShell, usar el patrón "mensaje desde fichero UTF-8" (ver sección *Encoding del mensaje* más abajo). Pasar `-m "áéí"` por la terminal corrompe los bytes.

## Flujo de commit diario

```powershell
git status
git diff --stat                              # vista panorámica
git add -A                                   # o paths específicos si hay ruido
git diff --staged                            # confirmar exactamente qué entra
git commit -m "feat(screens): add ScreenSpectrum with FFT 256 bins"
git push                                     # main → origin/main
```

Si el cambio es **multi-aspecto** (p.ej. fix + refactor + docs en distintos archivos):
- **Commits separados** por aspecto, no un mega-commit.
- Usa `git add -p` (interactive) o `git add <ruta>` para fragmentar.

## Encoding del mensaje (Windows / PowerShell)

PowerShell por defecto codifica los argumentos de línea de comandos en CP-850 (Western
European), no UTF-8. Si pasas un mensaje con `áéíóúñü` vía `git commit -m "..."` o here-string
`@"..."@`, los caracteres no-ASCII llegan corrompidos al objeto git.

**Síntomas:**
- `git log` muestra `Iteraci├│n` en lugar de `Iteración`, `ÔÇö` en lugar de `—`.
- En GitHub se ve igual de roto (los bytes guardados son ya CP-850, no UTF-8).

**Patrón robusto — mensaje desde fichero UTF-8:**
```powershell
# 1) Escribir el mensaje a fichero con BOM-less UTF-8 explícito.
$msg = @'
feat(scope): título con acentos

Cuerpo con caracteres especiales: áéíóúñ, em-dash —, comillas «».
'@
[System.IO.File]::WriteAllText(
    "$PWD\.git\COMMIT_MSG_TMP.txt",
    $msg,
    (New-Object System.Text.UTF8Encoding $false)   # $false = sin BOM
)

# 2) Commit forzando encoding UTF-8 a nivel del propio comando.
git -c i18n.commitEncoding=UTF-8 commit -F .git/COMMIT_MSG_TMP.txt
Remove-Item .git\COMMIT_MSG_TMP.txt
```

**Verificar el encoding tras commitear** (los bytes reales, no el render de la consola):
```powershell
git cat-file -p HEAD > raw_commit.bin
$bytes = [System.IO.File]::ReadAllBytes("$PWD\raw_commit.bin")
# Buscar un acento conocido y comprobar que aparece como C3 B3 (ó) o C3 A1 (á):
# `ó` UTF-8 = C3 B3   `á` UTF-8 = C3 A1   `—` em-dash UTF-8 = E2 80 94
Remove-Item raw_commit.bin
```
Si ves `B3` solo (sin el prefijo `C3`) o `97` solo (em-dash en CP-1252), el mensaje está
en encoding errado: hacer `git commit --amend -F ...` con el patrón robusto antes de push.

**Configuración persistente (one-shot):**
```powershell
git config --global i18n.commitEncoding utf-8
git config --global i18n.logOutputEncoding utf-8
```
Todavía hay que pasar el mensaje como fichero UTF-8 (PowerShell sigue corrompiendo `-m`),
pero al menos `git log` lo decodifica bien al leerlo de vuelta.

**Truco rápido para PowerShell 7+:** `[Console]::OutputEncoding = [Text.Encoding]::UTF8`
antes de `git log` mejora el render en pantalla, no afecta al storage.

## Tagging y release

Solo tras `iteration-close`:

```powershell
# 1. Pre-flight de iteration-close ya pasado (build + smoke + changelog + docs sync).
# 2. Commit final del cierre:
git add -A
git commit -m "chore: close iteration v0.MINOR.PATCH — <resumen 1 línea>"

# 3. Tag anotado (no lightweight) con el resumen del changelog:
git tag -a v0.MINOR.PATCH -m "v0.MINOR.PATCH — <título de la entrada del changelog>"

# 4. Push commit + tag explícitamente:
git push
git push origin v0.MINOR.PATCH
```

### Release combinada (commit + tag + push) en un paso
Cuando los pasos 1-2 de `iteration-close` ya están confirmados:
```powershell
$ver = "v0.X.Y"
$msg = "<resumen breve>"
git add -A
git commit -m "chore: close iteration $ver — $msg"
git tag -a $ver -m "$ver — $msg"
git push
git push origin $ver
```

**Nunca** `git push --tags` (sube tags antiguos no publicados intencionadamente).

## Recovery patterns

### Cambiar el mensaje del último commit (no pusheado)
```powershell
git commit --amend -m "<nuevo mensaje>"
```

### Añadir un fichero olvidado al último commit (no pusheado)
```powershell
git add <fichero>
git commit --amend --no-edit
```

### Deshacer el último commit conservando los cambios (no pusheado)
```powershell
git reset --soft HEAD~1
```

### Revertir un commit ya pusheado (sin reescribir historia)
```powershell
git revert <sha>
git push
```

### Borrar un tag mal creado (local + remoto)
```powershell
git tag -d v0.X.Y
git push origin :refs/tags/v0.X.Y
```

### Secret commiteado por error
1. **NO hacer simplemente otro commit** — el secret queda en historia.
2. Rotar/invalidar el secret inmediatamente en el sistema origen.
3. Si el commit no se ha pusheado: `git reset --soft HEAD~1`, quitar el fichero, recomprometer.
4. Si ya se pusheó: usar `git filter-repo` o BFG, **avisar al usuario antes** (reescribe historia pública).

## Antipatrones (PROHIBIDOS)

- `git push --force` o `git push -f` sobre `main`. Nunca.
- `git commit --amend` sobre commits **ya pusheados**.
- `git push --no-verify` para saltarse hooks (cuando se añadan).
- Mega-commit `wip` o `update` sin descripción.
- Mezclar refactor + feat + fix en un commit.
- Commitear `Config.h`, `*.log`, `.pio/`, `node_modules/`, binarios > 1 MB.
- `git rebase -i` interactivo sobre commits pusheados.
- Crear feature branches sin razón (proyecto es main-only).
- Pasar mensajes con caracteres no-ASCII vía `git commit -m "..."` en PowerShell sin
  fichero UTF-8 ni `-c i18n.commitEncoding=UTF-8` (rompe encoding en GitHub).
- Hacer `git tag` antes de que CI haya validado el commit en `main`.

## Comandos de inspección útiles
```powershell
git log --oneline -20                    # últimos 20 commits
git log --stat -5                        # con archivos cambiados
git log --grep "feat(web)" --oneline     # filtrar por scope
git log v0.1.0..HEAD --oneline           # cambios desde un tag
git diff HEAD~1 -- src/services/         # qué cambió en services
git blame src/core/Config.h              # autor por línea
git show <sha>                           # commit completo
```

## Post-push: validar CI

Tras `git push origin main`, GitHub Actions arranca el workflow `.github/workflows/ci.yml`
(jobs `build`, `test`, `check`).

- **Esperar verde antes de tag.** Un push que rompe CI no merece tag de release; el tag
  apunta a un commit que el CI rechaza.
- **Dónde mirar:** `https://github.com/navar00/ESP32S3_tech_test_framework_2/actions`.
- **CLI opcional** (si está instalado `gh`):
  ```powershell
  gh run watch                              # último run en vivo
  gh run list --branch main --limit 5      # historial reciente
  gh run view --log-failed                  # logs del job que falló
  ```
- Si CI falla por algo no relacionado con el código (cache corrupta, GitHub flake), volver
  a disparar con `gh workflow run ci.yml` o desde la UI ("Re-run failed jobs").
- Si CI falla por código: hacer un commit `fix(ci): ...` que repare; **no** `--amend` ni
  `--force` sobre el commit ya pusheado.

## Cross-references
- **`iteration-close`**: ejecuta antes que esta skill cuando hay tag/release. Esta skill encadena el commit+tag+push final.
- **`build-pipeline`**: el paso 1 del checklist (build limpio) usa la pipeline definida ahí.
- **`.gitignore`**: la fuente de verdad de qué NO se commitea (`Config.h`, `*.log`, `.pio/`, `_port_backup_*/`).
- **`.github/workflows/ci.yml`**: gates automáticos que deben pasar tras cada push a `main`.
