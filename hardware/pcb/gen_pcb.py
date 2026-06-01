#!/usr/bin/env python3
"""
Generate KiCad PCB layout files (.kicad_pcb) for both v1 and v2 variants.
These files open in KiCad 7/8 and can be auto-routed + exported as Gerbers.
"""

import textwrap

# ── Component placement data ──────────────────────────────────────────────
# Each entry: (ref, value, footprint_lib, at_x, at_y, rotation, layer, lcsc)
# Coordinates in mm from board origin (bottom-left = 0,0)
# Rotations: 0=default, 90=rotated CW 90°, 180=flipped, 270=rotated CCW 90°

V1_COMPONENTS = [
    # ref        value                footprint_lib_path                              x      y    rot  layer
    ("U2", "ESP32-C3-MINI-1",   "RF_Module:ESP32-C3-MINI-1",                       35.00, 28.00,  0, "F.Cu"),
    ("U3", "SHT31-DIS",         "Package_DFN_QFN:DFN-8-1EP_3x3mm_P0.5mm_EP1.5x2.3mm", 9.00,  9.00,  0, "F.Cu"),
    ("U4", "BH1750FVI",         "Package_SO:SOP-8_3.76x4.94mm_P1.27mm",           61.00,  9.00,  0, "F.Cu"),
    ("U5", "ICS-43434",         "Package_LCC:LGA-5_2x2.5mm_P0.8mm_LayoutBDA",      4.00, 27.00,  0, "F.Cu"),
    ("U1", "AMS1117-3.3",       "Package_TO_SOT_SMD:SOT-223-3_TabPin2",            12.00, 47.00,  0, "F.Cu"),
    ("J1", "USB_C_GCT_USB4105", "Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_Vertical", 35.00, 52.50,  0, "F.Cu"),
    ("J2", "JST_PH_B5B-PH-K",  "Connector_JST:JST_PH_B5B-PH-K_1x05_P2.00mm_Vertical", 35.00,  4.00,  0, "F.Cu"),
    ("J3", "Conn_UART_1x04",    "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical", 66.00, 10.00,  0, "F.Cu"),
    ("J4", "MQ2_Pads_1x04",    "Connector_PinHeader_2.54mm:PinHeader_2x02_P2.54mm_Vertical", 64.00, 27.00,  0, "F.Cu"),
    ("SW1","SW_RESET",          "Button_Switch_SMD:SW_Push_1P1T_NO_2.5x1.6mm_Vertical",  9.00, 51.00,  0, "F.Cu"),
    ("SW2","SW_BOOT",           "Button_Switch_SMD:SW_Push_1P1T_NO_2.5x1.6mm_Vertical", 59.00, 51.00,  0, "F.Cu"),
    ("D1", "LED_GREEN",         "LED_SMD:LED_0402_1005Metric",                     47.00, 51.00,  0, "F.Cu"),
    ("R3", "1k",                "Resistor_SMD:R_0402_1005Metric",                  43.00, 51.00,  0, "F.Cu"),
    ("R1", "5.1k",              "Resistor_SMD:R_0402_1005Metric",                  29.00, 51.00,  0, "F.Cu"),
    ("R2", "5.1k",              "Resistor_SMD:R_0402_1005Metric",                  27.00, 51.00,  0, "F.Cu"),
    ("R4", "10k",               "Resistor_SMD:R_0402_1005Metric",                  20.00, 42.00,  0, "F.Cu"),
    ("R5", "10k",               "Resistor_SMD:R_0402_1005Metric",                  20.00, 44.00,  0, "F.Cu"),
    ("R6", "4.7k",              "Resistor_SMD:R_0402_1005Metric",                  22.00, 18.00,  0, "F.Cu"),
    ("R7", "4.7k",              "Resistor_SMD:R_0402_1005Metric",                  24.00, 18.00,  0, "F.Cu"),
    ("R8", "10k",               "Resistor_SMD:R_0402_1005Metric",                  57.00, 27.00, 90, "F.Cu"),
    ("R9", "10k",               "Resistor_SMD:R_0402_1005Metric",                  57.00, 24.00, 90, "F.Cu"),
    ("R10","22k",               "Resistor_SMD:R_0402_1005Metric",                  57.00, 22.00, 90, "F.Cu"),
    ("R11","10k",               "Resistor_SMD:R_0402_1005Metric",                  27.00,  4.00,  0, "F.Cu"),
    ("C1", "10uF",              "Capacitor_SMD:C_0805_2012Metric",                 18.00, 47.00,  0, "F.Cu"),
    ("C2", "100nF",             "Capacitor_SMD:C_0402_1005Metric",                 21.00, 47.00,  0, "F.Cu"),
    ("C3", "10uF",              "Capacitor_SMD:C_0805_2012Metric",                 24.00, 47.00,  0, "F.Cu"),
    ("C4", "100nF",             "Capacitor_SMD:C_0402_1005Metric",                 27.00, 47.00,  0, "F.Cu"),
    ("C5", "100nF",             "Capacitor_SMD:C_0402_1005Metric",                 26.00, 18.00,  0, "F.Cu"),
    ("C6", "100nF",             "Capacitor_SMD:C_0402_1005Metric",                 28.00, 18.00,  0, "F.Cu"),
    ("C7", "10uF",              "Capacitor_SMD:C_0805_2012Metric",                 30.00, 18.00,  0, "F.Cu"),
    ("C8", "100nF",             "Capacitor_SMD:C_0402_1005Metric",                  7.00,  9.00, 90, "F.Cu"),
    ("C9", "100nF",             "Capacitor_SMD:C_0402_1005Metric",                 59.00,  9.00, 90, "F.Cu"),
    ("C10","100nF",             "Capacitor_SMD:C_0402_1005Metric",                  4.00, 24.00,  0, "F.Cu"),
]

