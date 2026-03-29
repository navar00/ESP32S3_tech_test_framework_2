# PROMPT — DEBUGGING EN TIEMPO DE EJECUCIÓN (RUNTIME)

**Marco de trabajo**: Usa las reglas y arquitectura definidas en `DevGuidelines.md` y `README.md` como contexto de referencia (no las repitas aquí).  
**Evidencia**: Razona únicamente a partir de los logs que proporcionaré aparte (mensaje posterior).

---

## MODO DE OPERACIÓN EN DOS FASES

### FASE 1 — ANÁLISIS Y PROPUESTA (sin tocar código)
1) **Reconstrucción temporal**  
   - Ordena cronológicamente los eventos del log: arranque, transiciones de pantalla, llamadas a subsistemas, errores.
   - Señala el **último estado estable** previo al problema.

2) **Señales relevantes**  
   - Extrae patrones del log: timeouts, errores, silencios anómalos, repeticiones, mensajes faltantes donde deberían existir.
   - Filtra explícitamente el **Ruido**: Señala qué errores en el log son normales o irrelevantes para este problema.
   - Cita líneas clave del log que respalden cada hallazgo.

3) **Clasificación del fallo (con evidencia)**  
   Marca **solo si hay indicios claros**:  
   - ☐ Bloqueo del loop principal  
   - ☐ Starvation del watchdog  
   - ☐ Presión/fragmentación de memoria  
   - ☐ Error de ciclo de vida (`onEnter`/`onExit`)  
   - ☐ Condición de carrera/concurrencia  
   - ☐ Fallo de integración con un subsistema (p. ej. radio)  
   - ☐ **Falta de Observabilidad** (No hay logs suficientes para concluir)
   - ☐ Otro (justificar)  
   Para cada candidato: **evidencias a favor** y **en contra** (citas del log).

4) **Punto exacto de ruptura**  
   - Identifica el momento/entrada del log donde el sistema pasa de correcto a incorrecto.  
   - Explica qué **debía** ocurrir y qué **ocurrió** según el log.

5) **Conclusión técnica y propuesta**  
   - **Causa más probable** (basada en el log).  
   - **Propuesta de solución** de alcance mínimo e incremental:
     - **Opción A (Fix)**: Si la causa es clara, qué código cambiar.
     - **Opción B (Instrumentación)**: Si falta info, qué logs añadir (`Logger::log`) para revelar el estado oculto.
   - Indica impacto documental (`README`/`DevGuidelines`).

> **IMPORTANTE**: En Fase 1 no escribas código. Entrega solo análisis + propuesta.

---

### FASE 2 — IMPLEMENTACIÓN CONTROLADA (solo si yo la apruebo)
**Condición de inicio**: esperar mi confirmación explícita de la propuesta de Fase 1.

1) **Implementa** exactamente lo propuesto (Fix o Instrumentación).  
2) **Compila** con `./build.ps1` e **itera** hasta build limpia.  
3) **Valida** rápidamente el síntoma (o captura nuevos logs si fue Instrumentación).  
4) **Documenta de forma incremental**:  
   - Actualiza **solo** las secciones relevantes del `README.md`.
   - Si se descubrió un nuevo patrón de error, añádelo a `DevGuidelines.md` (Anti-patterns).

**Entrega final de Fase 2**:  
- Resumen de cambios  
- Extracto sintético del build  
- Validación del síntoma  
- Lista de archivos tocados  
- Qué se añadió/modificó en documentación
