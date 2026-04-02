import json
import random
import tkinter as tk
import os
import sys

# Configuracion simulada del TFT
TFT_WIDTH = 320
TFT_HEIGHT = 240
CELL_SIZE = 40  # Tamaño gigante para testear paleta

# 1er arg = nombre del directorio, 2o arg = nombre de paleta (opcional)
target_folder = "GertrudArndt"
palette_name = "paleta1"

if len(sys.argv) > 1:
    target_folder = sys.argv[1]
if len(sys.argv) > 2:
    palette_name = sys.argv[2]

# Cargar el archivo JSON desde el directorio objetivo
script_dir = os.path.dirname(os.path.abspath(__file__))
json_path = os.path.join(script_dir, target_folder, f"{target_folder}.json")

colores = ["#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FFFFFF"]  # fallback inicial

try:
    if os.path.exists(json_path):
        with open(json_path, "r", encoding="utf-8") as f:
            data = json.load(f)
            
        if palette_name in data:
            colores = [c["hex"] for c in data[palette_name]]
            print(f"[{palette_name}] cargada con exito ({len(colores)} colores) desde la carpeta '{target_folder}'")
        else:
            # Intentar usar la primera clave disponible como fallback (ej. si piden 'paleta1' pero se llama 'fotoA')
            disponibles = list(data.keys())
            if disponibles:
                print(f"[{palette_name}] no se encontro en el JSON '{target_folder}.json'. Usando la primera disponible: [{disponibles[0]}].")
                colores = [c["hex"] for c in data[disponibles[0]]]
                palette_name = disponibles[0]
            else:
                print(f"[{json_path}] no contiene paletas validas. Usando colores de fallback por defecto.")
    else:
        print(f"Advertencia: No se encontro el fichero '{json_path}'.\n         \xbfHas lanzado extract_colors.py sobre la carpeta '{target_folder}' antes? \n         Usando colores de fallback por defecto.")
except Exception as e:
    print(f"Error cargando JSON en {json_path}: {e}")

# Inicializar ventana de Tkinter
root = tk.Tk()
root.title(f"Simulacion TFT 320x240 - Dirt: {target_folder} | Pal: {palette_name}")
root.geometry(f"{TFT_WIDTH}x{TFT_HEIGHT}")
root.resizable(False, False)

# Lienzo para dibujar
canvas = tk.Canvas(root, width=TFT_WIDTH, height=TFT_HEIGHT, bg="black", highlightthickness=0)
canvas.pack()

# Matriz de celdas
cols = TFT_WIDTH // CELL_SIZE
rows = TFT_HEIGHT // CELL_SIZE

# Crear rectangulos iniciales
rects = []
for y in range(rows):
    row_rects = []
    for x in range(cols):
        x0 = x * CELL_SIZE
        y0 = y * CELL_SIZE
        x1 = x0 + CELL_SIZE
        y1 = y0 + CELL_SIZE
        
        color = random.choice(colores)
        rect = canvas.create_rectangle(x0, y0, x1, y1, fill=color, outline="")
        row_rects.append(rect)
    rects.append(row_rects)

def refescar_pantalla():
    for y in range(rows):
        for x in range(cols):
            # Cambiar color con probabilidad media-baja
            if random.random() < 0.10:
                nuevo_color = random.choice(colores)
                canvas.itemconfig(rects[y][x], fill=nuevo_color)
                
    # Loop cada 200ms para efecto un poco más tranquilo
    root.after(200, refescar_pantalla)

# Iniciar animacion y UI
refescar_pantalla()
root.mainloop()
