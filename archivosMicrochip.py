import os
from pathlib import Path

def main():
    # Rutas relativas al directorio raíz (donde se ejecuta el script)
    app_core_dir = Path("app_core")
    cproj_file = Path("entornos_ide/AVR_MicrochipStudio/ATMega328P/ATMega328P/ATMega328P.cproj")

    if not app_core_dir.exists():
        print(f"❌ Error: No se encontró el directorio '{app_core_dir}'. Ejecuta el script desde la raíz del proyecto.")
        return
    if not cproj_file.exists():
        print(f"❌ Error: No se encontró el archivo '{cproj_file}'.")
        return

    print("🔍 Escaneando archivos en app_core...")
    archivos_fuente = []
    carpetas = set()

    # 1. Recorrer app_core recursivamente
    for path in app_core_dir.rglob("*"):
        if path.is_file() and path.suffix in [".c", ".h"]:
            # Obtener la ruta relativa respecto a app_core (ej. 'app\comandos.c')
            rel_path = path.relative_to(app_core_dir)
            archivos_fuente.append(rel_path)

            # Agregar todos los directorios padres a la lista de carpetas (excluyendo la raíz '.')
            for parent in rel_path.parents:
                if str(parent) != ".":
                    carpetas.add(str(parent))

    # Ordenar alfabéticamente para mantener consistencia en el control de versiones
    archivos_fuente = sorted(archivos_fuente)
    carpetas = sorted(list(carpetas))

    print(f"📂 Se encontraron {len(carpetas)} carpetas y {len(archivos_fuente)} archivos (.c y .h).")

    # 2. Construir el bloque XML para las carpetas
    xml_folders = "  <ItemGroup>\n"
    for folder in carpetas:
        # Microchip Studio exige barras invertidas (\)
        folder_win = str(folder).replace("/", "\\") + "\\"
        xml_folders += f'    <Folder Include="{folder_win}" />\n'
    xml_folders += "  </ItemGroup>\n"

    # 3. Construir el bloque XML para los archivos fuente
    xml_files = "  <ItemGroup>\n"
    for src in archivos_fuente:
        src_win = str(src).replace("/", "\\")
        xml_files += f'    <Compile Include="..\\..\\..\\..\\app_core\\{src_win}">\n'
        xml_files += f'      <SubType>compile</SubType>\n'
        xml_files += f'      <Link>{src_win}</Link>\n'
        xml_files += f'    </Compile>\n'
    xml_files += "  </ItemGroup>\n"

    # 4. Leer y modificar el archivo .cproj de forma segura (sin romper el formato XML original)
    print("✍️ Actualizando ATMega328P.cproj...")
    contenido = cproj_file.read_text(encoding="utf-8")

    # Los archivos .cproj agrupan los archivos justo entre el último </PropertyGroup> y el <Import Project>
    idx_prop_end = contenido.rfind("</PropertyGroup>")
    idx_import = contenido.find("<Import Project")

    if idx_prop_end == -1 or idx_import == -1:
        print("❌ Error: No se pudo identificar la estructura esperada del archivo .cproj.")
        return

    # Ensamblar el nuevo contenido reemplazando únicamente los ItemGroups de archivos/carpetas
    prefix = contenido[:idx_prop_end + len("</PropertyGroup>")] + "\n"
    suffix = contenido[idx_import:]
    nuevo_contenido = prefix + xml_folders + xml_files + "  " + suffix

    # Guardar los cambios
    cproj_file.write_text(nuevo_contenido, encoding="utf-8")
    print("✅ Archivo actualizado correctamente.")

if __name__ == "__main__":
    main()