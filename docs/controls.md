# Controles estándar del mueble Crazy Taxi (propuesta)

Un único criterio para todos los juegos del mueble, para que quien se siente a jugar no tenga que aprender cada port.
**Es una propuesta (2026-11-13):** `nk_cabinet_read` y `nk_cabinet_start_coin` ya la permiten; los ports la adoptan
cuando les toque. Hoy cada uno lo resuelve a su manera, y está anotado abajo.

*In English:* one control scheme for every game on the Crazy Taxi upright. A proposal: the kit already supports it,
ports adopt it as they are touched.

## El mueble

Volante analógico (~270°), acelerador, freno, palanca de 2 posiciones con retorno (adelante / atrás), START, SERVICE,
TEST y monedero. Valores JVS medidos y trampas del hardware:
[naomi-wheel-games/docs/hardware.md](https://github.com/0skarzer0/naomi-wheel-games/blob/main/docs/hardware.md).

## Propuesta

| Control del mueble | Función en el juego |
|---|---|
| Volante | Dirección (o volante giratorio / spinner en los arcades de 1 eje) |
| Acelerador | Acelerar |
| Freno | Frenar |
| Palanca adelante / atrás | Marcha alta / baja (SHIFT UP / DOWN) |
| Monedero | Crédito, como en la máquina original |
| START, **pulsación corta** | Empezar partida; si no hay créditos, inserta uno (`nk_cabinet_start_coin`: pulsar = moneda, soltar = START) |
| START mantenido ~1 s | Menú o pausa del port (si el juego no tiene otro) |
| SERVICE | Servicio / monedas de servicio, como la máquina original |
| TEST | Menú de pruebas de la placa; los ports no lo usan |

Por qué: sin monedero real a mano (pruebas, casa) un solo botón basta para arrancar, y con monedas reales todo
funciona como en la recreativa. Lo medido en OutRunners: una pulsación corta empieza la carrera.

## Hoy, port a port

| Port | START | Menú |
|---|---|---|
| OutRunners | pulsar = moneda, soltar = START | (sin menú propio) |
| MAME4ALL | START pulsado = crédito, al soltar = START | (sin documentar aquí) |
| Cannonball | (sin documentar aquí) | freno + START |
| Daytona nativo | (sin documentar aquí: mirar su HANDOFF) | — |

Para adoptar la propuesta, en `main()`: `nk_cabinet_start_coin(frames)` y leer `nk_cabinet_read`; lo demás lo hace el kit.
Si un port se aparta de esto, anotarlo en su HANDOFF y en esta tabla.
