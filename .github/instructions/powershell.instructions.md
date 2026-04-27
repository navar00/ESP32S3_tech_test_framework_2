---
applyTo: "**/*.ps1"
description: "PowerShell conventions for build/upload scripts in ESP32S3-TFT Framework."
---

# PowerShell — reglas locales

- En cadenas dobles, usar `${var}:` no `"$var:"` (parser-safe).
- `Set-StrictMode -Version Latest` + `$ErrorActionPreference = 'Stop'` en scripts no triviales.
- Si usas `[CmdletBinding(SupportsShouldProcess=$true)]`, blindar cmdlets que NO deben respetar `-WhatIf`:
  ```powershell
  $PSDefaultParameterValues['Set-Content:WhatIf'] = $false
  $PSDefaultParameterValues['Add-Content:WhatIf'] = $false
  $PSDefaultParameterValues['Out-File:WhatIf']    = $false
  $PSDefaultParameterValues['Copy-Item:WhatIf']   = $false
  ```
- Inicializar variables fuera de `try` cuando puedan no asignarse en alguna rama (StrictMode las exige).
- Salida limpia en pantalla, detalle en `<BuildDir>\.logs\build_latest.txt` / `upload_latest.txt` (rotación 10, fuera de OneDrive).
- Nunca usar alias en scripts: `Get-ChildItem` no `gci`, `Where-Object` no `?`.
- Rutas con espacios siempre entrecomilladas.
- No imprimir credenciales ni tokens.
