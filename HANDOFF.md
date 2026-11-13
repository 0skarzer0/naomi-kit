# naomi-kit — traspaso para otra sesión

Última actualización: 2026-11-13. Qué es: [README.md](README.md). Proyecto: [naomi-wheel-games](https://github.com/0skarzer0/naomi-wheel-games).

## Estado

- **Creada el 2026-11-13** a partir de las piezas que repetían los ports nativos de Daytona USA (rama `naomi-native`
  de daytona-arcade-recomp, `platform/dreamcast-native/naomi/`) y de OutRunners (rama `naomi` de outrunners-port,
  `ports/naomi/`).
- Compila sin avisos (`-Wall -Wextra`) con sh-elf GCC 15.2.0 y KOS `368a2eb9` (`KOS_SUBARCH=naomi`):
  `make -C examples/minimal`. Las herramientas de la Pi, probadas contra un NetDIMM simulado (no en la placa).
- **Usuarios:** OutRunners (submódulo en `ports/naomi/naomi-kit`). **Daytona nativo aún no**: tiene sus propias
  copias; pasarlo al kit cuando se pueda probar en la placa (cambiar lo que funciona sin poder probarlo no compensa).
- **Nada del kit se ha probado todavía en la placa como kit** (las piezas sí, cada una en su port).

## Diferencias con las copias de los ports

- Bloque en vivo con **cabecera común** (`nk_live_head`, 0x60 bytes, magic `NKLV`) que apunta a los nombres de los
  campos del juego: un solo `live.py` para todos. Los ports tenían cada uno su struct y su `live.py`. **Desde el
  2026-11-13 el lector está en [naomi-pi-tools](https://github.com/0skarzer0/naomi-pi-tools) (`pi/live.py`)**, que lee el
  bloque del kit y los antiguos (mame4all, Daytona nativo, KallistiOS de dc2naomi); aquí queda `nk_live.py`, la librería
  de `watch.py`, `profile.py` y `drive.py`, e `install.sh` copia el lector a la Pi.
- Monedas por **diferencia de contadores** (OutRunners); Daytona usaba `coin_pulse`, que según OutRunners llega ya
  borrado. Al pasar Daytona, comprobar en la placa que la moneda entra.
- `test_inputs` admite además freno (bits 24-31) y SERVICE (bit 4).
- El SRAM está **entero repartido** (README): un juego nuevo que guarde necesita que se libere una región (la del
  recomp de Daytona, aparcado, es la candidata).

## Plan para pasar Daytona nativo al kit (2026-11-13, sin hacer a propósito)

No se hizo en esta sesión: Daytona nativo **funciona en placa** y no se puede compilar ni probar sin los datos de la
ROM del usuario (`recipes/build_assets.sh`) ni enDjinn, así que reescribirlo a ciegas arriesga romper lo que ya va.
Hacerlo en una rama (`naomi-native-kit`), con la ROM delante y la placa al lado. Mapa pieza a pieza:

| Daytona (`platform/dreamcast-native/naomi/`) | En el kit | Notas |
|---|---|---|
| `nm_naomi_boot` (`naomi_log.c`) | `nk_boot(&config)` con `.need_dimm=true`, `.misaligned=false` | Daytona no usa accesos desalineados |
| `nm_naomi_pvr_up`, `nm_naomi_show_log` | `nk_show_log`; el aviso de «PVR en marcha» hay que añadirlo al kit si hace falta | |
| `nm_naomi_show_sound_log` (TEST mantenido: log filtrado por «Sound», «music»…) | **falta en el kit** (punto 4 de arriba): añadirlo con el filtro como parámetro | |
| lista de músicas copiadas a RAM (`naomi_log.c`) | `nk_dimmfs_cache(nombre)` | la lista y su orden se quedan en el port |
| `naomi_dimmfs.*`, `naomi_dimmcomms.c`, `nm_naomi_live_t` («NEST») | `nk_dimmfs_*`, `nk_dimmcomms_forever`, `nk_live_head` + palabras (fps, vbls, fps_worst, frame_us, render_us…) | el lector de naomi-pi-tools lee los dos formatos |
| `naomi_input.c` | `nk_cabinet_read`; **se queda** el `__wrap_enj_ctrl_*` que lo entrega a enDjinn | su calibración propia se sustituye por la del kit |
| `naomi_save.c` | `nk_sram_load/save(0x6000, 0x2000, "DTN1", …)` | la semilla de la suma difiere: añadir `seed` al kit o perder la partida una vez |
| `naomi_pack.py` | `tools/nk_pack.py` | mismo formato de archivo |

Después de migrar: comparar en placa fps, música, TEST, guardado y mandos antes de borrar nada de la copia.

## Siguientes pasos

1. Probar OutRunners en la placa con el kit (`live.py ADDR words`: magic `NKLV`, nombres de campos).
2. Pasar Daytona nativo al kit: `naomi_dimmfs.c`, `naomi_dimmcomms.c`, `naomi_log.c` (lo común), `naomi_input.c`
   (sobre `nk_cabinet_read`, manteniendo su mapeo a enDjinn), `naomi_save.c` (sobre `nk_sram_*`, misma región 0x6000-0x7FFF y
   cabecera de 16 bytes, pero la semilla de su suma es `0x44544e31` y la del kit el magic leído al revés: o se
   añade una semilla al kit, o se acepta perder la partida guardada una vez), `naomi_pack.py`. (Su `live.py` ya se borró: lo sustituye el de naomi-pi-tools, que lee su bloque `NEST`.)
3. Cannonball (rama `naomi` de dreamcast-cannonball) y MAME4ALL: ver qué piezas propias tienen y pasarlas.
4. Una pantalla común de "mantén TEST" con el log filtrado (la tiene Daytona) y el estándar de controles del mueble
   (naomi-wheel-games, "Siguientes pasos").
