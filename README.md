# lsg-overlay

Overlay para videojuegos en Windows, construido con **C++20**, **SDL3** y **Dear ImGui**. Se acopla a la ventana del videojuego para mostrar métricas y canjear mecánicas mediante servicios HTTP locales.

## Requisitos

- Windows
- Visual Studio 2026 (C++)
- CMake ≥ 4.2
- vcpkg (`VCPKG_ROOT` configurada)

## Compilación

Desde la raíz del repositorio. vcpkg instala las dependencias automáticamente.

```powershell
cmake --preset windows-msvc-release-static
cmake --build --preset windows-msvc-release-static
cmake --build --preset windows-msvc-release-static-install
```

Salida: `build/msvc-release-static/Release/lsg-overlay.exe`.

La instalación copia el ejecutable a `C:/Dev/tesis/lsg-client/src/shared/`. Para otra ruta, modifica `CMAKE_INSTALL_PREFIX` en `CMakeLists.txt` antes de configurar.

Para depurar:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

## Ejecución

`lsg-client` inicia el overlay automáticamente. Para probarlo manualmente, mantén el cliente y el videojuego abiertos. El cliente proporciona los datos.

```powershell
.\build\msvc-release-static\Release\lsg-overlay.exe <playerId> <targetHWND> <videogameId>

# playerId: id del jugador en el backend de lsg
# videogameId: id del videojuego en el backend de lsg
# targetHWND: HWND válido de la ventana del videojuego en ejecución

# Ejemplo para Stardew Valley con IDs de ejemplo
.\build\msvc-release-static\Release\lsg-overlay.exe 1 2493544 1
```

Servicios locales (`src/network.cpp`):
- Backend del cliente: `http://127.0.0.1:3333`.
- Servidor del mod/videojuego: `http://localhost:7878`.

**F3** muestra u oculta el overlay. Al cerrar la ventana del videojuego, el overlay termina.
