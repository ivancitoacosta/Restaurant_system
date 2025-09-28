# Prompt Maestro - Sistema de Gestión de Restaurante

Proyecto multiplataforma escrito en C (C17) con GTK y SQLite para administrar un restaurante. Incluye autenticación con roles, gestión de menú, manejo de salón y mesas, comandas en vivo, cobro y reportes.

## Características

- Interfaz GTK (GTK 4 por defecto, opcional GTK 3) con diseño simple de barra lateral.
- Persistencia en SQLite con migraciones versionadas.
- Servicios de dominio en C separados por capas (`ui`, `core`, `data`, `util`).
- Autenticación con contraseñas hasheadas (SHA-256 con sal basada en usuario).
- CRUD básico de menú y mesas a través de la interfaz.
- Comandas en vivo: agregar ítems, actualizar estados y generar ticket HTML.
- Reportes de ventas diarios exportados a CSV.
- Configuración en archivo `config.ini` (moneda, IVA, idioma, rutas).
- Logs diarios con rotación automática.
- Soporte de i18n simple (ES/EN).
- Script de bootstrap para crear/migrar la base de datos y datos de ejemplo.
- Pruebas unitarias con CTest.

## Estructura de carpetas

```
├── assets/          # Recursos estáticos (estilos, íconos)
├── include/         # Headers públicos por capa
├── migrations/      # Migraciones SQL de referencia
├── po/              # Catálogo de traducciones ES/EN
├── scripts/         # Scripts utilitarios (bootstrap)
├── src/             # Código fuente (ui, core, data, util)
├── tests/           # Pruebas unitarias
└── reports/         # Salida de reportes CSV (generada en runtime)
```

## Dependencias

- [CMake 3.16+](https://cmake.org/)
- [SQLite3](https://sqlite.org/)
- [GTK 4](https://www.gtk.org/) (o GTK 3 si se configura `-DUSE_GTK4=OFF`)
- Compilador C compatible con C17 (GCC, Clang, MinGW-w64)

### Linux (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libgtk-4-dev libsqlite3-dev
```

Para GTK 3:

```bash
sudo apt install libgtk-3-dev
cmake -B build -S . -DUSE_GTK4=OFF
```

### Windows (MSYS2 / MinGW-w64)

1. Instalar [MSYS2](https://www.msys2.org/).
2. Abrir la terminal `MSYS2 MINGW64` y ejecutar:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-pkgconf \
           mingw-w64-x86_64-gtk4 mingw-w64-x86_64-sqlite3
```

3. Configurar y compilar:

```bash
cmake -B build -S . -G "MinGW Makefiles"
cmake --build build
```

## Compilación y ejecución

```bash
cmake -B build -S .
cmake --build build
./build/restaurant_app
```

Para inicializar o actualizar la base de datos manualmente:

```bash
./build/restaurant_app --bootstrap
```

Para exportar el reporte diario (fecha en formato `YYYY-MM-DD`):

```bash
./build/restaurant_app --export-report=2024-05-21
```

## Tests

```bash
cmake -B build -S .
cmake --build build --target restaurant_tests
ctest --test-dir build
```

Las pruebas cubren utilidades (configuración, hash) y servicios de dominio mediante mocks simples.

## Migraciones y datos de ejemplo

- Migraciones aplicadas en runtime desde constantes C y replicadas en `migrations/` para referencia.
- `scripts/bootstrap_db.sh` ejecuta la app con `--bootstrap` para crear tablas y seed inicial (1 admin, 2 mozos, 10 mesas, platos de ejemplo).

## Tickets y reportes

- Los tickets HTML se generan en la carpeta `receipts/` con formato básico (puede imprimirse o transformarse a PDF con herramientas externas como `wkhtmltopdf`).
- Los reportes diarios se exportan a `reports/ventas_<fecha>.csv` con totales por comanda.

## Accesibilidad y atajos

- Navegación por teclado habilitada (GTK maneja foco automáticamente).
- Controles etiquetados y agrupados semánticamente.
- Se pueden añadir atajos adicionales configurando `GtkShortcutController` (pendiente para futuras versiones).

## Troubleshooting

- **GTK no encontrado:** validar que `pkg-config` detecte `gtk4`. Ejecutar `pkg-config --modversion gtk4`.
- **Errores de compilación en Windows:** asegurarse de usar la terminal `MSYS2 MINGW64` y limpiar `build/`.
- **Base corrupta:** eliminar `restaurant.db` y ejecutar `./restaurant_app --bootstrap`.
- **Logs:** revisar carpeta `logs/` para trazas detalladas.

## Mockups

La interfaz se organiza con barra lateral y tres vistas principales: Salón, Comandas y Reportes. En Salón se listan mesas con estado y selector para crear comanda. En Comandas se muestra listado de comandas abiertas, items, formulario para agregar platos y herramientas de cierre. En Reportes se ofrece exportación rápida a CSV.
