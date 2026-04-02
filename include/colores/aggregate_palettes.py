import os
import json

def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    master_json_path = os.path.join(base_dir, "master_palettes.json")
    aggregated_data = {}

    # Iterar por todos los elementos dentro del directorio 'colores'
    for item in os.listdir(base_dir):
        item_path = os.path.join(base_dir, item)
        
        # Si es un directorio, buscar en su interior su fichero JSON correspondiente
        if os.path.isdir(item_path):
            json_file_path = os.path.join(item_path, f"{item}.json")
            
            if os.path.exists(json_file_path):
                try:
                    with open(json_file_path, 'r', encoding='utf-8') as f:
                        data = json.load(f)
                        # Agregar al diccionario maestro bajo la dimension del nombre del directorio
                        aggregated_data[item] = data
                        print(f"  -> Agregadas las paletas del directorio '{item}' ({len(data)} paletas encontradas)")
                except Exception as e:
                    print(f"  -> Error procesando el archivo {json_file_path}: {e}")

    # Guardar todos los datos combinados en un archivo JSON principal
    if aggregated_data:
        try:
            with open(master_json_path, 'w', encoding='utf-8') as f:
                json.dump(aggregated_data, f, indent=4)
            print(f"\nExito! Archivo maestro de colores guardado en:\n{master_json_path}")
            print(f"Dimensiones anadidas: {', '.join(aggregated_data.keys())}")
        except Exception as e:
            print(f"Error al guardar el archivo maestro: {e}")
    else:
        print("\nNo se encontraron archivos JSON de paletas en las subcarpetas.")

if __name__ == "__main__":
    main()
