# PROMPT — CIERRE DE ITERACIÓN

Aplica estrictamente:
- DevGuidelines.md (Gobierno del código)
- README.md (Arquitectura y especificación técnica)

Estamos en **MODO CIERRE DE ITERACIÓN**.

---

## OBJETIVO
Validar que la iteración está realmente terminada desde el punto de vista de:
- estabilidad
- mantenibilidad
- observabilidad
- coherencia documental

No se añaden features nuevas en este modo.
Solo se valida, corrige y consolida.

---

## CHECKLIST OBLIGATORIA

### 1) COMPILACIÓN Y BUILD
- ✅ El proyecto compila usando `./build.ps1`
- ✅ No hay warnings relevantes
- ✅ El tamaño de binario y uso de RAM son coherentes
- ✅ No se ha introducido presión innecesaria de heap/PSRAM

Si algo aquí falla:
→ la iteración NO está cerrada.

---

### 2) ESTABILIDAD EN TIEMPO DE EJECUCIÓN
- ✅ La pantalla soporta ciclos completos:
  - entrar
  - salir
  - volver a entrar
- ✅ No hay memory leaks visibles
- ✅ No quedan tareas FreeRTOS activas tras `onExit`
- ✅ No se producen WDT resets ni Core Panics

---

### 3) DISCIPLINA ARQUITECTÓNICA
- ✅ La pantalla respeta su responsabilidad única
- ✅ No accede a hardware fuera de HAL
- ✅ No introduce dependencias cruzadas entre pantallas
- ✅ El `ScreenManager` sigue siendo el único orquestador

---

### 4) OBSERVABILIDAD
- ✅ Los logs permiten entender qué ocurre sin depurar
- ✅ Los errores incluyen contexto (no mensajes genéricos)
- ✅ Los eventos relevantes están trazados con TAG correcto

---

### 5) DOCUMENTACIÓN (OBLIGATORIA)
- ✅ El `README.md` refleja el estado real del sistema
- ✅ Las secciones afectadas han sido:
  - ampliadas o corregidas de forma incremental
- ✅ Si hubo problemas o decisiones relevantes:
  - han quedado documentadas como “lecciones aprendidas”

Si el código funciona pero el README miente:
→ la iteración está incompleta.

---

## RESULTADO ESPERADO

Devuelve:
1) Resumen breve de la iteración
2) Riesgos detectados (si los hay)
3) Cambios realizados en README.md
4) Confirmación explícita de:
   ✅ ITERACIÓN CERRADA
   o
   ❌ ITERACIÓN NO APTA (y por qué)

---

### 6) PUSH A GITHUB (Post-Cierre)
Si la iteración está cerrada (`✅ ITERACIÓN CERRADA`):

1.  **Verificar** que `.gitignore` excluye logs y artefactos de build.
2.  **Stage** todos los cambios relevantes:
    ```bash
    git add -A
    ```
3.  **Commit** con mensaje descriptivo siguiendo el formato:
    ```
    feat|fix|docs|refactor(scope): descripción breve

    - Cambio 1
    - Cambio 2
    ```
4.  **Push** a `origin main`:
    ```bash
    git push origin main
    ```
5.  **Confirmar** que el push fue exitoso (exit code 0, sin errores de autenticación o conflictos).

Si el push falla:
→ Diagnosticar (credenciales, conflictos, rama protegida) y resolver antes de cerrar.