# v2 = v1 + these additional components (board 80×60mm)
V2_EXTRA_COMPONENTS = [
    ("U6", "SIM7080G-M2M",     "RF_Module:SIM7080G-M2M",                          69.00, 27.00,  0, "F.Cu"),
    ("U7", "AMS1117-3.8",      "Package_TO_SOT_SMD:SOT-223-3_TabPin2",            73.00, 45.00,  0, "F.Cu"),
    ("J5", "Nano_SIM_Holder",  "Connector_Card:SIM_Wuerth_693071010811",          68.00, 50.00,  0, "F.Cu"),
    ("D2", "LED_BLUE",         "LED_SMD:LED_0402_1005Metric",                     73.00, 35.00,  0, "F.Cu"),
    ("D3", "PRTR5V0U2X",       "Package_TO_SOT_SMD:SOT-363_SC-70-6",             72.00, 50.00,  0, "F.Cu"),
    ("C11","470uF",            "Capacitor_SMD:C_1210_3225Metric",                 73.00, 40.00,  0, "F.Cu"),
    ("C12","100nF",            "Capacitor_SMD:C_0402_1005Metric",                 70.00, 43.00,  0, "F.Cu"),
    ("C13","10uF",             "Capacitor_SMD:C_0805_2012Metric",                 73.00, 43.00,  0, "F.Cu"),
    ("R12","10k",              "Resistor_SMD:R_0402_1005Metric",                  66.00, 35.00,  0, "F.Cu"),
    ("R13","470R",             "Resistor_SMD:R_0402_1005Metric",                  76.00, 35.00,  0, "F.Cu"),
]

LAYERS = """  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (32 "B.Adhes" user "B.Adhesive")
    (33 "F.Adhes" user "F.Adhesive")
    (34 "B.Paste" user)
    (35 "F.Paste" user)
    (36 "B.SilkS" user "B.Silkscreen")
    (37 "F.SilkS" user "F.Silkscreen")
    (38 "B.Mask" user)
    (39 "F.Mask" user)
    (40 "Dwgs.User" user "User.Drawings")
    (41 "Cmts.User" user "User.Comments")
    (42 "Eco1.User" user "User.Eco1")
    (43 "Eco2.User" user "User.Eco2")
    (44 "Edge.Cuts" user)
    (45 "Margin" user)
    (46 "B.CrtYd" user "B.Courtyard")
    (47 "F.CrtYd" user "F.Courtyard")
    (48 "B.Fab" user "B.Fab")
    (49 "F.Fab" user "F.Fab")
    (50 "User.1" user)
    (51 "User.2" user)
    (52 "User.3" user)
    (53 "User.4" user)
    (54 "User.5" user)
    (55 "User.6" user)
    (56 "User.7" user)
    (57 "User.8" user)
    (58 "User.9" user)
  )"""

