# ID-12LA-HE footprint derivation

Source: manufacturer provisional X1.2, 2015-02-28, pin diagram p.3 explicitly bottom view; mechanical drawing p.9. [Datasheet](https://www.id-innovations.com/ID-3%2612%2620LA-HE%28en%29new.pdf).

The project footprint is viewed from the carrier top. Its origin is the nominal body center, not a pin. Nominal body dimensions are 26.4 in x by 25.3 in y. This is a mirrored/rotated pin diagram, with the six-pin row at negative y and isolated pin11 at the right of that row. The five-pin row is at positive y.

| Pin | x mm | y mm |
|---|---:|---:|
| 1 | 3.10 | 7.35 |
| 2 | 1.10 | 7.35 |
| 3 | -0.90 | 7.35 |
| 4 | -2.90 | 7.35 |
| 5 | -4.90 | 7.35 |
| 6 | -4.90 | -7.65 |
| 7 | -2.90 | -7.65 |
| 8 | -0.90 | -7.65 |
| 9 | 1.10 | -7.65 |
| 10 | 3.10 | -7.65 |
| 11 | 7.10 | -7.65 |

Dimensional construction: body G=26.4, D=25.3; top-row offset from body top D−E=5.0; leftmost row pad from body left F−B=8.3; row separation C=15.0; pitch P=2.0; isolated pin gap is 4.0. Pin11 is x20.3/y5.0 and pin1 is x16.3/y20.0 measured from nominal body top-left.

The 1.1 mm finished drill and 1.7 mm pad are proposed manufacturing choices. A 0.67 mm square pin has a 0.948 mm diagonal. The 0.152 mm diametral allowance does **not** absorb the full published position-tolerance stack. The provisional table permits broad dimensions: physical sample and revised manufacturer drawing verification remain required before fabrication. Do not claim this footprint is production qualified.

The courtyard is the maximum centered body envelope (27.1 by25.9) plus0.5 mm per side. This treats envelope growth as centered; body-to-pin variation must be checked on a sample. The internal coil must not have a ground plane placed beneath it. The board-level RF keepout must allow the actual host pads and short escape traces; an unconditional track ban across all pads would be erroneous.

Maximum body height 6.6 mm and total pin-tip envelope 10.5 mm require a mechanical assembly check. Worst independent lead projection is Jmax − Hmin = 10.5 − 5.8 = 4.7 mm; subtracting maximum body height would not give the worst projection. The present enclosure requires controlled trimming after soldering, with inspected joint integrity and sufficient remaining lead. Follow the mechanical assembly drawing's final lead limit. Do not add a socket without recalculating enclosure clearance.
