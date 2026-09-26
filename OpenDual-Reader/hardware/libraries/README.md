# Project-local library provenance

`OpenDual.kicad_sym` contains independently drawn symbols with explicit datasheet pin numbers. Project tables and model references use portable `${KIPRJMOD}` paths.

Standard footprints and STEP models were copied from installed KiCad 10.0.6 libraries. They retain their upstream copyright and license (CC-BY-SA 4.0 with the KiCad library exception, subject to asset metadata). The upstream license is included as [KICAD-LIBRARY-LICENSE.md](KICAD-LIBRARY-LICENSE.md). Upstream: https://gitlab.com/kicad/libraries . Placement-specific reference visibility does not alter pad geometry.

`ID12LA_HE.kicad_mod` derives its pin locations from the manufacturer package drawing. Its broad tolerances still require a physical sample check. See `ID12LA_HE-footprint-notes.md`.

`ID12LA_HE.wrl`, `HF_MINI_V2.wrl` and `HF_ferrite.wrl` are editable, coarse dimensional proxies, not manufacturer CAD. The HF proxy shows a board/chip/connector envelope; it does not claim to reconstruct the vendor's internal circuitry. Main-board copper tracks are actual carrier connections. The supplied harness and remote tamper switch are not modeled as board-mounted components.

The 1812L fuse model is a simplified maximum envelope (4.73 x 3.41 x 1.55 mm) from the Littelfuse June 2024 datasheet; markings and internal layers are not modeled. The standard 1812 land pattern is an IPC-style prototype choice, not a claim of vendor reflow qualification.

The standard STEP files are KiCad library representations, not evidence of exact manufacturer CAD or guaranteed maximum-material dimensions. Model-file existence checks do not validate dimensions, fit, polarity or assembly tolerances.
