import json
import os
import glob
import sys
try:
    from PIL import Image
    from collections import Counter
except ImportError:
    print("Debes instalar 'Pillow' con: pip install Pillow")
    sys.exit(1)

def rgb_to_hex(rgb):
    return '#{:02x}{:02x}{:02x}'.format(rgb[0], rgb[1], rgb[2])

def extract_palette(image_path, num_colors=16):
    try:
        img = Image.open(image_path).convert('RGB')
        img = img.quantize(colors=num_colors, method=Image.Quantize.MEDIANCUT).convert('RGB')
        
        pixels = list(img.getdata())
        color_counts = Counter(pixels)
        most_common = color_counts.most_common(num_colors)
        
        palette = []
        for rgb, count in most_common:
            palette.append({
                'hex': rgb_to_hex(rgb),
                'rgb': list(rgb),
                'frequency': count
            })
        return palette
    except Exception as e:
        print(f"Error procesando {image_path}: {e}")
        return None

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    target_folder = "GertrudArndt"
    if len(sys.argv) > 1:
        target_folder = sys.argv[1]
        
    work_dir = os.path.join(script_dir, target_folder)
    
    if not os.path.isdir(work_dir):
        print(f"Error: El directorio '{work_dir}' no existe.")
        sys.exit(1)
        
    out_file = os.path.join(work_dir, f"{target_folder}.json")
    palettes = {}
    
    # Buscar todos los archivos de imagen en el directorio objetivo
    print(f"Analizando imagenes en: {work_dir}")
    
    import re
    all_files = [f for f in glob.glob(os.path.join(work_dir, "*")) if os.path.isfile(f) and not f.endswith(".json")]
    
    image_files = []
    assigned_names = {}
    reserved_nums = set()
    
    pattern = re.compile(r'^paleta(\d+)$', re.IGNORECASE)
    
    # Primera pasada: identificar imagenes validas y reservar nombres "paleta#"
    for path in all_files:
        try:
            with Image.open(path) as img:
                img.verify() # Comprueba que es una imagen valida (sin importar la extension)
            image_files.append(path)
            
            name = os.path.splitext(os.path.basename(path))[0]
            match = pattern.match(name)
            if match:
                num = int(match.group(1))
                assigned_names[path] = f"paleta{num}"
                reserved_nums.add(num)
        except Exception:
            pass # No es una imagen o no se puede procesar
            
    # Segunda pasada: asignar "paleta#" al resto
    next_num = 1
    for path in image_files:
        if path not in assigned_names:
            while next_num in reserved_nums:
                next_num += 1
            assigned_names[path] = f"paleta{next_num}"
            reserved_nums.add(next_num)
            
    for path in image_files:
        pal_name = assigned_names[path]
        print(f"  Procesando '{os.path.basename(path)}' como '{pal_name}'...")
        
        pal = extract_palette(path, 16)
        if pal:
            palettes[pal_name] = pal

    if palettes:
        with open(out_file, 'w', encoding='utf-8') as f:
            json.dump(palettes, f, indent=4)
        print(f"Exito! Guardadas {len(palettes)} paletas en '{out_file}'")
    else:
        print("No se encontraron imagenes para procesar o falló la extraccion.")
