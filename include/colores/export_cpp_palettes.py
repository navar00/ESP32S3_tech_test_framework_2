import os
import json

def build_cpp_header():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    master_json_path = os.path.join(base_dir, "master_palettes.json")
    
    if not os.path.exists(master_json_path):
        print("master_palettes.json no existe. Ejecuta aggregate_palettes.py primero.")
        return
        
    with open(master_json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
        
    out_file = os.path.join(base_dir, "..", "..", "src", "core", "PalettesData.h")
    out_file = os.path.abspath(out_file)
    
    # Export it as a raw string so ESP32 can send it via HTTP
    min_json = json.dumps(data)
    
    code = """// AUTOGENERADO - NO MODIFICAR
#pragma once
#include <Arduino.h>

const char PALETTES_JSON[] PROGMEM = R"rawjson(
"""
    code += min_json
    code += """
)rawjson";
"""
    
    with open(out_file, 'w', encoding='utf-8') as f:
        f.write(code)
        
    print(f"Exportado header C++ a: {out_file}")

if __name__ == "__main__":
    build_cpp_header()