SETUP = """  (setup
    (pad_to_mask_clearance 0)
    (allow_soldermask_bridges_in_footprints no)
    (pcbplotparams
      (layerselection 0x00010fc_ffffffff)
      (plot_on_all_layers_selection 0x0000000_00000000)
      (disableapertmacros no)
      (usegerberextensions no)
      (usegerberattributes yes)
      (usegerberadvancedattributes yes)
      (creategerberjobfile yes)
      (dashed_line_dash_ratio 12.000000)
      (dashed_line_gap_ratio 3.000000)
      (svgprecision 4)
      (plotframeref no)
      (viasonmask no)
      (mode 1)
      (useauxorigin no)
      (hpglpennumber 1)
      (hpglpenspeed 20)
      (hpglpendiameter 15.000000)
      (dxfpolygonmode yes)
      (dxfimperialunits yes)
      (dxfusepcbnewfont yes)
      (psnegative no)
      (psa4output no)
      (plotreference yes)
      (plotvalue yes)
      (plotfptext yes)
      (plotinvisibletext no)
      (sketchpadsonfab no)
      (subtractmaskfromsilk no)
      (outputformat 1)
      (mirror no)
      (drillshape 1)
      (scaleselection 1)
      (outputdirectory "gerbers/")
    )
  )"""

NET_DECLARATIONS = """  (net 0 "")
  (net 1 "GND")
  (net 2 "+3V3")
  (net 3 "+5V")
  (net 4 "I2C_SDA")
  (net 5 "I2C_SCL")
  (net 6 "I2S_WS")
  (net 7 "I2S_SCK")
  (net 8 "I2S_SD")
  (net 9 "ADC_MQ2")
  (net 10 "PPD42_OUT")
  (net 11 "UART_TX")
  (net 12 "UART_RX")
  (net 13 "EN_RST")
  (net 14 "BOOT_IO9")
  (net 15 "USB_DM")
  (net 16 "USB_DP")
  (net 17 "USB_CC1")
  (net 18 "USB_CC2")"""

NET_DECLARATIONS_V2 = NET_DECLARATIONS + """
  (net 19 "+3V8")
  (net 20 "SIM_TX")
  (net 21 "SIM_RX")
  (net 22 "SIM_PWRKEY")
  (net 23 "SIM_STATUS")
  (net 24 "SIM_VDD")
  (net 25 "SIM_DATA")
  (net 26 "SIM_CLK")
  (net 27 "SIM_RST")
  (net 28 "NETLIGHT")"""


def make_footprint(ref, value, lib, x, y, rot, layer):
    """Generate a KiCad footprint reference block."""
    return f"""  (footprint "{lib}" (layer "{layer}")
    (at {x} {y} {rot})
    (descr "{value}")
    (tags "{ref}")
    (property "Reference" "{ref}" (at 0 -2.5 0) (layer "F.SilkS")
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (property "Value" "{value}" (at 0 2.5 0) (layer "F.Fab")
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (property "Footprint" "{lib}" (at 0 0 0) (layer "F.Fab") (hide yes)
      (effects (font (size 1 1) (thickness 0.15)))
    )
    (property "Datasheet" "" (at 0 0 0) (layer "F.Fab") (hide yes)
      (effects (font (size 1 1) (thickness 0.15)))
    )
  )"""


