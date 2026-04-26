---
name: git-workflow
description: "Use when committing, pushing, tagging, or releasing in TechTest v2: writing commit messages, validating pre-commit hygiene, recovering from mistakes, or executing a release (commit + tag + push) on top of iteration-close. Triggers: 'commit', 'git push', 'mensaje de commit', 'qué subir', 'convención de commits', 'pre-commit', 'tag', 'release', 'cherry-pick', 'amend', 'undo last commit', 'revertir commit'."
---

# Skill: Git Workflow (TechTest v2)

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

## Cross-references
- **`iteration-close`**: ejecuta antes que esta skill cuando hay tag/release. Esta skill encadena el commit+tag+push final.
- **`build-pipeline`**: el paso 1 del checklist (build limpio) usa la pipeline definida ahí.
- **`.gitignore`**: la fuente de verdad de qué NO se commitea (`Config.h`, `*.log`, `.pio/`, `_port_backup_*/`).
