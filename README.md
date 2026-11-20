# KLiftSubject — Extraer Sujeto y Eliminar Fondo (IA) para KDE Plasma 6 + Wayland

**KLiftSubject** replica las Acciones Rápidas de **macOS Finder ("Eliminar fondo" / "Copiar sujeto" / Lift Subject Sticker)** de forma nativa en **KDE Plasma 6.8 + Dolphin + Wayland**.

Permite aislar el sujeto principal de cualquier imagen (`PNG`, `JPEG`, `WebP`) mediante IA local (`rembg`), recortar automáticamente los márgenes transparentes alrededor de la silueta del sujeto, generar un borde blanco estilo sticker de macOS/iOS y copiar el recorte directamente al portapapeles de Wayland (`wl-copy --type image/png`).

## Características

- **Integración directa en Dolphin**: Menú contextual **Acciones → Extraer Sujeto (IA)** con tres modos:
  1. **Eliminar fondo (guardar PNG)**: Genera `<archivo>-nobg.png` junto a la imagen original conservando las dimensiones.
  2. **Copiar sujeto al portapapeles**: Extrae el sujeto, recorta automáticamente la caja delimitadora alfa (`cropToSubject`) y copia el PNG transparente al portapapeles de Wayland.
  3. **Crear sticker con borde blanco**: Extrae el sujeto, recorta los márgenes transparentes, añade un contorno blanco suave (`addStickerOutline`), guarda `<archivo>-sticker.png` y lo copia al portapapeles.
- **Notificaciones nativas**: Informa del inicio y finalización del procesado mediante `notify-send`.
- **100 % Local y Privado**: Ejecuta `rembg` localmente sin enviar imágenes a servidores externos.

## Requisitos

- **CachyOS / Arch Linux** con KDE Plasma 6.8 (Wayland)
- Qt 6 (`Core`, `Gui`, `Test`) y `extra-cmake-modules` (ECM)
- `~/.local/bin/rembg` (instalado vía `pipx install "rembg[cpu,cli]"`)
- `wl-clipboard` (`wl-copy`) y `libnotify` (`notify-send`)

## Compilación e Instalación

```bash
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
sudo -n cmake --install build --prefix /usr
kbuildsycoca6
```

El menú de servicio se instala automáticamente con permisos de ejecución (`+x`) en:
`/usr/share/kio/servicemenus/kliftsubject.desktop`

## Uso por CLI

```bash
# Eliminar fondo y guardar <nombre>-nobg.png
kliftsubject --remove-bg foto.jpg

# Extraer sujeto, recortar márgenes transparentes y copiar al portapapeles Wayland
kliftsubject --copy-subject retrato.png

# Crear sticker con borde blanco (<nombre>-sticker.png) y copiar al portapapeles
kliftsubject --sticker mascota.webp
```

## Limitaciones conocidas

- La primera ejecución de `rembg` puede tardar unos segundos adicionales mientras carga el modelo U2Net en memoria.
- La copia al portapapeles requiere una sesión Wayland activa con `wl-copy`.
