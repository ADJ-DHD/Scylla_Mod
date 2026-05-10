BOOTLOADER = rp2040

SPLIT_KEYBOARD = yes
SERIAL_DRIVER = vendor
SPI_DRIVER = hardware

QUANTUM_PAINTER_ENABLE = yes
QUANTUM_PAINTER_DRIVERS += ili9341_spi

ENCODER_ENABLE = yes
ENCODER_MAP_ENABLE = yes

EXTRAKEY_ENABLE = yes
KEY_OVERRIDE_ENABLE = yes
MOUSEKEY_ENABLE = yes

VIA_ENABLE = yes
VIAL_ENABLE = yes
WPM_ENABLE = yes

VIAL_INSECURE = yes

SRC += assets/idle_240x320_cropped.qgf.c
SRC += assets/segoeui.qff.c
SRC += assets/bongocat_base.qgf.c