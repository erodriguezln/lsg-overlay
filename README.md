# lsg-overlay

Aplicación de overlay transparente e interactivo para clientes de videojuegos Win32, construida con **C++20**, **SDL3** y **Dear ImGui**.

El overlay se acopla como una ventana hija de una ventana Win32 objetivo y se comunica con un backend web utilizando `cpp-httplib` y `nlohmann-json` para la obtención de métricas de juego en tiempo real.

## Requisitos

- **CMake** ≥ 3.30
- Compilador con soporte para **C++20** (MSVC / Visual Studio 2022 o superior)
- **vcpkg** instalado en el sistema.
- Variable de entorno **`VCPKG_ROOT`** configurada apuntando al directorio de instalación de vcpkg.

## Gestión de Dependencias

El proyecto utiliza **vcpkg** en **Manifest Mode** para resolver automáticamente las librerías a través de los archivos:
- `vcpkg.json`: Declara las librerías necesarias y habilita los bindings de SDL3 para Dear ImGui.
- `vcpkg-configuration.json`: Fija la versión del registro (baseline) para asegurar compilaciones reproducibles.

## Compilación

El proyecto utiliza **CMake Presets** para simplificar la compilación.

### 1. Configurar el proyecto
Ejecuta la configuración para generar los archivos de construcción (vcpkg descargará y compilará las dependencias en este paso):

```bash
# Configuración en modo Debug
cmake --preset windows-msvc-debug

# Configuración en modo Release
cmake --preset windows-msvc-release
```

### 2. Compilar
```bash
# Compilar en modo Debug
cmake --build --preset windows-msvc-debug

# Compilar en modo Release e instalar DLLs
cmake --build --preset windows-msvc-release --target install
```

Los binarios finales de la versión Release y sus DLLs asociadas se instalarán automáticamente en la ruta compartida del cliente: `C:/Dev/tesis/lsg-client/src/shared/`.

## Ejecución

El programa debe ser lanzado desde la consola o como un proceso hijo pasando los siguientes tres parámetros:

```bash
lsg-overlay.exe <playerId> <targetHWND> <videogameId>
```

- **`playerId`**: ID del jugador para consultar las estadísticas.
- **`targetHWND`**: El manejador de ventana (HWND en formato numérico) de la ventana del juego sobre la cual se superpondrá el overlay.
- **`videogameId`**: ID del videojuego para sincronizar las mecánicas específicas.