def make_board_outline(w, h, radius=0.8):
    """Generate Edge.Cuts board outline as rounded rect."""
    lines = [
        f"  (gr_rect (start 0 0) (end {w} {h}) (layer \"Edge.Cuts\") (width 0.05) (fill none))",
    ]
    # Mounting holes (M3, 3mm from corners)
    for mx, my in [(3, 3), (w-3, 3), (3, h-3), (w-3, h-3)]:
        lines.append(
            f"  (footprint \"MountingHole:MountingHole_3.2mm_M3\" (layer \"F.Cu\")\n"
            f"    (at {mx} {my})\n"
            f"    (property \"Reference\" \"H\" (at 0 0 0) (layer \"F.Fab\") (hide yes)\n"
            f"      (effects (font (size 1 1) (thickness 0.15)))\n"
            f"    )\n"
            f"    (property \"Value\" \"MountingHole\" (at 0 0 0) (layer \"F.Fab\") (hide yes)\n"
            f"      (effects (font (size 1 1) (thickness 0.15)))\n"
            f"    )\n"
            f"  )"
        )
    return "\n".join(lines)


def make_title_block(title, rev, company):
    return f"""  (title_block
    (title "{title}")
    (date "2026-06-01")
    (rev "{rev}")
    (company "{company}")
  )"""


def generate_pcb(filename, title, rev, board_w, board_h, components, net_decls):
    with open(filename, 'w') as f:
        f.write(f"(kicad_pcb (version 20230121) (generator \"pcbnew\")\n\n")
        f.write(f"  (general\n    (thickness 1.6)\n    (legacy_teardrops no)\n  )\n\n")
        f.write(f"  (paper \"A3\")\n\n")
        f.write(make_title_block(title, rev, "Emprendimientos Varios IAV") + "\n\n")
        f.write(LAYERS + "\n\n")
        f.write(SETUP + "\n\n")
        f.write(net_decls + "\n\n")

        # Board outline + mounting holes
        f.write(make_board_outline(board_w, board_h) + "\n\n")

        # Courtyard rectangle (1mm inside board edge)
        f.write(f"  (gr_rect (start 1 1) (end {board_w-1} {board_h-1})\n"
                f"    (layer \"F.CrtYd\") (width 0.05) (fill none))\n\n")

        # All component footprints
        for (ref, value, lib, x, y, rot, layer) in components:
            f.write(make_footprint(ref, value, lib, x, y, rot, layer) + "\n\n")

        # JLCPCB 2-layer design rules comment
        f.write("  ;; JLCPCB 2-layer design rules:\n")
        f.write("  ;; Min track: 0.09mm (use 0.2mm), Min clearance: 0.1mm (use 0.2mm)\n")
        f.write("  ;; Min drill: 0.2mm, Min annular ring: 0.1mm\n")
        f.write("  ;; To generate Gerbers: File → Fabrication Outputs → Gerbers\n")
        f.write("  ;; To auto-route: Tools → External Plugins → Freerouting\n\n")

        f.write(")\n")
    print(f"Generated {filename} ({board_w}×{board_h}mm, {len(components)} components)")


if __name__ == "__main__":
    import os
    os.chdir("/home/ia/.paperclip/instances/default/workspaces/41a58b01-d78f-4b19-b2c0-29bc504cdac1/proyecto-monitoreo-contaminacion/hardware/pcb")

    # V1: WiFi-only, 70×55mm
    generate_pcb(
        "pollution-monitor-v1.kicad_pcb",
        "Monitor Contaminacion PCB v1 - WiFi Only",
        "1.0",
        70, 55,
        V1_COMPONENTS,
        NET_DECLARATIONS
    )

    # V2: WiFi + LTE-M, 80×60mm
    v2_components = V1_COMPONENTS.copy()
    v2_components.extend(V2_EXTRA_COMPONENTS)
    generate_pcb(
        "pollution-monitor-v2-ltem.kicad_pcb",
        "Monitor Contaminacion PCB v2 - WiFi + LTE-M",
        "2.0",
        80, 60,
        v2_components,
        NET_DECLARATIONS_V2
    )

    print("\nFiles ready. Next steps:")
    print("1. Open .kicad_pcb in KiCad 7/8")
    print("2. Tools → Update Footprints from Library (load actual pad geometry)")
    print("3. Tools → Freerouting → Start Freerouting (auto-route)")
    print("4. File → Fabrication Outputs → Gerbers → Generate")
    print("5. Upload Gerber ZIP to JLCPCB.com")
